// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SandboxLevelController.h"
#include "Marker.h"
#include "EnvironmentController.h"
#include "BaseCharacter.h" // TODO
#include "LevelController.generated.h"

class ATerrainController;


struct FCharacterLoadInfo {

	int TypeId;

	FString SandboxPlayerUid;

	FVector Location;

	FRotator Rotation;

	TArray<FTempContainerStack> Inventory;

	TArray<FTempContainerStack> Equipment;
};


/**
 * 
 */
UCLASS()
class DARKLITHOSPHERE_API ALevelController : public ASandboxLevelController
{
	GENERATED_BODY()


public:

	UPROPERTY(EditAnywhere, Category = "Sandbox")
	AEnvironmentController* Environment;

	UPROPERTY(EditAnywhere, Category = "Sandbox")
	ATerrainController* TerrainController;

	UPROPERTY(EditAnywhere, Category = "Sandbox")
	UMarkerMap* MarkerMap;

	UPROPERTY(EditAnywhere, Category = "Sandbox")
	UCharacterMap* CharacterMap;

	UPROPERTY(EditAnywhere, Category = "Sandbox")
	TSubclassOf <AActor> Waypoint;

	UFUNCTION(BlueprintCallable, Category = "Sandbox")
	void SaveMap();

	void LoadMap();

	const TArray<FCharacterLoadInfo>& GetTempCharacterList() const;

	const TMap<FString, FCharacterLoadInfo>& GetConservedCharacterMap() const;

	void SpawnTempCharacterList();

	ACharacter* SpawnCharacterByTypeId(const int TypeId, const FVector& Location, const FRotator& Rotation);

	ACharacter* SpawnCharacter(const FCharacterLoadInfo& TempCharacterInfo);

	void CharacterConservation(const FCharacterLoadInfo& TempCharacterInfo);

	virtual ASandboxObject* SpawnSandboxObject(const int ClassId, const FTransform& Transform) override;

	virtual bool RemoveSandboxObject(ASandboxObject* Obj) override;

	void SpawnEffect(const int32 EffectId, const FTransform& Transform);

protected:

	virtual void BeginPlay() override;

	void ContainerToJson(const UContainerComponent* Containre, TSharedRef<TJsonWriter<TCHAR>> JsonWriter);

	virtual void SaveLevelJsonExt(TSharedRef<TJsonWriter<TCHAR>> JsonWriter) override;

	virtual void LoadLevelJsonExt(TSharedPtr<FJsonObject> JsonParsed) override;

	virtual void SpawnPreparedObjects(const TArray<FSandboxObjectDescriptor>& ObjDescList) override;

	void PrepareObjectForSave(TArray<FSandboxObjectDescriptor>& ObjDescList) override;

private:

	void LoadCaharacterListJson(FString Name, TSharedPtr<FJsonObject> JsonParsed);

	TArray<FCharacterLoadInfo> TempCharacterList;
	
	TMap<FString, FCharacterLoadInfo> ConservedCharacterMap;
};
