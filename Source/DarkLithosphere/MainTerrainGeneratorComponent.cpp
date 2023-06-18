
#include "MainTerrainGeneratorComponent.h"
#include "TerrainController.h"
#include "UnrealSandboxTerrain.h"


void UMainTerrainGeneratorComponent::BeginPlay() {
	Super::BeginPlay();
}

//====================================================================================
// Terrain density and material generator
//====================================================================================

static const float CaveLayerZ = 20000.f; //18000

bool IsCaveLayerZone(int Z) {
	//static constexpr  linux
	const int ZoneCaveLevel = -(CaveLayerZ / 1000);
	return Z <= ZoneCaveLevel + 1 && Z >= ZoneCaveLevel - 1;
}

void UMainTerrainGeneratorComponent::PrepareMetaData() {
	UE_LOG(LogTemp, Log, TEXT("Prepare terrain generator metadata..."));
	GenerateStructures();
}

// TODO not needed anymore? ZoneGenType override
bool UMainTerrainGeneratorComponent::IsForcedComplexZone(const TVoxelIndex& ZoneIndex) {
	if (IsCaveLayerZone(ZoneIndex.Z)) {
		return true;
	}
	
	if (ZoneIndex.X == 0 && ZoneIndex.Y == 0 && ZoneIndex.Z == -1) {
		//return true;
	}

	return Super::IsForcedComplexZone(ZoneIndex);
}

// TODO noise factor
float UMainTerrainGeneratorComponent::FunctionMakeSphere(const float InDensity, const FVector& V, const FVector& Origin, const float Radius, const float NoiseFactor) const {
	static const float E = 50;
	static const float NoisePosScale = 0.007f / 2;
	static const float NoiseScale = 0.18;

	if (InDensity > 0.5f) {
		const FVector P = V - Origin;
		const float R = std::sqrt(P.X * P.X + P.Y * P.Y + P.Z * P.Z);
		if (R < Radius + E) {
			if (R < Radius - E) {
				return 0.f;
			} else {
				const float N = PerlinNoise(P, NoisePosScale, NoiseScale);
				float Density = 1 / (1 + exp((Radius - R) / 100)) + N;
				if (Density < InDensity) {
					return Density;
				}
			}
		}
	}

	return InDensity;
};

TGenerationResult UMainTerrainGeneratorComponent::FunctionMakeSolidSphere(const float InDensity, const TMaterialId InMaterialId, const FVector& V, const FVector& Origin, const float Radius, const TMaterialId ShellMaterialId) const {
	static const float E = 50;
	static const float Thickness = 100;

	const FVector P = V - Origin;
	const float R = std::sqrt(P.X * P.X + P.Y * P.Y + P.Z * P.Z);

	float Density = InDensity;
	TMaterialId MaterialId = InMaterialId;

	if (R < Radius + Thickness && R > Radius - Thickness) {
		MaterialId = ShellMaterialId;
		Density = 1;
	}

	if (R < Radius + E - Thickness) {
		Density = 1 / (1 + exp((Radius - R - Thickness) / 100));
		return std::make_tuple(Density, MaterialId);
	}

	if (R < Radius - E - Thickness) {
		Density = 0;
		return std::make_tuple(Density, MaterialId);
	}

	TVoxelIndex Index = GetController()->GetZoneIndex(V);
	if (R > Radius) {
		if (R > Radius + Thickness + E * 2) {
			Density = InDensity;
		} else {
			const float D = exp((Radius - R) / 100);
			Density = D;
			if (R > Radius + E) {
				if (InDensity > D) {
					Density = InDensity;
				} 
			}
		}
	}

	return std::make_tuple(Density, MaterialId);
};

float UMainTerrainGeneratorComponent::FunctionMakeVerticalCylinder(const float InDensity, const FVector& V, const FVector& Origin, const float Radius, const float Top, const float Bottom, const float NoiseFactor) const {
	static const float E = 50;
	static const float NoisePositionScale = 0.007f;
	const float NoiseValueScale = 0.1 * NoiseFactor;

	if (InDensity > 0.5f) {
		if (V.Z < (Origin.Z + Top + E) && V.Z > (Origin.Z + Bottom - E)) {
			const FVector P = V - Origin;
			const float R = std::sqrt(P.X * P.X + P.Y * P.Y);
			if (R < Radius + E) {
				if (R < Radius - E) {
					return 0.f;
				} else {
					const float N = PerlinNoise(P, NoisePositionScale, NoiseValueScale);
					float Density = 1 / (1 + exp((Radius - R) / 100)) + N;
					if (Density < InDensity) {
						return Density;
					}
				}
			}
		}
	}

	return InDensity;
};

