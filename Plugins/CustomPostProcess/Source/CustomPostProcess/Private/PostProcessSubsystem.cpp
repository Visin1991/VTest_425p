// Fill out your copyright notice in the Description page of Project Settings.


#include "PostProcessSubsystem.h"
#include "PostProcessLensFlareAsset.h"

#include "RenderGraph.h"
#include "ScreenPass.h"
#include "PostProcess/PostProcessLensFlares.h"

namespace
{
// RDG buffer input shared by all passes
BEGIN_SHADER_PARAMETER_STRUCT(FCustomLensFlarePassParameters, )
SHADER_PARAMETER_RDG_TEXTURE(Texture2D, InputTexture)
RENDER_TARGET_BINDING_SLOTS()
END_SHADER_PARAMETER_STRUCT()

// The vertex shader to draw a rectangle.
class FCustomScreenPassVS : public FGlobalShader
{
public:
    DECLARE_GLOBAL_SHADER(FCustomScreenPassVS);

    static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters&)
    {
        return true;
    }

    FCustomScreenPassVS() = default;
    FCustomScreenPassVS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
        : FGlobalShader(Initializer)
    {}
};
IMPLEMENT_GLOBAL_SHADER(FCustomScreenPassVS, "/CustomShaders/ScreenPass.usf", "CustomScreenPassVS", SF_Vertex);

#if WITH_EDITOR
// Rescale shader
class FLensFlareRescalePS : public FGlobalShader
{
public:
    DECLARE_GLOBAL_SHADER(FLensFlareRescalePS);
    SHADER_USE_PARAMETER_STRUCT(FLensFlareRescalePS, FGlobalShader);

    BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
        SHADER_PARAMETER_STRUCT_INCLUDE(FCustomLensFlarePassParameters, Pass)
        SHADER_PARAMETER_SAMPLER(SamplerState, InputSampler)
        SHADER_PARAMETER(FVector2D, InputViewportSize)
        END_SHADER_PARAMETER_STRUCT()

        static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
    {
        return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
    }
};
IMPLEMENT_GLOBAL_SHADER(FLensFlareRescalePS, "/CustomShaders/Rescale.usf", "RescalePS", SF_Pixel);
#endif

// Downsample shader
class FDownsamplePS : public FGlobalShader
{
public:
    DECLARE_GLOBAL_SHADER(FDownsamplePS);
    SHADER_USE_PARAMETER_STRUCT(FDownsamplePS, FGlobalShader);

    BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
        SHADER_PARAMETER_STRUCT_INCLUDE(FCustomLensFlarePassParameters, Pass)
        SHADER_PARAMETER_SAMPLER(SamplerState, InputSampler)
        SHADER_PARAMETER(FVector2D, InputSize)
        SHADER_PARAMETER(float, ThresholdLevel)
        SHADER_PARAMETER(float, ThresholdRange)
        END_SHADER_PARAMETER_STRUCT()

        static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
    {
        return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
    }
};
IMPLEMENT_GLOBAL_SHADER(FDownsamplePS, "/CustomShaders/DownsampleThreshold.usf", "DownsampleThresholdPS", SF_Pixel);

    // TODO_SHADER_KAWASE
    // Blur shader (use Dual Kawase method)
class FKawaseBlurDownPS : public FGlobalShader
{
public:
    DECLARE_GLOBAL_SHADER(FKawaseBlurDownPS);
    SHADER_USE_PARAMETER_STRUCT(FKawaseBlurDownPS, FGlobalShader);

    BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
        SHADER_PARAMETER_STRUCT_INCLUDE(FCustomLensFlarePassParameters, Pass)
        SHADER_PARAMETER_SAMPLER(SamplerState, InputSampler)
        SHADER_PARAMETER(FVector2D, BufferSize)
        END_SHADER_PARAMETER_STRUCT()

        static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
    {
        return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
    }
};
class FKawaseBlurUpPS : public FGlobalShader
{
public:
    DECLARE_GLOBAL_SHADER(FKawaseBlurUpPS);
    SHADER_USE_PARAMETER_STRUCT(FKawaseBlurUpPS, FGlobalShader);

    BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
        SHADER_PARAMETER_STRUCT_INCLUDE(FCustomLensFlarePassParameters, Pass)
        SHADER_PARAMETER_SAMPLER(SamplerState, InputSampler)
        SHADER_PARAMETER(FVector2D, BufferSize)
        END_SHADER_PARAMETER_STRUCT()

