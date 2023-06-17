// Copyright Epic Games, Inc. All Rights Reserved.

#include "DarkLithosphereGameMode.h"
#include "DarkLithosphereCharacter.h"
#include "UObject/ConstructorHelpers.h"
#include "MainPlayerController.h"

ADarkLithosphereGameMode::ADarkLithosphereGameMode() {
	// set default pawn class to our Blueprinted character
	/*
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/Sandbox/Character/BP_MainFemaleCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
	*/

	UE_LOG(LogTemp, Warning, TEXT("ADarkLithosphereGameMode"));
}


void ADarkLithosphereGameMode::PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage) {
	Super::PreLogin(Options, Address, UniqueId, ErrorMessage);
}


void ADarkLithosphereGameMode::Logout(AController* Exiting) {
	Super::Logout(Exiting);

	UE_LOG(LogTemp, Log, TEXT("Logout -> %s"), *Exiting->GetName());
}