float UMainTerrainGeneratorComponent::FunctionMakeBox(const float InDensity, const FVector& P, const FBox& InBox) const {
	const float ExtendXP = InBox.Max.X;
	const float ExtendYP = InBox.Max.Y;
	const float ExtendZP = InBox.Max.Z;
	const float ExtendXN = InBox.Min.X;
	const float ExtendYN = InBox.Min.Y;
	const float ExtendZN = InBox.Min.Z;

	static const float E = 50;
	FBox Box = InBox.ExpandBy(E);

	static float D = 100;
	static const float NoisePositionScale = 0.005f;
	static const float NoiseValueScale = 0.1;
	float R = InDensity;

	if (InDensity < 0.5f) {
		return InDensity;
	}

	if (FMath::PointBoxIntersection(P, Box)) {
		R = 0;

		if (FMath::Abs(P.X - ExtendXP) < 50 || FMath::Abs(-P.X + ExtendXN) < 50) {
				const float DensityXP = 1 / (1 + exp((ExtendXP - P.X) / D));
				const float DensityXN = 1 / (1 + exp((-ExtendXN + P.X) / D));
				const float DensityX = (DensityXP + DensityXN);
				const float N = PerlinNoise(P, NoisePositionScale, NoiseValueScale);
				R = DensityX + N;
		}

		if (FMath::Abs(P.Y - ExtendYP) < 50 || FMath::Abs(-P.Y + ExtendYN) < 50) {
			if (R < 0.5f) {
				const float DensityYP = 1 / (1 + exp((ExtendYP - P.Y) / D));
				const float DensityYN = 1 / (1 + exp((-ExtendYN + P.Y) / D));
				const float DensityY = (DensityYP + DensityYN);
				const float N = PerlinNoise(P, NoisePositionScale, NoiseValueScale);
				R = DensityY + N;
			}
		}

		if (FMath::Abs(P.Z - ExtendZP) < 50 || FMath::Abs(-P.Z + ExtendZN) < 50) {
			if (R < 0.5f) {
				const float DensityZP = 1 / (1 + exp((ExtendZP - P.Z) / D));
				const float DensityZN = 1 / (1 + exp((-ExtendZN + P.Z) / D));
				const float DensityZ = (DensityZP + DensityZN);
				const float N = PerlinNoise(P, NoisePositionScale, NoiseValueScale / 2);
				R = DensityZ + N;
			}
		}
	}

	if (R > 1) {
		R = 1;
	}

	if (R < 0) {
		R = 0;
	}

	return R;
};