        static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
    {
        return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
    }
};
IMPLEMENT_GLOBAL_SHADER(FKawaseBlurDownPS, "/CustomShaders/DualKawaseBlur.usf", "KawaseBlurDownsamplePS", SF_Pixel);
IMPLEMENT_GLOBAL_SHADER(FKawaseBlurUpPS, "/CustomShaders/DualKawaseBlur.usf", "KawaseBlurUpsamplePS", SF_Pixel);

// TODO_SHADER_CHROMA
// Chromatic shift shader
class FLensFlareChromaPS : public FGlobalShader
{
public:
    DECLARE_GLOBAL_SHADER(FLensFlareChromaPS);
    SHADER_USE_PARAMETER_STRUCT(FLensFlareChromaPS, FGlobalShader);

    BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
        SHADER_PARAMETER_STRUCT_INCLUDE(FCustomLensFlarePassParameters, Pass)
        SHADER_PARAMETER_SAMPLER(SamplerState, InputSampler)
        SHADER_PARAMETER(float, ChromaShift)
    END_SHADER_PARAMETER_STRUCT()

    static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
    {
        return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
    }
};
IMPLEMENT_GLOBAL_SHADER(FLensFlareChromaPS, "/CustomShaders/Chroma.usf", "ChromaPS", SF_Pixel);

// Ghost shader
class FLensFlareGhostsPS : public FGlobalShader
{
public:
    DECLARE_GLOBAL_SHADER(FLensFlareGhostsPS);
    SHADER_USE_PARAMETER_STRUCT(FLensFlareGhostsPS, FGlobalShader);

    BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
        SHADER_PARAMETER_STRUCT_INCLUDE(FCustomLensFlarePassParameters, Pass)
        SHADER_PARAMETER_SAMPLER(SamplerState, InputSampler)
        SHADER_PARAMETER_ARRAY(FVector4, GhostColors, [8])
        SHADER_PARAMETER_ARRAY(float, GhostScales, [8])
        SHADER_PARAMETER(float, Intensity)
        END_SHADER_PARAMETER_STRUCT()

        static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
    {
        return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
    }
};
IMPLEMENT_GLOBAL_SHADER(FLensFlareGhostsPS, "/CustomShaders/Ghosts.usf", "GhostsPS", SF_Pixel);

 // TODO_SHADER_HALO
class FLensFlareHaloPS : public FGlobalShader
{
public:
    DECLARE_GLOBAL_SHADER(FLensFlareHaloPS);
    SHADER_USE_PARAMETER_STRUCT(FLensFlareHaloPS, FGlobalShader);

    BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
        SHADER_PARAMETER_STRUCT_INCLUDE(FCustomLensFlarePassParameters, Pass)
        SHADER_PARAMETER_SAMPLER(SamplerState, InputSampler)
        SHADER_PARAMETER(float, Width)
        SHADER_PARAMETER(float, Mask)
        SHADER_PARAMETER(float, Compression)
        SHADER_PARAMETER(float, Intensity)
        SHADER_PARAMETER(float, ChromaShift)
        END_SHADER_PARAMETER_STRUCT()

        static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
    {
        return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
    }
};
IMPLEMENT_GLOBAL_SHADER(FLensFlareHaloPS, "/CustomShaders/Halo.usf", "HaloPS", SF_Pixel);

// TODO_SHADER_GLARE
// Glare shader pass
class FLensFlareGlareVS : public FGlobalShader
{
public:
    DECLARE_GLOBAL_SHADER(FLensFlareGlareVS);
    SHADER_USE_PARAMETER_STRUCT(FLensFlareGlareVS, FGlobalShader);

    BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
        SHADER_PARAMETER_STRUCT_INCLUDE(FCustomLensFlarePassParameters, Pass)
        SHADER_PARAMETER_SAMPLER(SamplerState, InputSampler)
        SHADER_PARAMETER(FIntPoint, TileCount)
        SHADER_PARAMETER(FVector4, PixelSize)
        SHADER_PARAMETER(FVector2D, BufferSize)
        END_SHADER_PARAMETER_STRUCT()
};
class FLensFlareGlareGS : public FGlobalShader
{
public:
    DECLARE_GLOBAL_SHADER(FLensFlareGlareGS);
    SHADER_USE_PARAMETER_STRUCT(FLensFlareGlareGS, FGlobalShader);

    BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
        SHADER_PARAMETER(FVector4, PixelSize)
        SHADER_PARAMETER(FVector2D, BufferSize)
        SHADER_PARAMETER(FVector2D, BufferRatio)
        SHADER_PARAMETER(float, GlareIntensity)
        SHADER_PARAMETER(float, GlareDivider)
        SHADER_PARAMETER(FVector4, GlareTint)
        SHADER_PARAMETER_ARRAY(float, GlareScales, [3])
        END_SHADER_PARAMETER_STRUCT()
};
class FLensFlareGlarePS : public FGlobalShader
{
public:
    DECLARE_GLOBAL_SHADER(FLensFlareGlarePS);
    SHADER_USE_PARAMETER_STRUCT(FLensFlareGlarePS, FGlobalShader);

    BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
        SHADER_PARAMETER_SAMPLER(SamplerState, GlareSampler)
        SHADER_PARAMETER_TEXTURE(Texture2D, GlareTexture)
        END_SHADER_PARAMETER_STRUCT()

    static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
    {
        return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
    }
};
IMPLEMENT_GLOBAL_SHADER(FLensFlareGlareVS, "/CustomShaders/Glare.usf", "GlareVS", SF_Vertex);
IMPLEMENT_GLOBAL_SHADER(FLensFlareGlareGS, "/CustomShaders/Glare.usf", "GlareGS", SF_Geometry);
IMPLEMENT_GLOBAL_SHADER(FLensFlareGlarePS, "/CustomShaders/Glare.usf", "GlarePS", SF_Pixel);

// Final bloom mix shader
class FLensFlareBloomMixPS : public FGlobalShader
{
public:
    DECLARE_GLOBAL_SHADER(FLensFlareBloomMixPS);
    SHADER_USE_PARAMETER_STRUCT(FLensFlareBloomMixPS, FGlobalShader);

    BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
        SHADER_PARAMETER_STRUCT_INCLUDE(FCustomLensFlarePassParameters, Pass)
        SHADER_PARAMETER_SAMPLER(SamplerState, InputSampler)
        SHADER_PARAMETER_RDG_TEXTURE(Texture2D, BloomTexture)
        SHADER_PARAMETER_RDG_TEXTURE(Texture2D, GlareTexture)
        SHADER_PARAMETER_TEXTURE(Texture2D, GradientTexture)
        SHADER_PARAMETER_SAMPLER(SamplerState, GradientSampler)
        SHADER_PARAMETER(FVector4, Tint)
        SHADER_PARAMETER(FVector2D, InputViewportSize)
        SHADER_PARAMETER(FVector2D, BufferSize)
        SHADER_PARAMETER(FVector2D, PixelSize)
        SHADER_PARAMETER(FIntVector, MixPass)
        SHADER_PARAMETER(float, Intensity)
        END_SHADER_PARAMETER_STRUCT()

        static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
    {
        return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
    }
};
IMPLEMENT_GLOBAL_SHADER(FLensFlareBloomMixPS, "/CustomShaders/Mix.usf", "MixPS", SF_Pixel);


}

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
#if WITH_EDITOR
    if (SceneColorViewport.Rect.Width() != SceneColorViewport.Extent.X
        || SceneColorViewport.Rect.Height() != SceneColorViewport.Extent.Y)
    {
        const FString PassName("LensFlareRescale");

        // Build target buffer
        FRDGTextureDesc Desc = Inputs.HalfSceneColor.Texture->Desc;
        Desc.Reset();
        Desc.Extent = SceneColorViewport.Rect.Size();
        Desc.Format = PF_FloatRGB;
        Desc.ClearValue = FClearValueBinding(FLinearColor::Transparent);
        FRDGTextureRef RescaleTexture = GraphBuilder.CreateTexture(Desc, *PassName);

        // Setup shaders
        TShaderMapRef<FCustomScreenPassVS> VertexShader(View.ShaderMap);
        TShaderMapRef<FLensFlareRescalePS> PixelShader(View.ShaderMap);

        // Setup shader parameters
        FLensFlareRescalePS::FParameters* PassParameters = GraphBuilder.AllocParameters<FLensFlareRescalePS::FParameters>();
        PassParameters->Pass.InputTexture = Inputs.HalfSceneColor.Texture;
        PassParameters->Pass.RenderTargets[0] = FRenderTargetBinding(RescaleTexture, ERenderTargetLoadAction::ENoAction);
        PassParameters->InputSampler = BilinearClampSampler;
        PassParameters->InputViewportSize = SceneColorViewportSize;

        // Render shader into buffer
        DrawShaderPass(
            GraphBuilder,
            PassName,
            PassParameters,
            VertexShader,
            PixelShader,
            ClearBlendState,
            SceneColorViewport.Rect
        );

        // Assign result before end of scope
        InputTexture = RescaleTexture;
    }
