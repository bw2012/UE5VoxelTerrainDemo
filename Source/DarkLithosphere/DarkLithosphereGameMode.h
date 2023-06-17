// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "DarkLithosphereGameMode.generated.h"

UCLASS(minimalapi)
class ADarkLithosphereGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ADarkLithosphereGameMode();

	virtual void PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage) override;

	virtual void Logout(AController* Exiting);

};