float UMainTerrainGeneratorComponent::FunctionMakeCaveLayer(float Density, const FVector& WorldPos) const {
	if (Density < 0.5) {
		return Density;
	}

	const float BaseCaveLevel = CaveLayerZ;
	float Density1 = 1;

	static const float scale0 = 0.005f; // small
	static const float scale1 = 0.001f; // small

	const FVector v0(WorldPos.X * scale0, WorldPos.Y * scale0, WorldPos.Z * 0.0002); // Stalactite
	const float Noise0 = PerlinNoise(v0);

	// cave height
	static const float scale3 = 0.001f;
	const FVector v3(WorldPos.X * scale3, WorldPos.Y * scale3, 0); // extra cave height
	const float Noise3 = PerlinNoise(v3);
	const float BaseCaveHeight = 800; // 400
	const float ExtraCaveHeight = 490 * Noise3;
	float CaveHeight = BaseCaveHeight + ExtraCaveHeight;
	//float CaveHeight = BaseCaveHeight;
	if (CaveHeight < 0) {
		CaveHeight = 0; // protection if my calculation is failed
	}

	//cave level
	static const float scale4 = 0.00025f;
	const FVector v4(WorldPos.X * scale4, WorldPos.Y * scale4, 10); // extra cave height
	const float Noise4 = PerlinNoise(v4);
	const float ExtraCaveLevel = 1000 * Noise4;
	const float CaveLevel = BaseCaveLevel + ExtraCaveLevel;

	// cave layer function
	const float CaveHeightF = CaveHeight * 160 * 4; // 80000 -> 473
	float CaveLayer = 1 - exp(-pow((WorldPos.Z + CaveLevel), 2) / CaveHeightF); // 80000 -> 473 = 473 * 169.13

	if (WorldPos.Z < -(BaseCaveLevel - 1200) && WorldPos.Z > -(BaseCaveLevel + 1200)) {
		const float CaveLayerN = 1 - CaveLayer;
		Density1 = CaveLayer + CaveLayerN * Noise0 * 0.05;
	}

	if (Density1 < 0) {
		Density1 = 0;
	}

	if (Density1 > 1) {
		Density1 = 1;
	}

	const static float scale300 = 0.00009f; // big
	const static float scale30 = scale300 / 3; 
	const static float scale31 = scale300 * 1.1; 

	const FVector& V = WorldPos;
	const FVector v00(V.X * scale30, V.Y * scale30, 0);
	const float noise_big = PerlinNoise(v00) * 10.f;
	const static float test = 0.8; // ������

	const static float ExtendXP = 1;
	float Density2 = 1 / (1 + exp(0.7 - FMath::Abs(noise_big)));

	if (Density2 > 0.5) {
		const FVector v01(V.X * scale31, V.Y * scale31, -33338 * scale31);
		float Density3 = 1 / (1 + exp(0.7 - FMath::Abs(PerlinNoise(v01) * 10.f)));
		Density2 = Density3;
	}

	if (Density2 < 0) {
		Density2 = 0;
	}

	if (Density2 > 1) {
		Density2 = 1;
	}

	float ttt3 = Density1 + Density2;

	if (ttt3 < 0) {
		ttt3 = 0;
	}

	if (ttt3 > 1) {
		ttt3 = 1;
	}

	return ttt3;
}

float UMainTerrainGeneratorComponent::DensityFunctionExt(float Density, const TVoxelIndex& ZoneIndex, const FVector& WorldPos, const FVector& LocalPos) const {

	if (ZoneIndex.X == 0 && ZoneIndex.Y == 0 && ZoneIndex.Z == -1) {
		float Result = Density;

		FVector P = LocalPos;
		float r = std::sqrt(P.X * P.X + P.Y * P.Y + P.Z * P.Z);
		const float MaxR = 500;
		float t = 1 - exp(-pow(r, 2) / (MaxR * 100));

		//float D = 1 / (1 + exp((r - MaxR) / 10));
		Result *= t;

		//return Result;
	}

	if (IsCaveLayerZone(ZoneIndex.Z)) {
		return FunctionMakeCaveLayer(Density, WorldPos);
	}

	return Density;
}

bool TBiome::IsForest() const {
	return ValForestMeadow > 0.f;
}

TBiome UMainTerrainGeneratorComponent::ClcBiome(const FVector& WorldPos) const {
	static const float NoisePositionScale = 0.00005f;
	static const float NoiseValueScale = 1.f;
	FVector Pos(WorldPos.X, WorldPos.Y, 0);

	TBiome Biome;
	Biome.ValForestMeadow = PerlinNoise(Pos, NoisePositionScale, NoiseValueScale);

	return Biome;
}

bool IsSpawnArea(const FVector& WorldPos) {
	const float R = std::sqrt(WorldPos.X * WorldPos.X + WorldPos.Y * WorldPos.Y + WorldPos.Z * WorldPos.Z);
	if (R < 3500) {
		return true;
	}

	return false;
}