#endif


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


    //TODO_MIX
    {
        const FString PassName("LensFlareMix");

        FIntRect MixViewport = FIntRect(
            0,
            0,
            View.ViewRect.Width() / 2,
            View.ViewRect.Height() / 2
        );

        FVector2D BufferSize = FVector2D(MixViewport.Width(), MixViewport.Height());

        // Create buffer
        FRDGTextureDesc Description = Inputs.Bloom.Texture->Desc;
        Description.Reset();
        Description.Extent = MixViewport.Size();
        Description.Format = PF_FloatRGBA;
        Description.ClearValue = FClearValueBinding(FLinearColor::Transparent);
        FRDGTextureRef MixTexture = GraphBuilder.CreateTexture(Description, *PassName);

        // Shader parameters
        TShaderMapRef<FCustomScreenPassVS> VertexShader(View.ShaderMap);
        TShaderMapRef<FLensFlareBloomMixPS> PixelShader(View.ShaderMap);

        FLensFlareBloomMixPS::FParameters* PassParameters = GraphBuilder.AllocParameters<FLensFlareBloomMixPS::FParameters>();
        PassParameters->Pass.RenderTargets[0] = FRenderTargetBinding(MixTexture, ERenderTargetLoadAction::ENoAction);
        PassParameters->InputSampler = BilinearClampSampler;
        PassParameters->GradientTexture = GWhiteTexture->TextureRHI;
        PassParameters->GradientSampler = BilinearClampSampler;
        PassParameters->BufferSize = BufferSize;
        PassParameters->PixelSize = FVector2D(1.0f, 1.0f) / BufferSize;
        PassParameters->InputViewportSize = BloomInputViewportSize;
        PassParameters->Tint = FVector4(PostProcessAsset->Tint);
        PassParameters->Intensity = PostProcessAsset->Intensity;

        if (PostProcessAsset->Gradient != nullptr)
        {
            const FTextureRHIRef TextureRHI = PostProcessAsset->Gradient->Resource->TextureRHI;
            PassParameters->GradientTexture = TextureRHI;
        }
        // Plug in buffers
        const int32 MixBloomPass = CVarLensFlareRenderBloom.GetValueOnRenderThread();

        PassParameters->MixPass = FIntVector(
            (Inputs.bCompositeWithBloom && MixBloomPass),
            (FlareTexture != nullptr),
            (GlareTexture != nullptr)
        );

        if (Inputs.bCompositeWithBloom && MixBloomPass)
        {
            PassParameters->BloomTexture = Inputs.Bloom.Texture;
        }
        else
        {
            PassParameters->BloomTexture = InputTexture;
        }

        if (FlareTexture != nullptr)
        {
            PassParameters->Pass.InputTexture = FlareTexture;
        }
        else
        {
            PassParameters->Pass.InputTexture = InputTexture;
        }

        if (GlareTexture != nullptr)
        {
            PassParameters->GlareTexture = GlareTexture;
        }
        else
        {
            PassParameters->GlareTexture = InputTexture;
        }

        // Render
        DrawShaderPass(
            GraphBuilder,
            PassName,
            PassParameters,
            VertexShader,
            PixelShader,
            ClearBlendState,
            MixViewport
        );

        OutputTexture = MixTexture;
        OutputRect = MixViewport;
    }

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
    // Shader setup
    TShaderMapRef<FCustomScreenPassVS>  VertexShader(View.ShaderMap);
    TShaderMapRef<FKawaseBlurDownPS>    PixelShaderDown(View.ShaderMap);
    TShaderMapRef<FKawaseBlurUpPS>      PixelShaderUp(View.ShaderMap);

    // Data setup
    FRDGTextureRef PreviousBuffer = InputTexture;
    const FRDGTextureDesc& InputDescription = InputTexture->Desc;

    const FString PassDownName  = TEXT("Down");
    const FString PassUpName    = TEXT("Up");
    const int32 ArraySize = BlurSteps * 2;

    // Viewport resolutions
    // Could have been a bit more clever and avoid duplicate
    // sizes for upscale passes but heh... it works.
    int32 Divider = 2;
    TArray<FIntRect> Viewports;
    for( int32 i = 0; i < ArraySize; i++ )
    {
        FIntRect NewRect = FIntRect(
            0,
            0,
            Viewport.Width() / Divider,
            Viewport.Height() / Divider
        );

        Viewports.Add( NewRect );

        if( i < (BlurSteps - 1) )
        {
            Divider *= 2;
        }
        else
        {
            Divider /= 2;
        }
    }

    // Render
    for (int32 i = 0; i < ArraySize; i++)
    {
        // Build texture
        FRDGTextureDesc BlurDesc = InputDescription;
        BlurDesc.Reset();
        BlurDesc.Extent = Viewports[i].Size();
        BlurDesc.Format = PF_FloatRGB;
        BlurDesc.NumMips = 1;
        BlurDesc.ClearValue = FClearValueBinding(FLinearColor::Transparent);

        FVector2D ViewportResolution = FVector2D(
            Viewports[i].Width(),
            Viewports[i].Height()
        );

        const FString PassName =
            FString("KawaseBlur")
            + FString::Printf(TEXT("_%i_"), i)
            + ((i < BlurSteps) ? PassDownName : PassUpName)
            + FString::Printf(TEXT("_%ix%i"), Viewports[i].Width(), Viewports[i].Height());

        FRDGTextureRef Buffer = GraphBuilder.CreateTexture(BlurDesc, *PassName);

        // Render shader
        if (i < BlurSteps)
        {
            FKawaseBlurDownPS::FParameters* PassDownParameters = GraphBuilder.AllocParameters<FKawaseBlurDownPS::FParameters>();
            PassDownParameters->Pass.InputTexture = PreviousBuffer;
            PassDownParameters->Pass.RenderTargets[0] = FRenderTargetBinding(Buffer, ERenderTargetLoadAction::ENoAction);
            PassDownParameters->InputSampler = BilinearClampSampler;
            PassDownParameters->BufferSize = ViewportResolution;

            DrawShaderPass(
                GraphBuilder,
                PassName,
                PassDownParameters,
                VertexShader,
                PixelShaderDown,
                ClearBlendState,
                Viewports[i]
            );
        }
        else
        {
            FKawaseBlurUpPS::FParameters* PassUpParameters = GraphBuilder.AllocParameters<FKawaseBlurUpPS::FParameters>();
            PassUpParameters->Pass.InputTexture = PreviousBuffer;
            PassUpParameters->Pass.RenderTargets[0] = FRenderTargetBinding(Buffer, ERenderTargetLoadAction::ENoAction);
            PassUpParameters->InputSampler = BilinearClampSampler;
            PassUpParameters->BufferSize = ViewportResolution;

            DrawShaderPass(
                GraphBuilder,
                PassName,
                PassUpParameters,
                VertexShader,
                PixelShaderUp,
                ClearBlendState,
                Viewports[i]
            );
        }

        PreviousBuffer = Buffer;
    }

    return PreviousBuffer;
}

