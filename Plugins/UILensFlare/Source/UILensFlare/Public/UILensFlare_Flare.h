// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UILensFlare_Flare.generated.h"


USTRUCT(BlueprintType)
struct FUILensType
{
	GENERATED_BODY()

	FUILensType(FVector2D _size = FVector2D(100,100), int32 _step = 0, float _scale = 1.0, float _intensity=1.0) : ImageSize(_size), Step(_step), Scale(_scale), Intensity(_intensity){}

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	UMaterialInstance* LensMaterial;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FVector2D ImageSize;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	int Step;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float Scale;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float Intensity;
};

UCLASS()
class UILENSFLARE_API UUILensFlare_Flare : public UUserWidget
{
	GENERATED_BODY()
	
public:

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	class UImage* LensImage;

	FUILensType lensType;
};