FSandboxFoliage UMainTerrainGeneratorComponent::FoliageExt(const int32 FoliageTypeId, const FSandboxFoliage& FoliageType, const TVoxelIndex& ZoneIndex, const FVector& WorldPos) {
	FSandboxFoliage Foliage = FoliageType;
	TBiome Biome = ClcBiome(WorldPos);

	// tree
	if (Foliage.Type == ESandboxFoliageType::Tree) {
		if (IsSpawnArea(WorldPos)) {
			Foliage.Probability *= 0.01;
			return Foliage;
		}

		if (Biome.IsForest()) {
			//forest

			if (CheckExtZoneParam(ZoneIndex, "wood_logs", "Y")) {
				Foliage.Probability /= 3;
				return Foliage;
			}

			Foliage.Probability *= 30;
			return Foliage;
		}
	}

	// bush
	if (Foliage.Type == ESandboxFoliageType::Bush) {
		// spawn area 
		if (IsSpawnArea(WorldPos)) {
			Foliage.Probability *= 0.01;
			return Foliage;
		}

		if (!Biome.IsForest()) {
			Foliage.Probability /= 5;
			return Foliage;
		} else {
			Foliage.Probability *= 3;
		}
	}

	// grass
	if (Foliage.Type == ESandboxFoliageType::Grass) {
		if (Biome.IsForest()) {
			Foliage.ScaleMaxZ = 0.95;
		}
	}

	if (FoliageTypeId == 2) {
		const float R = std::sqrt(WorldPos.X * WorldPos.X + WorldPos.Y * WorldPos.Y + WorldPos.Z * WorldPos.Z);
		if (R < 500) {
			Foliage.Probability = 0;
		}
	}

	// flowers
	if (Foliage.Type == ESandboxFoliageType::Flower) {
		if (Biome.IsForest()) {
			Foliage.Probability = 0;
		}
	}

	if (Foliage.Type == ESandboxFoliageType::ForestFoliage) {
		if (Biome.IsForest()) {
			if (CheckExtZoneParam(ZoneIndex, "fern", "Y")) {
				if (FoliageTypeId == 3) { //fern
					Foliage.Probability *= 10;
					Foliage.SpawnStep = 25;
				}
			}

			if (FoliageTypeId == 6) { //nettle
				Foliage.Probability = 0;
			}

			if (CheckExtZoneParam(ZoneIndex, "nettle", "Y")) {
				if (FoliageTypeId == 6) { //nettle
					Foliage.Probability = 0.1;
				}
			}

		} else {
			Foliage.Probability = 0;
		}
	}

	return Foliage;
}

FRandomStream UMainTerrainGeneratorComponent::MakeNewRandomStream(const FVector& ZonePos) const {
	int32 Hash = ZoneHash(ZonePos);
	FRandomStream Rnd = FRandomStream();
	Rnd.Initialize(Hash);
	Rnd.Reset();
	return Rnd;
}

FRotator SelectRotation() {
	const auto DirIndex = FMath::RandRange(0, 3);
	static const FRotator Direction[4] = { FRotator(0), FRotator(0, 0, 90), FRotator(0, 0, -90), FRotator(0, 0, 180) };
	const FRotator Rotation = Direction[DirIndex];
	return Rotation;
}


void UMainTerrainGeneratorComponent::GenerateRandomInstMesh(TInstanceMeshTypeMap& ZoneInstanceMeshMap, uint32 MeshTypeId, FRandomStream& Rnd, const TVoxelIndex& ZoneIndex, const TVoxelData* Vd, int Min, int Max) const {
	FVector WorldPos(0);
	FVector Normal(0);

	FVector ZonePos = Vd->getOrigin();

	int Num = (Min != Max) ? Rnd.RandRange(Min, Max) : 1;

	for (int I = 0; I < Num; I++) {
		if (SelectRandomSpawnPoint(Rnd, ZoneIndex, Vd, WorldPos, Normal)) {
			const FVector LocalPos = WorldPos - ZonePos;
			const FVector Scale = FVector(1, 1, 1);
			FRotator Rotation = Normal.Rotation();
			Rotation.Pitch -= 90;

			const FQuat DeltaRotation = FQuat(Normal, Rnd.FRandRange(0.f, 360.f));

			FTransform NewTransform(Rotation, LocalPos, Scale);

			NewTransform.TransformRotation(DeltaRotation);

			const FTerrainInstancedMeshType* MeshType = GetController()->GetInstancedMeshType(MeshTypeId, 0);
			if (MeshType) {
				auto& InstanceMeshContainer = ZoneInstanceMeshMap.FindOrAdd(MeshType->GetMeshTypeCode());
				InstanceMeshContainer.MeshType = *MeshType;
				InstanceMeshContainer.TransformArray.Add(NewTransform);
			}

		}
	}
}