FRDGTextureRef UPostProcessSubsystem::RenderThreshold(
    FRDGBuilder& GraphBuilder,
    FRDGTextureRef InputTexture,
    FIntRect& InputRect,
    const FViewInfo& View
)
{
    // TODO_THRESHOLD
    RDG_EVENT_SCOPE(GraphBuilder, "ThresholdPass");
    FRDGTextureRef OutputTexture = nullptr;

    FIntRect Viewport = View.ViewRect;
    FIntRect Viewport2 = FIntRect(0, 0,
        View.ViewRect.Width() / 2,
        View.ViewRect.Height() / 2
    );
    FIntRect Viewport4 = FIntRect(0, 0,
        View.ViewRect.Width() / 4,
        View.ViewRect.Height() / 4
    );

    {
        const FString PassName("LensFlareDownsample");

        // Build texture
        FRDGTextureDesc Description = InputTexture->Desc;
        Description.Reset();
        Description.Extent = Viewport4.Size();
        Description.Format = PF_FloatRGB;
        Description.ClearValue = FClearValueBinding(FLinearColor::Black);
        FRDGTextureRef Texture = GraphBuilder.CreateTexture(Description, *PassName);

        // Render shader
        TShaderMapRef<FCustomScreenPassVS> VertexShader(View.ShaderMap);
        TShaderMapRef<FDownsamplePS> PixelShader(View.ShaderMap);

        FDownsamplePS::FParameters* PassParameters = GraphBuilder.AllocParameters<FDownsamplePS::FParameters>();
        PassParameters->Pass.InputTexture = InputTexture;
        PassParameters->Pass.RenderTargets[0] = FRenderTargetBinding(Texture, ERenderTargetLoadAction::ENoAction);
        PassParameters->InputSampler = BilinearClampSampler;
        PassParameters->InputSize = FVector2D(Viewport2.Size());
        PassParameters->ThresholdLevel = PostProcessAsset->ThresholdLevel;
        PassParameters->ThresholdRange = PostProcessAsset->ThresholdRange;

        DrawShaderPass(
            GraphBuilder,
            PassName,
            PassParameters,
            VertexShader,
            PixelShader,
            ClearBlendState,
            Viewport4
        );

        OutputTexture = Texture;
    }

    // TODO_THRESHOLD_BLUR
    {
        OutputTexture = RenderBlur(
            GraphBuilder,
            OutputTexture,
            View,
            Viewport2,
            1
        );
    }
    return OutputTexture;
}

