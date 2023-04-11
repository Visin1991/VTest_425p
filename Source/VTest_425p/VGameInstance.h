// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "VGameInstance.generated.h"

/**
 * 
 */
UCLASS()
class VTEST_425P_API UVGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	UVGameInstance(const FObjectInitializer& ObjectInitializer);

	virtual void Init();

	UFUNCTION(Exec)
	void LoadMenu();

	UFUNCTION(Exec)
	void Host();

	UFUNCTION(Exec)
	void Join(const FString& Address);

private:
	TSubclassOf<class UUserWidget> MainMenuClass;

};