void UMainTerrainGeneratorComponent::PostGenerateNewInstanceObjects(const TVoxelIndex& ZoneIndex, const TZoneGenerationType ZoneType, const TVoxelData* Vd, TInstanceMeshTypeMap& ZoneInstanceMeshMap) const {
	if (Vd->getDensityFillState() != TVoxelDataFillState::MIXED) {
		return;
	}

	ATerrainController* TerrainController = (ATerrainController*)GetController();
	ALevelController* LevelController = TerrainController->LevelController;

	if (!LevelController) {
		return;
	}

	FVector ZonePos = Vd->getOrigin();

	FRandomStream Rnd = MakeNewRandomStream(ZonePos);

	// rocks
	if (ZoneType == TZoneGenerationType::Landscape) {
		float Chance = Rnd.GetFraction();
		if (Chance < 0.2f) {
			static const int RockTypeV[] = {900, 905, 906, 907, 908};
			int I = Rnd.RandRange(0, 4);
			GenerateRandomInstMesh(ZoneInstanceMeshMap, RockTypeV[I], Rnd, ZoneIndex, Vd);
		}

		if (ZoneIndex.X == 0 && ZoneIndex.Y == 0) {
			GenerateRandomInstMesh(ZoneInstanceMeshMap, 900, Rnd, ZoneIndex, Vd);
		}
	}


	// rocks
	if (ZoneType == TZoneGenerationType::Landscape) {
		if (ZoneIndex.X == 0 && ZoneIndex.Y == 0) {
			GenerateRandomInstMesh(ZoneInstanceMeshMap, 909, Rnd, ZoneIndex, Vd, 5);
		}
	}

	// green crystals
	if (ZoneIndex.Z <= -8 && ZoneIndex.Z > -20) {
		FVector Pos(0);
		FVector Normal(0);
		float Chance = Rnd.GetFraction();
		if (Chance < 0.05f) {
			if (SelectRandomSpawnPoint(Rnd, ZoneIndex, Vd, Pos, Normal)) {
				FVector Scale = FVector(1, 1, 1);
				FRotator Rotation = Normal.Rotation();
				Rotation.Pitch -= 90;
				FTransform NewTransform(Rotation, Pos, Scale);
				AsyncTask(ENamedThreads::GameThread, [=]() {
					TerrainController->SpawnSandboxObject(300, NewTransform);
				});
			}
		}
	}

	if (IsCaveLayerZone(ZoneIndex.Z)) {
		const int Min = 10;
		const int Max = 30;
		const int MeshTypeId = 999;

		FVector WorldPos(0);
		FVector Normal(0);

		int Num = (Min != Max) ? Rnd.RandRange(Min, Max) : 1;
		for (int I = 0; I < Num; I++) {
			if (SelectRandomSpawnPoint(Rnd, ZoneIndex, Vd, WorldPos, Normal)) {
				if (Normal.Z > 0.5) {
					const float NoiseScale = 0.0004f; // medium
					const FVector V(WorldPos.X * NoiseScale, WorldPos.Y * NoiseScale, 0); // extra cave height
					const float Noise = PerlinNoise(V);

					if (Noise > 0.2) {
						const float Angle = Rnd.FRandRange(0.f, 360.f);
						const float Chance = Rnd.FRandRange(0.f, 1.f);
						const static float Probability = 0.05;
						if (Chance <= Probability) {
							const float Scale2 = Rnd.FRandRange(1.2f, 1.6f);
							FTransform NewTransform2(FRotator(0, Angle, 0), WorldPos, FVector(Scale2, Scale2, Scale2));
							AsyncTask(ENamedThreads::GameThread, [=]() {
								TerrainController->SpawnSandboxObject(200, NewTransform2);
							});

						} else {
							const float Scale = Rnd.FRandRange(0.5f, 1.f);
							const FVector LocalPos = WorldPos - ZonePos;
							FTransform NewTransform(FRotator(0, Angle, 0), LocalPos, FVector(Scale, Scale, Scale));

							const FTerrainInstancedMeshType* MeshType = GetController()->GetInstancedMeshType(MeshTypeId, 0);
							if (MeshType) {
								auto& InstanceMeshContainer = ZoneInstanceMeshMap.FindOrAdd(MeshType->GetMeshTypeCode());
								InstanceMeshContainer.MeshType = *MeshType;
								InstanceMeshContainer.TransformArray.Add(NewTransform);
							}
						}
					}
				}
			}
		}
	}

	/*
	if (ZoneIndex.X == 0 && ZoneIndex.Y == 0 && ZoneIndex.Z == 0) {
		FVector Origin(0, 0, 0);
		FVector Scale(1, 1, 1);

		float Z = GroundLevelFunction(ZoneIndex, Origin);
		FVector Location(0, 0, Z + 100);

		const auto S = 3;
		for (auto X = -S; X <= S; X++) {
			for (auto Y = -S; Y <= S; Y++) {
				const FVector NewLocation(Location.X + X * 60, Location.Y + Y * 60, Location.Z);

				const float Noise = 2.5f;
				const float NoiseAngleX = FMath::RandRange(-Noise, Noise);
				const float NoiseAngleY = FMath::RandRange(-Noise, Noise);
				const float NoiseAngleZ = FMath::RandRange(-Noise, Noise);

				FRotator Rotation = SelectRotation();
				Rotation += FRotator(NoiseAngleX, NoiseAngleY, NoiseAngleZ);

				FTransform NewTransform(Rotation, NewLocation, Scale);

				const FTerrainInstancedMeshType* MeshType = GetController()->GetInstancedMeshType(901, 0);
				if (MeshType) {
					auto& InstanceMeshContainer = ZoneInstanceMeshMap.FindOrAdd(MeshType->GetMeshTypeCode());
					InstanceMeshContainer.MeshType = *MeshType;
					InstanceMeshContainer.TransformArray.Add(NewTransform);
				}
			}
		}
	}
	*/

	if (CheckExtZoneParam(ZoneIndex, "wood_logs", "Y")) {
		GenerateRandomInstMesh(ZoneInstanceMeshMap, 904, Rnd, ZoneIndex, Vd, 1, 7); // chopped woods
		GenerateRandomInstMesh(ZoneInstanceMeshMap, 902, Rnd, ZoneIndex, Vd, 1, 5); // logs
		GenerateRandomInstMesh(ZoneInstanceMeshMap, 903, Rnd, ZoneIndex, Vd);	// stump
	}

	if (CheckExtZoneParam(ZoneIndex, "blue_crystalls", "Y")) {
		GenerateRandomInstMesh(ZoneInstanceMeshMap, 909, Rnd, ZoneIndex, Vd, 5, 10);
	}

}