FRDGTextureRef UPostProcessSubsystem::RenderFlare(
    FRDGBuilder& GraphBuilder,
    FRDGTextureRef InputTexture,
    FIntRect& InputRect,
    const FViewInfo& View
)
{
    // TODO_FLARE_CHROMA
    RDG_EVENT_SCOPE(GraphBuilder, "FlarePass");

    FRDGTextureRef OutputTexture = nullptr;

    FIntRect Viewport = View.ViewRect;
    FIntRect Viewport2 = FIntRect(0, 0,
        View.ViewRect.Width() / 2,
        View.ViewRect.Height() / 2
    );
    FIntRect Viewport4 = FIntRect(0, 0,
        View.ViewRect.Width() / 4,
        View.ViewRect.Height() / 4
    );

    FRDGTextureRef ChromaTexture = nullptr;

    {
        const FString PassName("LensFlareChromaGhost");

        // Build buffer
        FRDGTextureDesc Description = InputTexture->Desc;
        Description.Reset();
        Description.Extent = Viewport2.Size();
        Description.Format = PF_FloatRGB;
        Description.ClearValue = FClearValueBinding(FLinearColor::Black);
        ChromaTexture = GraphBuilder.CreateTexture(Description, *PassName);

        // Shader parameters
        TShaderMapRef<FCustomScreenPassVS> VertexShader(View.ShaderMap);
        TShaderMapRef<FLensFlareChromaPS> PixelShader(View.ShaderMap);

        FLensFlareChromaPS::FParameters* PassParameters = GraphBuilder.AllocParameters<FLensFlareChromaPS::FParameters>();
        PassParameters->Pass.InputTexture = InputTexture;
        PassParameters->Pass.RenderTargets[0] = FRenderTargetBinding(ChromaTexture, ERenderTargetLoadAction::ENoAction);
        PassParameters->InputSampler = BilinearBorderSampler;
        PassParameters->ChromaShift = PostProcessAsset->GhostChromaShift;

        // Render
        DrawShaderPass(
            GraphBuilder,
            PassName,
            PassParameters,
            VertexShader,
            PixelShader,
            ClearBlendState,
            Viewport2
        );
    }

    // TODO_FLARE_GHOSTS
    {
        const FString PassName("LensFlareGhosts");

        // Build buffer
        FRDGTextureDesc Description = InputTexture->Desc;
        Description.Reset();
        Description.Extent = Viewport2.Size();
        Description.Format = PF_FloatRGB;
        Description.ClearValue = FClearValueBinding(FLinearColor::Transparent);
        FRDGTextureRef Texture = GraphBuilder.CreateTexture(Description, *PassName);

        // Shader parameters
        TShaderMapRef<FCustomScreenPassVS> VertexShader(View.ShaderMap);
        TShaderMapRef<FLensFlareGhostsPS> PixelShader(View.ShaderMap);

        FLensFlareGhostsPS::FParameters* PassParameters = GraphBuilder.AllocParameters<FLensFlareGhostsPS::FParameters>();
        PassParameters->Pass.InputTexture = ChromaTexture;
        PassParameters->Pass.RenderTargets[0] = FRenderTargetBinding(Texture, ERenderTargetLoadAction::ENoAction);
        PassParameters->InputSampler = BilinearBorderSampler;
        PassParameters->Intensity = PostProcessAsset->GhostIntensity;

        PassParameters->GhostColors[0] = PostProcessAsset->Ghost1.Color;
        PassParameters->GhostColors[1] = PostProcessAsset->Ghost2.Color;
        PassParameters->GhostColors[2] = PostProcessAsset->Ghost3.Color;
        PassParameters->GhostColors[3] = PostProcessAsset->Ghost4.Color;
        PassParameters->GhostColors[4] = PostProcessAsset->Ghost5.Color;
        PassParameters->GhostColors[5] = PostProcessAsset->Ghost6.Color;
        PassParameters->GhostColors[6] = PostProcessAsset->Ghost7.Color;
        PassParameters->GhostColors[7] = PostProcessAsset->Ghost8.Color;

        PassParameters->GhostScales[0] = PostProcessAsset->Ghost1.Scale;
        PassParameters->GhostScales[1] = PostProcessAsset->Ghost2.Scale;
        PassParameters->GhostScales[2] = PostProcessAsset->Ghost3.Scale;
        PassParameters->GhostScales[3] = PostProcessAsset->Ghost4.Scale;
        PassParameters->GhostScales[4] = PostProcessAsset->Ghost5.Scale;
        PassParameters->GhostScales[5] = PostProcessAsset->Ghost6.Scale;
        PassParameters->GhostScales[6] = PostProcessAsset->Ghost7.Scale;
        PassParameters->GhostScales[7] = PostProcessAsset->Ghost8.Scale;

        // Render
        DrawShaderPass(
            GraphBuilder,
            PassName,
            PassParameters,
            VertexShader,
            PixelShader,
            ClearBlendState,
            Viewport2
        );

        OutputTexture = Texture;
    }

    {
        // Render shader
        const FString PassName("LensFlareHalo");

        TShaderMapRef<FCustomScreenPassVS> VertexShader(View.ShaderMap);
        TShaderMapRef<FLensFlareHaloPS> PixelShader(View.ShaderMap);

        FLensFlareHaloPS::FParameters* PassParameters = GraphBuilder.AllocParameters<FLensFlareHaloPS::FParameters>();
        PassParameters->Pass.InputTexture = InputTexture;
        PassParameters->Pass.RenderTargets[0] = FRenderTargetBinding(OutputTexture, ERenderTargetLoadAction::ELoad);
        PassParameters->InputSampler = BilinearBorderSampler;
        PassParameters->Intensity = PostProcessAsset->HaloIntensity;
        PassParameters->Width = PostProcessAsset->HaloWidth;
        PassParameters->Mask = PostProcessAsset->HaloMask;
        PassParameters->Compression = PostProcessAsset->HaloCompression;
        PassParameters->ChromaShift = PostProcessAsset->HaloChromaShift;

        DrawShaderPass(
            GraphBuilder,
            PassName,
            PassParameters,
            VertexShader,
            PixelShader,
            AdditiveBlendState,
            Viewport2
        );
    }

    {
        OutputTexture = RenderBlur(
            GraphBuilder,
            OutputTexture,
            View,
            Viewport2,
            1
        );
    }

    return OutputTexture;
}

