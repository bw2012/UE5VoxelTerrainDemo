// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BaseObject.h"
#include "../BaseCharacter.h"
#include "SandboxTerrainController.h"
#include "MiningTool.generated.h"

/**
 * 
 */
UCLASS()
class DARKLITHOSPHERE_API AMiningTool : public ABaseObject
{
	GENERATED_BODY()

public:

	AMiningTool();

	UPROPERTY(EditAnywhere, Category = "Sandbox")
	float Strength;

	UPROPERTY(EditAnywhere, Category = "Sandbox")
	USoundCue* DiggingRockSound;

	UPROPERTY(EditAnywhere, Category = "Sandbox")
	USoundCue* DiggingSoilSound;

	void OnAltAction(const FHitResult& Hit, ABaseCharacter* Owner);

	bool OnTracePlayerActionPoint(const FHitResult& Res, ABaseCharacter* Owner);

	void ToggleToolMode();

	int GetSandboxTypeId() const override;

	virtual bool VisibleInHand(FTransform& Transform) override;

private:

	int DiggingToolMode = 0;

	void PlaySound(const FSandboxTerrainMaterial& MatInfo, const FVector& Location, UWorld* World);
};
