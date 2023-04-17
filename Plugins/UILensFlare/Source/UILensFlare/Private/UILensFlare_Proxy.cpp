// Fill out your copyright notice in the Description page of Project Settings.


#include "UILensFlare_Proxy.h"
#include "Curves/CurveLinearColorAtlas.h"
#include "Blueprint/UserWidget.h"
#include "UILensFlare_Flare.h"
#include "Kismet/GameplayStatics.h"
#include "Components/Image.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/CanvasPanelSlot.h"
#include "Kismet/KismetMaterialLibrary.h"

#define LightSource2CenterX "lightsource2CenterX"
#define LightSource2CenterY "lightsource2CenterY"

// Sets default values*
AUILensFlare_Proxy::AUILensFlare_Proxy(const FObjectInitializer& ObjectInitializer)
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	ConstructorHelpers::FClassFinder<UUILensFlare_Flare> WBPLensFlareClass(TEXT("/UILensFlare/WBP_UILensFlare_Proxy"));
	if (!ensure(WBPLensFlareClass.Class != nullptr)) return;

	LensFlareWidgetClass = WBPLensFlareClass.Class;
	bAlreadyOutOfScreen = false;

}

// Called when the game starts or when spawned
void AUILensFlare_Proxy::BeginPlay()
{
	Super::BeginPlay();

	if (Curve != nullptr)
	{
		CurveSizeY = Curve->GetSizeY();
	}

	if (LensFlareWidgetClass != nullptr)
	{
		// Create Flare UI
		// For Loop ......
		for (auto lensType : UILensTypes)
		{
			UUILensFlare_Flare* Flare = CreateWidget<UUILensFlare_Flare>(GetWorld(), LensFlareWidgetClass);
			if (!ensure(Flare != nullptr))return;
			if (!ensure(Flare->LensImage != nullptr))return;

			Flare->lensType = lensType;
			auto slot = UWidgetLayoutLibrary::SlotAsCanvasSlot(Flare->LensImage);
			slot->SetSize(lensType.ImageSize);

			Flare->AddToViewport();
			LensFlareWidgets.Add(Flare);

			if (Flare->lensType.LensMaterial)
			{
				{
					Flare->lensType.mi = UKismetMaterialLibrary::CreateDynamicMaterialInstance(this, Flare->lensType.LensMaterial);
					if (Flare->lensType.mi)
					{
						for (auto parm : Flare->lensType.scalarParameters)
						{
							Flare->lensType.mi->SetScalarParameterValue(parm.name, parm.value);
						}
						Flare->LensImage->SetBrushFromMaterial(Flare->lensType.mi);
					}
				}
			}
		}
	}
}

void AUILensFlare_Proxy::CalculateLensFlarePosition()
{
	auto controller = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (controller == nullptr) { return; }

	auto cameraLocation = controller->PlayerCameraManager->GetCameraLocation();

	float visibilityIntensity = 1;
	TArray<AActor*> actor2Ignore;
	FHitResult result;
	auto actorLocation = GetActorLocation();
	bool hitAnything = UKismetSystemLibrary::LineTraceSingle(GetWorld(), actorLocation, cameraLocation, ETraceTypeQuery::TraceTypeQuery1,false, actor2Ignore,EDrawDebugTrace::Type::None, result,true);
	
	//Set Visibility Intensity for all Materials
	FVector2D lightSourceScreenPos;
	bool validProjection = UGameplayStatics::ProjectWorldToScreen(controller, actorLocation, lightSourceScreenPos, false);
	float viewportScale = UWidgetLayoutLibrary::GetViewportScale(this);
	FVector2D viewportSize = UWidgetLayoutLibrary::GetViewportSize(this)/viewportScale;
	auto screenCenter = viewportSize * 0.5f;
	lightSourceScreenPos = lightSourceScreenPos /viewportScale;
	FVector2D lightsource2Center = lightSourceScreenPos - screenCenter;

	FVector2D validHalfSize = (viewportSize + viewportSize * FadeOutOffset) * 0.5f;
	FVector2D asbLightSource2Center = lightsource2Center.GetAbs();
	FVector2D borderSize = validHalfSize - asbLightSource2Center;

	if (borderSize.X < 0 || borderSize.Y < 0 || (!validProjection) || hitAnything) 
	{
		if (!bAlreadyOutOfScreen)
		{
			// UE_LOG(LogTemp, Warning, TEXT("Light Source Out Of Screen ......"));
			for (auto flare : LensFlareWidgets)
			{
				//auto negDoubleSize = flare->lensType.ImageSize * -2.0f;
				//flare->SetPositionInViewport(negDoubleSize, false);
				flare->SetVisibility(ESlateVisibility::Hidden);
			}
			bAlreadyOutOfScreen = true;
		}
		return;	//If Lightsource Out of Screen with fade offset, hide all Lens Flare Widget
	}

	//UE_LOG(LogTemp, Warning, TEXT("[X:%f | Y:%f]"), lightSourceScreenPos.X, lightSourceScreenPos.Y);
	for (auto flare : LensFlareWidgets)
	{
		if (IsValid(flare))
		{
			// Inside the Valid Screen......
			if (bAlreadyOutOfScreen)
			{
				flare->SetVisibility(ESlateVisibility::Visible);
			}

			// auto slot = UWidgetLayoutLibrary::SlotAsCanvasSlot(flare->LensImage);
			// auto slotSize = slot->GetSize();
			auto halfSlotSize = flare->lensType.ImageSize * 0.5f;
			auto flareScreenPos = lightSourceScreenPos - halfSlotSize;
			flareScreenPos -= lightsource2Center * flare->lensType.Scale * flare->lensType.Step;
			flare->SetPositionInViewport(flareScreenPos,false);

			//Widget to Center percent......
			if (flare->lensType.mi)
			{
				FVector2D widget2HalfScreenCenterPercent = (lightsource2Center / viewportSize).GetAbs() * 2;
				//UE_LOG(LogTemp, Warning, TEXT("[X:%f | Y:%f]"), widget2HalfScreenCenterPercent.X, widget2HalfScreenCenterPercent.Y);
				//Set Material Infos......
				flare->lensType.mi->SetScalarParameterValue(LightSource2CenterX, widget2HalfScreenCenterPercent.X);
				flare->lensType.mi->SetScalarParameterValue(LightSource2CenterY, widget2HalfScreenCenterPercent.Y);
				for (auto parameter : flare->lensType.scalarParameters)
				{
					flare->lensType.mi->SetScalarParameterValue(parameter.name, parameter.value);
				}

			}
		}
	}

	bAlreadyOutOfScreen = false;
}

// Called every frame
void AUILensFlare_Proxy::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	CalculateLensFlarePosition();
}

