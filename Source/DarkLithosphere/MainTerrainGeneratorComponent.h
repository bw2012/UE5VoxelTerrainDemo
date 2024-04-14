// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "TerrainGeneratorComponent.h"
#include <memory>
#include "MainTerrainGeneratorComponent.generated.h"

// не доделано
struct TBiome {

	float ValForestMeadow = 0;

	bool IsForest() const;

};

class TMainChunk : public TChunkData {

public:

	std::shared_ptr<TChunkFloatMatrix> CaveLayer2Top = nullptr;

	std::shared_ptr<TChunkFloatMatrix> CaveLayer2Bottom = nullptr;

	std::shared_ptr<TChunkFloatMatrix> CaveLayer2Pillar = nullptr;

public:

	TMainChunk(int Size);

};


/**
 * 
 */
UCLASS()
class DARKLITHOSPHERE_API UMainTerrainGeneratorComponent : public UTerrainGeneratorComponent {
	GENERATED_BODY()
	
public:

	void BeginPlay() override;

	virtual float DensityFunctionExt(float InDensity, const TFunctionIn& In) const override;

	virtual FSandboxFoliage FoliageExt(const int32 FoliageTypeId, const FSandboxFoliage & FoliageType, const TVoxelIndex & ZoneIndex, const FVector & WorldPos) override;

protected:

	virtual void OnBatchGenerationFinished() override;

	virtual bool IsForcedComplexZone(const TVoxelIndex& ZoneIndex) override;

	virtual void PrepareMetaData() override;

	void GenerateStructures();

	virtual void PostGenerateNewInstanceObjects(const TVoxelIndex& ZoneIndex, const TZoneGenerationType ZoneType, const TVoxelData* Vd, TInstanceMeshTypeMap& ZoneInstanceMeshMap) const override;

	TBiome ClcBiome(const FVector& WorldPos) const;

	virtual TMaterialId MaterialFuncionExt(const TGenerateVdTempItm* GenItm, const TMaterialId MatId, const FVector& WorldPos, const TVoxelIndex VoxelIndex) const override;

	virtual void ExtVdGenerationData(TGenerateVdTempItm& VdGenerationData) override;

	void RegionGenerateStructures(int RegionX, int RegionY);

	float FunctionMakeCaveLayer1(float Density, const FVector& WorldPos) const;

	float FunctionMakeCaveLayer2Density(float InDensity, const TFunctionIn& In) const;

	TMaterialId FunctionMakeCaveLayer2Material(const TGenerateVdTempItm* GenItm, const TMaterialId MatId, const FVector& WorldPos, const TVoxelIndex& VoxelIndex) const;

	float FuncCaveLayer2BottomLevel(const FVector& P) const;

	float FuncCaveLayer2Pillar(const FVector& P) const;

	virtual TChunkDataPtr NewChunkData() override;

	virtual void GenerateChunkDataExt(TChunkDataPtr ChunkData, const TVoxelIndex& Index, int X, int Y, const FVector& WorldPos) const override;

public:

	void GenerateZoneSandboxObject(const TVoxelIndex& Index);

	virtual float GroundLevelFunction(const TVoxelIndex& ZoneIndex, const FVector& WorldPos) const;

};
