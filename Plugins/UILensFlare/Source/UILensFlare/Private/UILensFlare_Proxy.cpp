// Fill out your copyright notice in the Description page of Project Settings.


#include "UILensFlare_Proxy.h"
#include "Curves/CurveLinearColorAtlas.h"
#include "Blueprint/UserWidget.h"
#include "UILensFlare_Flare.h"
#include "Kismet/GameplayStatics.h"
#include "Components/Image.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/CanvasPanelSlot.h"

// Sets default values
AUILensFlare_Proxy::AUILensFlare_Proxy(const FObjectInitializer& ObjectInitializer)
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	ConstructorHelpers::FClassFinder<UUILensFlare_Flare> WBPLensFlareClass(TEXT("/UILensFlare/WBP_UILensFlare_Proxy"));
	if (!ensure(WBPLensFlareClass.Class != nullptr)) return;

	LensFlareWidgetClass = WBPLensFlareClass.Class;

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
		UUILensFlare_Flare* Flare = CreateWidget<UUILensFlare_Flare>(GetWorld(), LensFlareWidgetClass);
		if (!ensure(Flare != nullptr))return;
		if (!ensure(Flare->LensImage != nullptr))return;

		//......
		// Initiate all Material parameters and values ......
		
		auto slot = UWidgetLayoutLibrary::SlotAsCanvasSlot(Flare->LensImage);
		// auto slotSize = slot->GetSize();
		
		// Set Slot size......
		// slot->SetSize(x,y);



		Flare->AddToViewport();
		LensFlareWidgets.Add(Flare);
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
	if (hitAnything) {
		visibilityIntensity = 0;
	}
	
	//Set Visibility Intensity for all Materials

	FVector2D outScreenPos;
	bool validProjection = UGameplayStatics::ProjectWorldToScreen(controller, actorLocation, outScreenPos, false);
	auto viewportScale = UWidgetLayoutLibrary::GetViewportScale(this);
	auto viewportSize = UWidgetLayoutLibrary::GetViewportSize(this)/viewportScale;
	auto screenCenter = viewportSize * 0.5f;
	outScreenPos = outScreenPos/viewportScale;
	auto lightsource2Center = outScreenPos - screenCenter;

	if (!validProjection)
	{
		// If Projection failed, hide all Lens Flare Widget

		return;
	}


	for (auto flare : LensFlareWidgets)
	{
		if (IsValid(flare))
		{
			auto slot = UWidgetLayoutLibrary::SlotAsCanvasSlot(flare->LensImage);
			auto slotSize = slot->GetSize();

			bool lightSource = false;
			if (lightSource) {
				// Position for LightSource Widget
				slotSize = slotSize * 0.5f;	// Slot Center
				outScreenPos = outScreenPos - slotSize;
				flare->SetPositionInViewport(outScreenPos);
			}
			
			//

		}
	}




}


// Called every frame
void AUILensFlare_Proxy::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

