// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SandboxLevelController.h"
#include "Marker.h"
#include "SandboxEnvironment.h"
#include "BaseCharacter.h" // TODO
#include "LevelController.generated.h"

class ATerrainController;


struct FTempCharacterLoadInfo {

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
	ASandboxEnvironment* Environment;

	UPROPERTY(EditAnywhere, Category = "Sandbox")
	ATerrainController* TerrainController;

	UPROPERTY(EditAnywhere, Category = "Sandbox")
	UMarkerMap* MarkerMap;

	UPROPERTY(EditAnywhere, Category = "Sandbox")
	UCharacterMap* CharacterMap;

	UFUNCTION(BlueprintCallable, Category = "Sandbox")
	void SaveMap();

	void LoadMap();

	const TArray<FTempCharacterLoadInfo>& GetTempCharacterList() const;

	const TMap<FString, FTempCharacterLoadInfo>& GetTempCharacterMap() const;

	void SpawnTempCharacterList();

	ACharacter* SpawnCharacterByTypeId(const int TypeId, const FVector& Location, const FRotator& Rotation);

	virtual ASandboxObject* SpawnSandboxObject(const int ClassId, const FTransform& Transform) override;

	virtual bool RemoveSandboxObject(ASandboxObject* Obj) override;

protected:

	void ContainerToJson(const UContainerComponent* Containre, TSharedRef<TJsonWriter<TCHAR>> JsonWriter);

	virtual void SaveLevelJsonExt(TSharedRef<TJsonWriter<TCHAR>> JsonWriter) override;

	virtual void LoadLevelJsonExt(TSharedPtr<FJsonObject> JsonParsed) override;

	virtual void SpawnPreparedObjects(const TArray<FSandboxObjectDescriptor>& ObjDescList) override;

	void PrepareObjectForSave(TArray<FSandboxObjectDescriptor>& ObjDescList) override;

private:

	TArray<FTempCharacterLoadInfo> TempCharacterList;
	
	TMap<FString, FTempCharacterLoadInfo> TempCharacterMap;
};
