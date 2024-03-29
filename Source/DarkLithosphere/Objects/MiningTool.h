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
	UMaterialInstance* CursorMaterial;

	UPROPERTY(EditAnywhere, Category = "Sandbox")
	UStaticMesh* Sphere;

	UPROPERTY(EditAnywhere, Category = "Sandbox")
	UStaticMesh* Cube;

	void OnAltAction(const FHitResult& Hit, ABaseCharacter* Owner);

	bool OnTracePlayerActionPoint(const FHitResult& Res, ABaseCharacter* Owner);

	void ToggleToolMode();

	void SwitchUp();

	void SwitchDown();

	int GetSandboxTypeId() const override;

	virtual bool VisibleInHand(FTransform& Transform) override;

private:

	int DiggingToolMode = 0;

	int DiggingToolSize = 0;

	void SpawnItems(APlayerController* Controller, uint32 MeshTypeId, const FTransform& InstanceTransform);


};
