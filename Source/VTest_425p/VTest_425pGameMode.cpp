// Copyright Epic Games, Inc. All Rights Reserved.

#include "VTest_425pGameMode.h"
#include "VTest_425pCharacter.h"
#include "UObject/ConstructorHelpers.h"

AVTest_425pGameMode::AVTest_425pGameMode()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/ThirdPersonCPP/Blueprints/ThirdPersonCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}
