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
	// Sets default values for this pawn's properties
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
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	/** Called for forwards/backward input */
	void MoveForward(float Value);

	/** Called for side to side input */
	void MoveRight(float Value);

	void ZoomIn();

	void ZoomOut();

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;


public:
	void SetSandboxMode(int Mode);

	void OnMainAction(const FHitResult& CursorHit);

private:
	 
	int SandboxMode = 0;

	float InitialTargetArmLength;


};
