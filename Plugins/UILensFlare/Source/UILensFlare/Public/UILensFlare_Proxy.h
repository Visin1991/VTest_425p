// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "UILensFlare_Flare.h"
#include "UILensFlare_Proxy.generated.h"


UCLASS()
class UILENSFLARE_API AUILensFlare_Proxy : public AActor
{
	GENERATED_BODY()
	
public:	
	AUILensFlare_Proxy(const FObjectInitializer& ObjectInitializer);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Lens)
	TArray<FUILensType> UILensTypes;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Lens)
	UCurveLinearColorAtlas* Curve;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	void CalculateLensFlarePosition();

private:
	int CurveSizeY;

	// FVector2D LastValidProjection;

	TSubclassOf<class UUILensFlare_Flare> LensFlareWidgetClass;

	TArray<UUILensFlare_Flare*> LensFlareWidgets;

};
