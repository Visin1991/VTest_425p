// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/EngineSubsystem.h"
#include "PostProcess/PostProcessLensFlares.h" //[zv] custom lens falre
#include "PostProcessSubsystem.generated.h"

DECLARE_MULTICAST_DELEGATE_FourParams(FPP_LensFlares, FRDGBuilder&, const FViewInfo&, const FLensFlareInputs&, FLensFlareOutputsData&);
extern RENDERER_API FPP_LensFlares PP_LensFlares;

class UPostProcessLensFlareAsset;
/**
 * 
 */
UCLASS()
class CUSTOMPOSTPROCESS_API UPostProcessSubsystem : public UEngineSubsystem
{
	GENERATED_BODY()

public:
    // Init function to setup the delegate and load the data asset
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;

    // Used for cleanup
    virtual void Deinitialize() override;

private:
    // The reference to the data asset storing the settings
    UPROPERTY(Transient)
    UPostProcessLensFlareAsset* PostProcessAsset;

    // Called by engine delegate Render Thread
    void RenderLensFlare(
        FRDGBuilder& GraphBuilder,
        const FViewInfo& View,
        const FLensFlareInputs& Inputs,
        FLensFlareOutputsData& Outputs
    );

    // Threshold prender pass
    FRDGTextureRef RenderThreshold(
        FRDGBuilder& GraphBuilder,
        FRDGTextureRef InputTexture,
        FIntRect& InputRect,
        const FViewInfo& View
    );

    // Ghosts + Halo render pass
    FRDGTextureRef RenderFlare(
        FRDGBuilder& GraphBuilder,
        FRDGTextureRef InputTexture,
        FIntRect& InputRect,
        const FViewInfo& View
    );

    // Glare render pass
    FRDGTextureRef RenderGlare(
        FRDGBuilder& GraphBuilder,
        FRDGTextureRef InputTexture,
        FIntRect& InputRect,
        const FViewInfo& View
    );

    // Sub-pass for blurring
    FRDGTextureRef RenderBlur(
        FRDGBuilder& GraphBuilder,
        FRDGTextureRef InputTexture,
        const FViewInfo& View,
        const FIntRect& Viewport,
        int BlurSteps
    );

    // Cached blending and sampling states
    // which are re-used across render passes
    FRHIBlendState* ClearBlendState = nullptr;
    FRHIBlendState* AdditiveBlendState = nullptr;

    FRHISamplerState* BilinearClampSampler = nullptr;
    FRHISamplerState* BilinearBorderSampler = nullptr;
    FRHISamplerState* BilinearRepeatSampler = nullptr;
    FRHISamplerState* NearestRepeatSampler = nullptr;
	
};
