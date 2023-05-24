
#pragma once

#include "Engine.h"
#include "GameFramework/Actor.h"
#include "SandboxEffect.generated.h"

UCLASS(BlueprintType, Blueprintable)
class UNREALSANDBOXTOOLKIT_API ASandboxEffect : public AActor {
	GENERATED_BODY()
	
public:	
	ASandboxEffect();

protected:

	virtual void BeginPlay() override;

public:

	
};
