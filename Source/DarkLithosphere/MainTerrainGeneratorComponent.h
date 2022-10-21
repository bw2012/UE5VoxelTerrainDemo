// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "TerrainGeneratorComponent.h"
#include "MainTerrainGeneratorComponent.generated.h"


struct TLandscapeZoneHandler {
	TVoxelIndex ZoneIndex;

	std::function<float(const float, const TVoxelIndex&, const FVector&)> Function = nullptr;
};

struct TBiome {

	float ValForestMeadow = 0;

	bool IsForest() const;

};


class TMetaStructure {

protected:

	TVoxelIndex OriginIndex;

public:

	virtual TArray<TVoxelIndex> GetRelevantZones(UMainTerrainGeneratorComponent* Generator) const;
	
	virtual void MakeMetaData(UMainTerrainGeneratorComponent* Generator) const;

};


/**
 * 
 */
UCLASS()
class DARKLITHOSPHERE_API UMainTerrainGeneratorComponent : public UTerrainGeneratorComponent {
	GENERATED_BODY()
	
public:

	void BeginPlay() override;

	virtual float DensityFunctionExt(float Density, const TVoxelIndex& ZoneIndex, const FVector& WorldPos, const FVector& LocalPos) const override;

	virtual bool UseCustomFoliage(const TVoxelIndex& ZoneIndex) override;

	virtual FSandboxFoliage FoliageExt(const int32 FoliageTypeId, const FSandboxFoliage & FoliageType, const TVoxelIndex & ZoneIndex, const FVector & WorldPos) override;

	virtual bool SpawnCustomFoliage(const TVoxelIndex& Index, const FVector& WorldPos, int32 FoliageTypeId, FSandboxFoliage FoliageType, FRandomStream& Rnd, FTransform& Transform) override;

	void AddLandscapeStructure(const TLandscapeZoneHandler& Structure);

protected:

	virtual void OnBatchGenerationFinished() override;

	virtual bool IsForcedComplexZone(const TVoxelIndex& ZoneIndex) override;

	void virtual PrepareMetaData() override;

	void GenerateStructures();

	virtual void PostGenerateNewInstanceObjects(const TVoxelIndex& ZoneIndex, const TZoneGenerationType ZoneType, const TVoxelData* Vd, TInstanceMeshTypeMap& ZoneInstanceMeshMap) const override;

	FRandomStream MakeNewRandomStream(const FVector& ZonePos) const;

	TBiome ClcBiome(const FVector& WorldPos) const;

	virtual TMaterialId MaterialFuncionExt(const TGenerateVdTempItm* GenItm, const TMaterialId MatId, const FVector& WorldPos) const override;

	virtual void ExtVdGenerationData(TGenerateVdTempItm& VdGenerationData) override;

	void GenerateRandomInstMesh(TInstanceMeshTypeMap& ZoneInstanceMeshMap, uint32 MeshTypeId, FRandomStream& Rnd, const TVoxelIndex& ZoneIndex, const TVoxelData* Vd, int Min = 1, int Max = 1) const;

	void GenerateRandomInstMeshAsFoliage(TInstanceMeshTypeMap& ZoneInstanceMeshMap, uint32 MeshTypeId, FRandomStream& Rnd, const TVoxelIndex& ZoneIndex, const TVoxelData* Vd, int Min = 1, int Max = 1) const;

public:

	float FunctionMakeBox(const float InDensity, const FVector& P, const FBox& InBox) const;

	float FunctionMakeCaveLayer(float Density, const FVector& WorldPos) const;

	float FunctionMakeVerticalCylinder(const float InDensity, const FVector& V, const FVector& Origin, const float Radius, const float Top, const float Bottom, const float NoiseFactor = 1.f) const;

	float FunctionMakeSphere(const float InDensity, const FVector& V, const FVector& Origin, const float Radius, const float NoiseFactor) const;

	TGenerationResult FunctionMakeSolidSphere(const float InDensity, const TMaterialId InMaterialId, const FVector& V, const FVector& Origin, const float Radius, const TMaterialId ShellMaterialId) const;

	void GenerateZoneSandboxObject(const TVoxelIndex& Index);

	virtual float GroundLevelFunction(const TVoxelIndex& ZoneIndex, const FVector& WorldPos) const;

private:

	std::unordered_map<TVoxelIndex, std::vector<TLandscapeZoneHandler>> LandscapeStructureMap;
};