FRDGTextureRef UPostProcessSubsystem::RenderGlare(
    FRDGBuilder& GraphBuilder,
    FRDGTextureRef InputTexture,
    FIntRect& InputRect,
    const FViewInfo& View
)
{
    RDG_EVENT_SCOPE(GraphBuilder, "GlarePass");

    FRDGTextureRef OutputTexture = nullptr;

    FIntRect Viewport4 = FIntRect(
        0,
        0,
        View.ViewRect.Width() / 4,
        View.ViewRect.Height() / 4
    );

    // Only render the Glare if its intensity is different from 0
    if (PostProcessAsset->GlareIntensity > SMALL_NUMBER)
    {
        const FString PassName("LensFlareGlare");

        // This compute the number of point that will be drawn
        // Since we want one point for 2 by 2 pixel block we just 
        // need to divide the resolution by two to get this value.
        FIntPoint TileCount = Viewport4.Size();
        TileCount.X = TileCount.X / 2;
        TileCount.Y = TileCount.Y / 2;
        int32 Amount = TileCount.X * TileCount.Y;

        // Compute the ratio between the width and height
        // to know how to adjust the scaling of the quads.
        // (This assume width is bigger than height.)
        FVector2D BufferRatio = FVector2D(
            float(Viewport4.Height()) / float(Viewport4.Width()),
            1.0f
        );

        // Build the buffer
        FRDGTextureDesc Description = InputTexture->Desc;
        Description.Reset();
        Description.Extent = Viewport4.Size();
        Description.Format = PF_FloatRGB;
        Description.ClearValue = FClearValueBinding(FLinearColor::Transparent);
        FRDGTextureRef GlareTexture = GraphBuilder.CreateTexture(Description, *PassName);

        // Setup a few other variables that will 
        // be needed by the shaders.
        FVector4 PixelSize = FVector4(0, 0, 0, 0);
        PixelSize.X = 1.0f / float(Viewport4.Width());
        PixelSize.Y = 1.0f / float(Viewport4.Height());
        PixelSize.Z = PixelSize.X;
        PixelSize.W = PixelSize.Y * -1.0f;

        FVector2D BufferSize = FVector2D(Description.Extent);

        // Setup shader
        FCustomLensFlarePassParameters* PassParameters = GraphBuilder.AllocParameters<FCustomLensFlarePassParameters>();
        PassParameters->InputTexture = InputTexture;
        PassParameters->RenderTargets[0] = FRenderTargetBinding(GlareTexture, ERenderTargetLoadAction::EClear);

        // Vertex shader
        FLensFlareGlareVS::FParameters VertexParameters;
        VertexParameters.Pass = *PassParameters;
        VertexParameters.InputSampler = BilinearBorderSampler;
        VertexParameters.TileCount = TileCount;
        VertexParameters.PixelSize = PixelSize;
        VertexParameters.BufferSize = BufferSize;

        // Geometry shader
        FLensFlareGlareGS::FParameters GeometryParameters;
        GeometryParameters.BufferSize = BufferSize;
        GeometryParameters.BufferRatio = BufferRatio;
        GeometryParameters.PixelSize = PixelSize;
        GeometryParameters.GlareIntensity = PostProcessAsset->GlareIntensity;
        GeometryParameters.GlareTint = FVector4(PostProcessAsset->GlareTint);
        GeometryParameters.GlareScales[0] = PostProcessAsset->GlareScale.X;
        GeometryParameters.GlareScales[1] = PostProcessAsset->GlareScale.Y;
        GeometryParameters.GlareScales[2] = PostProcessAsset->GlareScale.Z;
        GeometryParameters.GlareDivider = FMath::Max(PostProcessAsset->GlareDivider, 0.01f);

        // Pixel shader
        FLensFlareGlarePS::FParameters PixelParameters;
        PixelParameters.GlareSampler = BilinearClampSampler;
        PixelParameters.GlareTexture = GWhiteTexture->TextureRHI;

        if (PostProcessAsset->GlareLineMask != nullptr)
        {
            const FTextureRHIRef TextureRHI = PostProcessAsset->GlareLineMask->Resource->TextureRHI;
            PixelParameters.GlareTexture = TextureRHI;
        }

        TShaderMapRef<FLensFlareGlareVS> VertexShader(View.ShaderMap);
        TShaderMapRef<FLensFlareGlareGS> GeometryShader(View.ShaderMap);
        TShaderMapRef<FLensFlareGlarePS> PixelShader(View.ShaderMap);

        // Required for Lambda capture
        FRHIBlendState* BlendState = this->AdditiveBlendState;

        GraphBuilder.AddPass(
            RDG_EVENT_NAME("%s", *PassName),
            PassParameters,
            ERDGPassFlags::Raster,
            [
                VertexShader, VertexParameters,
                GeometryShader, GeometryParameters,
                PixelShader, PixelParameters,
                BlendState, Viewport4, Amount
            ] (FRHICommandListImmediate& RHICmdList)
            {
                RHICmdList.SetViewport(
                    Viewport4.Min.X, Viewport4.Min.Y, 0.0f,
                    Viewport4.Max.X, Viewport4.Max.Y, 1.0f
                );

                FGraphicsPipelineStateInitializer GraphicsPSOInit;
                RHICmdList.ApplyCachedRenderTargets(GraphicsPSOInit);
                GraphicsPSOInit.BlendState = BlendState;
                GraphicsPSOInit.RasterizerState = TStaticRasterizerState<>::GetRHI();
                GraphicsPSOInit.DepthStencilState = TStaticDepthStencilState<false, CF_Always>::GetRHI();
                GraphicsPSOInit.BoundShaderState.VertexDeclarationRHI = GEmptyVertexDeclaration.VertexDeclarationRHI;
                GraphicsPSOInit.BoundShaderState.VertexShaderRHI = VertexShader.GetVertexShader();
                GraphicsPSOInit.BoundShaderState.GeometryShaderRHI = GeometryShader.GetGeometryShader();
                GraphicsPSOInit.BoundShaderState.PixelShaderRHI = PixelShader.GetPixelShader();
                GraphicsPSOInit.PrimitiveType = PT_PointList;
                SetGraphicsPipelineState(RHICmdList, GraphicsPSOInit);

                SetShaderParameters(RHICmdList, VertexShader, VertexShader.GetVertexShader(), VertexParameters);
                SetShaderParameters(RHICmdList, GeometryShader, GeometryShader.GetGeometryShader(), GeometryParameters);
                SetShaderParameters(RHICmdList, PixelShader, PixelShader.GetPixelShader(), PixelParameters);

                RHICmdList.SetStreamSource(0, nullptr, 0);
                RHICmdList.DrawPrimitive(0, 1, Amount);
            });

        OutputTexture = GlareTexture;

    } // End of if()

    return OutputTexture;

    return nullptr;
}