void UMainTerrainGeneratorComponent::OnBatchGenerationFinished() {

}

void UMainTerrainGeneratorComponent::GenerateZoneSandboxObject(const TVoxelIndex& ZoneIndex) {
	ATerrainController* TerrainController = (ATerrainController*)GetController();
	ALevelController* LevelController = TerrainController->LevelController;
	if (!LevelController) {
		return;
	}

	if (ZoneIndex.X == 1 && ZoneIndex.Y == 1 && ZoneIndex.Z == 0) {
		FVector Location(1000, -1000, 500);
		FRotator Rotation(0);
		//LevelController->SpawnBaseCharacter(3, Location, Rotation);
	}

	if (ZoneIndex.X == 10 && ZoneIndex.Y == -2 && ZoneIndex.Z == -3) {
		FVector Location(10000, -2000, -3000);
		FRotator Rotation(0);
		//LevelController->SpawnBaseCharacter(3, Location, Rotation);
	}
}


float UMainTerrainGeneratorComponent::GroundLevelFunction(const TVoxelIndex& Index, const FVector& WorldPos) const {
	const TVoxelIndex ZoneIndex(Index.X, Index.Y, 0);
	const FVector& V = WorldPos;

	const float scale1 = 0.001f; // small
	const float scale2 = 0.0004f; // medium
	const float scale3 = 0.00009f; // big
	const float scale4 = scale3 / 3; // huge

	const float MaxR = 10000000 / 2;
	float R = FMath::Sqrt(V.X * V.X + V.Y * V.Y);
	float T = 1 - exp(-pow(R, 2) / (MaxR * 100));

	float noise_small = PerlinNoise(V.X * scale1, V.Y * scale1, 0) * 0.5f; // 0.5
	float noise_medium = PerlinNoise(V.X * scale2, V.Y * scale2, 0) * 5.f;
	float noise_big = PerlinNoise(V.X * scale3, V.Y * scale3, 0) * 10.f;
	float noise_huge = PerlinNoise(V.X * scale4, V.Y * scale4, 0) * 50.f;
	const float gl = noise_small + noise_medium + noise_big + noise_huge * T;

	float Lvl = (gl * 100) + 205.f;

	Lvl = PerformLandscapeZone(ZoneIndex, WorldPos, Lvl);

	return Lvl;
}

