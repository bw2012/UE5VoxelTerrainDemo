// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "SandboxTerrainController.h"
#include "LevelController.h"
#include "TerrainController.generated.h"

class ABaseObject;

struct TVdGenBlock;
class UTerrainInstancedStaticMesh;

typedef int(*PCudaGetInfo)();
typedef int(*PCudaGenerateVd)(TVdGenBlock*);


USTRUCT()
struct FSandboxObjectsByZone {
	GENERATED_BODY()

	UPROPERTY()
	TMap<FString, ASandboxObject*> WorldObjectMap;

	UPROPERTY()
	TMap<FString, FSandboxObjectDescriptor> Stash;
};

/**
 * 
 */
UCLASS()
class DARKLITHOSPHERE_API ATerrainController : public ASandboxTerrainController
{
	GENERATED_BODY()
	
	
public:

	UPROPERTY(EditAnywhere, Category = "UnrealSandbox Debug")
	bool bUseCUDAGenerator = false;

	UPROPERTY(EditAnywhere, Category = "UnrealSandbox Toolkit")
	ALevelController* LevelController;

	// TODO remove
	UPROPERTY(EditAnywhere, Category = "UnrealSandbox Toolkit")
	TSubclassOf<AActor> TestActor;
	// test

	UFUNCTION(BlueprintImplementableEvent, meta = (DisplayName = "On Progress Save Voxel Terrain"))
	void OnProgressSaveTerrain(float Progress);

	UFUNCTION(BlueprintImplementableEvent, meta = (DisplayName = "On Finish Save Voxel Terrain"))
	void OnFinishSaveTerrain();

	UFUNCTION(BlueprintImplementableEvent, meta = (DisplayName = "On Start Background Save"))
	void EventStartBackgroundSave();

	UFUNCTION(BlueprintImplementableEvent, meta = (DisplayName = "On Finish Background Save"))
	void EventFinishBackgroundSave();

	UFUNCTION(BlueprintImplementableEvent, meta = (DisplayName = "On Progress Background Save"))
	void EventProgressBackgroundSave(float Progress);

	UFUNCTION(BlueprintImplementableEvent, meta = (DisplayName = "On Finish Load Initial Zones"))
	void OnFinishLoadInitialZones();

	UFUNCTION(BlueprintCallable, Category = "UnrealSandbox")
	void ShutdownAndSaveMap();


public:

	virtual void BeginPlay() override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	
	void MakeFlattenGrassTrace(const FHitResult& Overlap);

	void RegisterSandboxObject(ASandboxObject* SandboxObject);

	void UnRegisterSandboxObject(ASandboxObject* SandboxObject);

	ASandboxObject* SpawnSandboxObject(const int32 ClassId, const FTransform& Transform);

	void DestroySandboxObjectByName(const TVoxelIndex& ZoneIndex, const FString& Name);

	const TMap<TVoxelIndex, FSandboxObjectsByZone>& GetObjectsByZoneMap() const;

	void AddToStash(const FSandboxObjectDescriptor& ObjDesc);

	void SpawnFromStash(const TVoxelIndex& ZoneIndex);

	TArray<FVector> Test(FVector PlayerLocation, float Radius);

protected:

	virtual void BeginPlayServer();

	virtual void BeginPlayClient();

	virtual UTerrainGeneratorComponent* NewTerrainGenerator() override;

	virtual void OnOverlapActorTerrainEdit(const FOverlapResult& OverlapResult, const FVector& Pos) override;

	virtual void OnFinishGenerateNewZone(const TVoxelIndex& Index);

	virtual bool OnZoneSoftUnload(const TVoxelIndex& ZoneIndex) override;

	virtual void OnRestoreZoneSoftUnload(const TVoxelIndex& ZoneIndex) override;

	virtual void OnFinishLoadZone(const TVoxelIndex& Index) override;

	virtual void OnStartBackgroundSaveTerrain() override;

	virtual void OnFinishBackgroundSaveTerrain() override;

	virtual void OnProgressBackgroundSaveTerrain(float Progress) override;

	virtual void OnFinishInitialLoad();

	virtual void OnDestroyInstanceMesh(UTerrainInstancedStaticMesh* InstancedMeshComp, int32 ItemIndex) override;

	virtual void OnFinishRegisterPlayer();

	virtual void GetAnchorObjectsLocation(TArray<FVector>& List) const override;

private:

	TMap<TVoxelIndex, FSandboxObjectsByZone> ObjectsByZoneMap;

	TMap<FString, ABaseObject*> ZoneAnchorsMap;

};
