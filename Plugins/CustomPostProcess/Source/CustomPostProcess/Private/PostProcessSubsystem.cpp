// Fill out your copyright notice in the Description page of Project Settings.


#include "PostProcessSubsystem.h"
#include "PostProcessLensFlareAsset.h"

#include "RenderGraph.h"
#include "ScreenPass.h"
#include "PostProcess/PostProcessLensFlares.h"

TAutoConsoleVariable<int32> CVarLensFlareRenderBloom(
    TEXT("r.LensFlare.RenderBloom"),
    1,
    TEXT(" 0: Don't mix Bloom into lens-flare\n")
    TEXT(" 1: Mix the Bloom into the lens-flare"),
    ECVF_RenderThreadSafe);

TAutoConsoleVariable<int32> CVarLensFlareRenderFlarePass(
    TEXT("r.LensFlare.RenderFlare"),
    1,
    TEXT(" 0: Don't render flare pass\n")
    TEXT(" 1: Render flare pass (ghosts and halos)"),
    ECVF_RenderThreadSafe);

TAutoConsoleVariable<int32> CVarLensFlareRenderGlarePass(
    TEXT("r.LensFlare.RenderGlare"),
    1,
    TEXT(" 0: Don't render glare pass\n")
    TEXT(" 1: Render flare pass (star shape)"),
    ECVF_RenderThreadSafe);

DECLARE_GPU_STAT(LensFlaresFroyok)

FVector2D GetInputViewportSize(const FIntRect& Input, const FIntPoint& Extent)
{
    // Based on
    // GetScreenPassTextureViewportParameters()
    // Engine/Source/Runtime/Renderer/Private/ScreenPass.cpp

    FVector2D ExtentInverse = FVector2D(1.0f / Extent.X, 1.0f / Extent.Y);

    FVector2D RectMin = FVector2D(Input.Min);
    FVector2D RectMax = FVector2D(Input.Max);

    FVector2D Min = RectMin * ExtentInverse;
    FVector2D Max = RectMax * ExtentInverse;

    return (Max - Min);
}

// The function that draw a shader into a given RenderGraph texture
template<typename TShaderParameters, typename TShaderClassVertex, typename TShaderClassPixel>
inline void DrawShaderPass(
    FRDGBuilder& GraphBuilder,
    const FString& PassName,
    TShaderParameters* PassParameters,
    TShaderMapRef<TShaderClassVertex> VertexShader,
    TShaderMapRef<TShaderClassPixel> PixelShader,
    FRHIBlendState* BlendState,
    const FIntRect& Viewport
)
{
    const FScreenPassPipelineState PipelineState(VertexShader, PixelShader, BlendState);

    GraphBuilder.AddPass(
        FRDGEventName(TEXT("%s"), *PassName),
        PassParameters,
        ERDGPassFlags::Raster,
        [PixelShader, PassParameters, Viewport, PipelineState](FRHICommandListImmediate& RHICmdList)
        {
            RHICmdList.SetViewport(
                Viewport.Min.X, Viewport.Min.Y, 0.0f,
                Viewport.Max.X, Viewport.Max.Y, 1.0f
            );

            SetScreenPassPipelineState(RHICmdList, PipelineState);

            SetShaderParameters(
                RHICmdList,
                PixelShader,
                PixelShader.GetPixelShader(),
                *PassParameters
            );

            DrawRectangle(
                RHICmdList,                             // FRHICommandList
                0.0f, 0.0f,                             // float X, float Y
                Viewport.Width(), Viewport.Height(),  // float SizeX, float SizeY
                Viewport.Min.X, Viewport.Min.Y,     // float U, float V
                Viewport.Width(),                       // float SizeU
                Viewport.Height(),                      // float SizeV
                Viewport.Size(),                        // FIntPoint TargetSize
                Viewport.Size(),                        // FIntPoint TextureSize
                PipelineState.VertexShader,             // const TShaderRefBase VertexShader
                EDrawRectangleFlags::EDRF_Default       // EDrawRectangleFlags Flags
            );
        });
}

void UPostProcessSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    //--------------------------------
    // Delegate setup
    //--------------------------------
    FPP_LensFlares::FDelegate Delegate = FPP_LensFlares::FDelegate::CreateLambda(
        [=](FRDGBuilder& GraphBuilder, const FViewInfo& View, const FLensFlareInputs& Inputs, FLensFlareOutputsData& Outputs)
        {
            RenderLensFlare(GraphBuilder, View, Inputs, Outputs);
        });

    ENQUEUE_RENDER_COMMAND(BindRenderThreadDelegates)([Delegate](FRHICommandListImmediate& RHICmdList)
        {
            PP_LensFlares.Add(Delegate);
        });


    //--------------------------------
    // Data asset loading
    //--------------------------------
    FString Path = "PostProcessLensFlareAsset'/CustomPostProcess/DefaultLensFlare.DefaultLensFlare'";


    PostProcessAsset = LoadObject<UPostProcessLensFlareAsset>(nullptr, *Path);
    check(PostProcessAsset);
}

