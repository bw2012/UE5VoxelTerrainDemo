// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "TerrainGeneratorComponent.h"
#include "MainTerrainGeneratorComponent.generated.h"

struct TBiome {

	float ValForestMeadow = 0;

	bool IsForest() const;

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

	virtual FSandboxFoliage FoliageExt(const int32 FoliageTypeId, const FSandboxFoliage & FoliageType, const TVoxelIndex & ZoneIndex, const FVector & WorldPos) override;

protected:

	virtual void OnBatchGenerationFinished() override;

	virtual bool IsForcedComplexZone(const TVoxelIndex& ZoneIndex) override;

	void virtual PrepareMetaData() override;

	void GenerateStructures();

	virtual void PostGenerateNewInstanceObjects(const TVoxelIndex& ZoneIndex, const TZoneGenerationType ZoneType, const TVoxelData* Vd, TInstanceMeshTypeMap& ZoneInstanceMeshMap) const override;

	TBiome ClcBiome(const FVector& WorldPos) const;

	virtual TMaterialId MaterialFuncionExt(const TGenerateVdTempItm* GenItm, const TMaterialId MatId, const FVector& WorldPos) const override;

	virtual void ExtVdGenerationData(TGenerateVdTempItm& VdGenerationData) override;

	void RegionGenerateStructures(int RegionX, int RegionY);

	float FunctionMakeCaveLayer1(float Density, const FVector& WorldPos) const;

	float FunctionMakeCaveLayer2Density(float Density, const FVector& WorldPos) const;

	TMaterialId FunctionMakeCaveLayer2Material(const TMaterialId MatId, const FVector& WorldPos) const;

	float FuncCaveLayer2BottomLevel(const FVector& P) const;

	float FuncCaveLayer2Pillar(const FVector& P) const;

public:

	float FunctionMakeBox(const float InDensity, const FVector& P, const FBox& InBox) const;

	float FunctionMakeVerticalCylinder(const float InDensity, const FVector& V, const FVector& Origin, const float Radius, const float Top, const float Bottom, const float NoiseFactor = 1.f) const;

	float FunctionMakeSphere(const float InDensity, const FVector& V, const FVector& Origin, const float Radius, const float NoiseFactor) const;

	TGenerationResult FunctionMakeSolidSphere(const float InDensity, const TMaterialId InMaterialId, const FVector& V, const FVector& Origin, const float Radius, const TMaterialId ShellMaterialId) const;

	void GenerateZoneSandboxObject(const TVoxelIndex& Index);

	virtual float GroundLevelFunction(const TVoxelIndex& ZoneIndex, const FVector& WorldPos) const;

};
