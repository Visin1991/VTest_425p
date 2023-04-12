// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MainMenu.generated.h"

/**
 * 
 */
UCLASS()
class VTEST_425P_API UMainMenu : public UUserWidget
{
	GENERATED_BODY()

	virtual void NativeConstruct() override;
	
public:
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	class UImage* Image;
};
