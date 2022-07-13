// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SandboxObject.h"
#include "BaseObject.h"
#include "CubeObject.generated.h"

/**
 * 
 */
UCLASS()
class DARKLITHOSPHERE_API ACubeObject : public ABaseObject {
	GENERATED_BODY()
	
public:

	ACubeObject();

	virtual bool PlaceToWorldClcPosition(const FVector& SourcePos, const FRotator& SourceRotation, const FHitResult& Res, FVector& Location, FRotator& Rotation, bool bFinal) const override;

	virtual void OnPlaceToWorld() override;

private:

	UPROPERTY()
	class USoundBase* Sound;
};
