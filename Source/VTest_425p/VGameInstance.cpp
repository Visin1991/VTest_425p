// Fill out your copyright notice in the Description page of Project Settings.


#include "VGameInstance.h"
#include "UObject/ConstructorHelpers.h"
#include "Blueprint/UserWidget.h"

UVGameInstance::UVGameInstance(const FObjectInitializer& ObjectInitializer)
{
	ConstructorHelpers::FClassFinder<UUserWidget> BPMainMenuClass(TEXT("/Game/13_UI/WBP_MainMenu"));
	if (!ensure(BPMainMenuClass.Class != nullptr)) return;

	MainMenuClass = BPMainMenuClass.Class;
}

void UVGameInstance::Init()
{
	UE_LOG(LogTemp,Warning,TEXT("Found class %s"),*MainMenuClass->GetName());
}

void UVGameInstance::LoadMenu()
{
	if (!ensure(MainMenuClass != nullptr))return;

	UUserWidget* Menu = CreateWidget<UUserWidget>(this, MainMenuClass);
	if (!ensure(Menu != nullptr))return;
		
	Menu->AddToViewport();

	APlayerController* PlayerController = GetFirstLocalPlayerController();
	if (!ensure(PlayerController != nullptr)) return;

	FInputModeUIOnly InputModeData;
	InputModeData.SetWidgetToFocus(Menu->TakeWidget());
	InputModeData.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);

	PlayerController->SetInputMode(InputModeData);
	PlayerController->bShowMouseCursor = true;
}

void UVGameInstance::Host()
{

}

void UVGameInstance::Join(const FString& Address)
{

}