void UPostProcessSubsystem::Deinitialize()
{
    ClearBlendState = nullptr;
    AdditiveBlendState = nullptr;
    BilinearClampSampler = nullptr;
    BilinearBorderSampler = nullptr;
    BilinearRepeatSampler = nullptr;
    NearestRepeatSampler = nullptr;
}

void UPostProcessSubsystem::RenderLensFlare(
    FRDGBuilder& GraphBuilder,
    const FViewInfo& View,
    const FLensFlareInputs& Inputs,
    FLensFlareOutputsData& Outputs
)
{
    check(Inputs.Bloom.IsValid());
    check(Inputs.HalfSceneColor.IsValid());

    if (PostProcessAsset == nullptr)
    {
        return;
    }

    RDG_GPU_STAT_SCOPE(GraphBuilder, LensFlaresFroyok)
    RDG_EVENT_SCOPE(GraphBuilder, "LensFlaresFroyok"); const FScreenPassTextureViewport BloomViewport(Inputs.Bloom);
    const FVector2D BloomInputViewportSize = GetInputViewportSize(BloomViewport.Rect, BloomViewport.Extent);

    const FScreenPassTextureViewport SceneColorViewport(Inputs.HalfSceneColor);
    const FVector2D SceneColorViewportSize = GetInputViewportSize(SceneColorViewport.Rect, SceneColorViewport.Extent);

    // Input
    FRDGTextureRef InputTexture = Inputs.HalfSceneColor.Texture;
    FIntRect InputRect = SceneColorViewport.Rect;

    // Outputs
    FRDGTextureRef OutputTexture = Inputs.HalfSceneColor.Texture;
    FIntRect OutputRect = SceneColorViewport.Rect;

    // States
    if (ClearBlendState == nullptr)
    {
        // Blend modes from:
        // '/Engine/Source/Runtime/RenderCore/Private/ClearQuad.cpp'
        // '/Engine/Source/Runtime/Renderer/Private/PostProcess/PostProcessMaterial.cpp'
        ClearBlendState = TStaticBlendState<>::GetRHI();
        AdditiveBlendState = TStaticBlendState<CW_RGB, BO_Add, BF_One, BF_One>::GetRHI();

        BilinearClampSampler = TStaticSamplerState<SF_Bilinear, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI();
        BilinearBorderSampler = TStaticSamplerState<SF_Bilinear, AM_Border, AM_Border, AM_Border>::GetRHI();
        BilinearRepeatSampler = TStaticSamplerState<SF_Bilinear, AM_Wrap, AM_Wrap, AM_Wrap>::GetRHI();
        NearestRepeatSampler = TStaticSamplerState<SF_Point, AM_Wrap, AM_Wrap, AM_Wrap>::GetRHI();
    }


    // TODO_RESCALE


    ////////////////////////////////////////////////////////////////////////
    // Render passes
    ////////////////////////////////////////////////////////////////////////
    FRDGTextureRef ThresholdTexture = nullptr;
    FRDGTextureRef FlareTexture = nullptr;
    FRDGTextureRef GlareTexture = nullptr;

    ThresholdTexture = RenderThreshold(
        GraphBuilder,
        InputTexture,
        InputRect,
        View
    );

    if (CVarLensFlareRenderFlarePass.GetValueOnRenderThread())
    {
        FlareTexture = RenderFlare(
            GraphBuilder,
            ThresholdTexture,
            InputRect,
            View
        );
    }

    if (CVarLensFlareRenderGlarePass.GetValueOnRenderThread())
    {
        GlareTexture = RenderGlare(
            GraphBuilder,
            ThresholdTexture,
            InputRect,
            View
        );
    }


    // TODO_MIX


    ////////////////////////////////////////////////////////////////////////
    // Final Output
    ////////////////////////////////////////////////////////////////////////
    Outputs.Texture = OutputTexture;
    Outputs.Rect = OutputRect;

} // End RenderLensFlare()

FRDGTextureRef UPostProcessSubsystem::RenderBlur(
    FRDGBuilder& GraphBuilder,
    FRDGTextureRef InputTexture,
    const FViewInfo& View,
    const FIntRect& Viewport,
    int BlurSteps
)
{
    // TODO_BLUR
    return nullptr;
}

FRDGTextureRef UPostProcessSubsystem::RenderThreshold(
    FRDGBuilder& GraphBuilder,
    FRDGTextureRef InputTexture,
    FIntRect& InputRect,
    const FViewInfo& View
)
{
    // TODO_THRESHOLD

    // TODO_THRESHOLD_BLUR
    return nullptr;
}

FRDGTextureRef UPostProcessSubsystem::RenderFlare(
    FRDGBuilder& GraphBuilder,
    FRDGTextureRef InputTexture,
    FIntRect& InputRect,
    const FViewInfo& View
)
{
    // TODO_FLARE_CHROMA

    // TODO_FLARE_GHOSTS

    // TODO_FLARE_HALO
    return nullptr;
}

FRDGTextureRef UPostProcessSubsystem::RenderGlare(
    FRDGBuilder& GraphBuilder,
    FRDGTextureRef InputTexture,
    FIntRect& InputRect,
    const FViewInfo& View
)
{
    // TODO_GLARE
    return nullptr;
}