// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UILensFlare_Flare.generated.h"

/**
 * 
 */
UCLASS()
class UILENSFLARE_API UUILensFlare_Flare : public UUserWidget
{
	GENERATED_BODY()
	
public:

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	class UImage* LensImage;
};