void UMainTerrainGeneratorComponent::ExtVdGenerationData(TGenerateVdTempItm& VdGenerationData) {
	const auto& ZoneIndex = VdGenerationData.ZoneIndex;
	const auto& ZoneGenerationType = VdGenerationData.Type;
	const FVector ZonePos = GetController()->GetZonePos(ZoneIndex);

	if (ZoneGenerationType != TZoneGenerationType::Landscape && ZoneGenerationType != TZoneGenerationType::AirOnly) {
		std::shared_ptr<TZoneOreData> ZoneOreData = nullptr;

		if (ZoneIndex.Z < -2) {
			int32 Hash = ZoneHash(ZoneIndex);
			FRandomStream Rnd = FRandomStream();
			Rnd.Initialize(Hash);
			Rnd.Reset();

			static const float NoisePositionScale = 0.00015f;
			static const float NoiseValueScale = 1.f;
			FVector Pos(ZonePos.X, ZonePos.Y, ZonePos.Z);
			float PerlinOreFactor = PerlinNoise(Pos, NoisePositionScale, NoiseValueScale);

			if (PerlinOreFactor > 0.2) {
				float Probability = Rnd.FRandRange(0.f, 1.f);
				if (Probability < 0.1) {
					ZoneOreData = std::make_shared<TZoneOreData>();
					ZoneOreData->ZoneIndex = ZoneIndex;
					ZoneOreData->Origin = GetController()->GetZonePos(ZoneIndex);
					ZoneOreData->MatId = 6;
				}
			}
		}

		if (ZoneIndex.X == 0 && ZoneIndex.Y == 0 && ZoneIndex.Z == -2) {
			ZoneOreData = std::make_shared<TZoneOreData>();
			ZoneOreData->ZoneIndex = ZoneIndex;
			ZoneOreData->Origin = GetController()->GetZonePos(ZoneIndex);
			ZoneOreData->MatId = 6;

			//TZoneGenerationType::Other or FullSolidOneMaterial to FullSolidMultipleMaterials
			//return FullSolidMultipleMaterials;
		}

		if (ZoneOreData) {
			VdGenerationData.OreData = ZoneOreData;
		}
	}

	if (ZoneGenerationType == TZoneGenerationType::Landscape) {
		TBiome Biome = ClcBiome(ZonePos);

		if (Biome.IsForest()) {
			int32 Hash = ZoneHash(ZoneIndex);
			FRandomStream Rnd = FRandomStream();
			Rnd.Initialize(Hash);
			Rnd.Reset();

			float Probability = Rnd.FRandRange(0.f, 1.f);
			if (Probability < 0.025) {
				ZoneExtData.FindOrAdd(ZoneIndex).Add("wood_logs", "Y");


				if (Rnd.FRandRange(0.f, 1.f) < 0.3) {
					ZoneExtData.FindOrAdd(ZoneIndex).Add("blue_crystalls", "Y");
				}
			}

			Probability = Rnd.FRandRange(0.f, 1.f);
			if (Probability < 0.05) {
				ZoneExtData.FindOrAdd(ZoneIndex).Add("fern", "Y");
			} else {
				Probability = Rnd.FRandRange(0.f, 1.f);
				if (Probability < 0.025) {
					ZoneExtData.FindOrAdd(ZoneIndex).Add("nettle", "Y");
				}
			}

		}
	}
}

TMaterialId UMainTerrainGeneratorComponent::MaterialFuncionExt(const TGenerateVdTempItm* GenItm, const TMaterialId MatId, const FVector& WorldPos) const {
	auto ZoneIndex = GenItm->ZoneIndex;

	if (GenItm->OreData != nullptr) {
		const TZoneOreData* ZoneOreData = GenItm->OreData.get();
		FVector Tmp = WorldPos - ZoneOreData->Origin;
		float R = std::sqrt(Tmp.X * Tmp.X + Tmp.Y * Tmp.Y + Tmp.Z * Tmp.Z);
		if (R < 250) {
			return ZoneOreData->MatId;
		}
	}
	
	return MatId;
}
