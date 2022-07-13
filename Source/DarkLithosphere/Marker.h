// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Runtime/Engine/Classes/Engine/DataAsset.h"
#include "Marker.generated.h"


class AMarker;

UCLASS(BlueprintType, Blueprintable)
class DARKLITHOSPHERE_API UMarkerMap : public UDataAsset {
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, Category = "DW")
	TMap<int32, TSubclassOf<AMarker>> MarkerTypeMap;
};


UCLASS()
class DARKLITHOSPHERE_API AMarker : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AMarker();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

};
