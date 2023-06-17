// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "SandboxObjectMap.h"
#include "DummyPawn.generated.h"

UCLASS()
class DARKLITHOSPHERE_API ADummyPawn : public APawn
{
	GENERATED_BODY()

public:
	ADummyPawn();

public:

	UPROPERTY(EditAnywhere, Category = "DW")
	float MaxZoom;

	UPROPERTY(EditAnywhere, Category = "DW")
	float MinZoom;

	UPROPERTY(EditAnywhere, Category = "DW")
	float ZoomStep;

	UStaticMeshComponent* GetCursorMesh();

protected:
	virtual void BeginPlay() override;

	void MoveForward(float Value);

	void MoveRight(float Value);

	void ZoomIn();

	void ZoomOut();

public:	
	virtual void Tick(float DeltaTime) override;

	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;


public:
	void SetSandboxMode(int Mode);

	void OnMainAction(const FHitResult& CursorHit);

private:
	 
	int SandboxMode = 0;

	float InitialTargetArmLength;


};
