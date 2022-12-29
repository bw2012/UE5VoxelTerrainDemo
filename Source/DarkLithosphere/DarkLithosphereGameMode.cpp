// Copyright Epic Games, Inc. All Rights Reserved.

#include "DarkLithosphereGameMode.h"
#include "DarkLithosphereCharacter.h"
#include "UObject/ConstructorHelpers.h"

ADarkLithosphereGameMode::ADarkLithosphereGameMode()
{
	// set default pawn class to our Blueprinted character
	/*
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/Sandbox/Character/BP_MainFemaleCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
	*/
}
