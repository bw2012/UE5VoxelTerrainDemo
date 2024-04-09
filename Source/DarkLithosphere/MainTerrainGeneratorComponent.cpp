
#include "MainTerrainGeneratorComponent.h"
#include "TerrainController.h"
#include "UnrealSandboxTerrain.h"


void UMainTerrainGeneratorComponent::BeginPlay() {
	Super::BeginPlay();
}

//====================================================================================
// Terrain density and material generator
//====================================================================================

static const float CaveLayer1Z = 20000.f; //18000

static const float CaveLayer2Z = 40000.f; 


bool IsCaveLayer1Zone(int Z) {
	//static constexpr  linux
	static const int ZoneCave1Level = -(CaveLayer1Z / 1000);
	return Z <= ZoneCave1Level + 1 && Z >= ZoneCave1Level - 1;
}

bool IsCaveLayer2Zone(int Z) {
	static const int H = 2;
	static const int ZoneCave2Level = -(CaveLayer2Z / 1000);
	return Z <= ZoneCave2Level + 2 && Z >= ZoneCave2Level - 2;
}

bool IsCaveLayer2TopZone(int Z) {
	static const int H = 2;
	static const int ZoneCave2Level = -(CaveLayer2Z / 1000);
	return Z == ZoneCave2Level + H || Z == ZoneCave2Level + H - 1;
}

bool IsCaveLayer2BottomZone(int Z) {
	static const int H = 2;
	static const int ZoneCave2Level = -(CaveLayer2Z / 1000);
	return Z == ZoneCave2Level - H || Z == ZoneCave2Level - H + 1;
}

bool IsCaveLayer2MediumZone(int Z) {
	static const int ZoneCave2Level = -(CaveLayer2Z / 1000);
	return Z == ZoneCave2Level;
}

void UMainTerrainGeneratorComponent::PrepareMetaData() {
	UE_LOG(LogTemp, Log, TEXT("Prepare terrain generator metadata..."));
	GenerateStructures();
}

// TODO not needed anymore? ZoneGenType override
bool UMainTerrainGeneratorComponent::IsForcedComplexZone(const TVoxelIndex& ZoneIndex) {
	if (IsCaveLayer1Zone(ZoneIndex.Z)) {
		//return true;
	}

	if (IsCaveLayer2Zone(ZoneIndex.Z)) {
		if (ZoneIndex.X == 0 && ZoneIndex.Y == 0) {
			AsyncTask(ENamedThreads::GameThread, [=, this]() { DrawDebugBox(GetWorld(), GetController()->GetZonePos(ZoneIndex), FVector(USBT_ZONE_SIZE / 2), FColor(255, 255, 255, 0), true); });
		}
		return true;
	}
	
	//test cavern
	if (ZoneIndex.X == 0 && ZoneIndex.Y == 0 && ZoneIndex.Z == -2) {
		return true;
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
	static const float E = 50;

	const float ExtendXP = InBox.Max.X;
	const float ExtendYP = InBox.Max.Y;
	const float ExtendZP = InBox.Max.Z;
	const float ExtendXN = InBox.Min.X;
	const float ExtendYN = InBox.Min.Y;
	const float ExtendZN = InBox.Min.Z;

	const FBox Box = InBox.ExpandBy(E);

	static float D = 100;
	static const float NoisePositionScale = 0.005f;
	static const float NoiseValueScale1 = 0.145;
	static const float NoiseValueScale2 = 0.08;
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
				const float N = PerlinNoise(P, NoisePositionScale, NoiseValueScale1);
				R = DensityX + N;
		}

		if (FMath::Abs(P.Y - ExtendYP) < 50 || FMath::Abs(-P.Y + ExtendYN) < 50) {
			if (R < 0.5f) {
				const float DensityYP = 1 / (1 + exp((ExtendYP - P.Y) / D));
				const float DensityYN = 1 / (1 + exp((-ExtendYN + P.Y) / D));
				const float DensityY = (DensityYP + DensityYN);
				const float N = PerlinNoise(P, NoisePositionScale, NoiseValueScale1);
				R = DensityY + N;
			}
		}

		const float NPZ = NormalizedPerlinNoise(FVector(P.X, P.Y, 0), 0.0005f, 400) + NormalizedPerlinNoise(FVector(P.X, P.Y, 0), 0.0005f * 4, 100);

		if (FMath::Abs(P.Z - ExtendZP) < 300 || FMath::Abs(-P.Z + ExtendZN) < 50) {
			if (R < 0.5f) {
				const float DensityZP = 1 / (1 + exp((ExtendZP - NPZ - P.Z) / D));
				const float DensityZN = 1 / (1 + exp((-ExtendZN + P.Z) / D));
				const float DensityZ = (DensityZP + DensityZN);
				const float N = PerlinNoise(P, NoisePositionScale, NoiseValueScale2);
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

float UMainTerrainGeneratorComponent::FunctionMakeCaveLayer1(float Density, const FVector& WorldPos) const {
	if (Density < 0.5) {
		return Density;
	}

	const float BaseCaveLevel = CaveLayer1Z;
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

#define CLC_CAVE2_BOTTOM(P)  (NormalizedPerlinNoise(FVector(P.X, P.Y, 0), 0.0005f, 1000) + NormalizedPerlinNoise(FVector(P.X, P.Y, 0), 0.0005f * 3, 300))

#define CLC_CAVE2_PILLAR_TOP(F)  (F * 2000)

#define CLC_CAVE2_PILLAR_BOTTOM(F)  (F * 4000)

float UMainTerrainGeneratorComponent::FunctionMakeCaveLayer2Density(float Density, const FVector& WorldPos) const {

	if (Density < 0.5) {
		return Density;
	}

	float R = Density;

	const float BaseLevel = -CaveLayer2Z;

	const FVector P = WorldPos;

	const float ZUp = 2000 + BaseLevel;
	const float ZDown = -2000 + BaseLevel;

	static float D = 100;

	static const float NoisePositionScale = 0.005f * 3;
	static const float NoiseValueScale1 = 0.17;

	const float NZN = NormalizedPerlinNoise(FVector(P.X, P.Y, 0), 0.0005f, 1000) + NormalizedPerlinNoise(FVector(P.X, P.Y, 0), 0.0005f * 3, 300);
	const float NZP = NormalizedPerlinNoise(FVector(P.X, P.Y, 0), 0.00061f, 1000);

	//TVoxelIndex I2 = GetController()->GetZoneIndex(WorldPos);
	//if (I2.X == 0 && I2.Y == 0 && I2.Z == -6) {
		//FVector VVV(WorldPos.X, WorldPos.Y, NZN);
		//AsyncTask(ENamedThreads::GameThread, [=, this]() { DrawDebugPoint(GetController()->GetWorld(), VVV, 3.f, FColor(255, 255, 255, 0), true); });
	//}

	const float Pillar = FuncCaveLayer2Pillar(P);

	//TVoxelIndex I2 = GetController()->GetZoneIndex(WorldPos);
	//if (I2.Z == -41 && I2.X < 2 && I2.X > -2 && I2.Y < 2 && I2.Y > -2) {
		//const float MaxR = 10000000 / 2;
		//float R = FMath::Sqrt(V.X * V.X + V.Y * V.Y);
		//float T = 1 - exp(-pow(R, 2) / (MaxR * 100));

		//float T = FuncCaveLayer2BottomLevel(WorldPos);

		//T = T + (NNN * 1000);
		//FVector VVV(WorldPos.X, WorldPos.Y, T);
		//AsyncTask(ENamedThreads::GameThread, [=, this]() { DrawDebugPoint(GetController()->GetWorld(), VVV, 3.f, FColor(255, 255, 255, 0), true); });
	//}


	if (P.Z < ZUp && P.Z > ZDown) {

		const float BottomLevel = FuncCaveLayer2BottomLevel(WorldPos) + CLC_CAVE2_PILLAR_BOTTOM(FuncCaveLayer2Pillar(WorldPos));

		const float TopLevel = NZP - CaveLayer2Z + 2000 - 470 - CLC_CAVE2_PILLAR_TOP(Pillar);


		TVoxelIndex I2 = GetController()->GetZoneIndex(WorldPos);
		if (I2.X == -12 && I2.Y == -6 && I2.Z == -39) {
			FVector VVV1(WorldPos.X, WorldPos.Y, TopLevel);
			FVector VVV2(WorldPos.X, WorldPos.Y, BottomLevel);
			AsyncTask(ENamedThreads::GameThread, [=, this]() { DrawDebugPoint(GetController()->GetWorld(), VVV1, 3.f, FColor(255, 255, 255, 0), true); });
			AsyncTask(ENamedThreads::GameThread, [=, this]() { DrawDebugPoint(GetController()->GetWorld(), VVV2, 3.f, FColor(255, 255, 255, 0), true); });
		}


		if (Pillar > 0.58) {
			R = Density;
		} if (P.Z < BottomLevel - 100) {
			R = Density;
		} else if (P.Z > TopLevel - 100) {
			R = Density;
		} else {
			//R = 0;

			//if (FMath::Abs(P.Z - ZUp) < 300 || FMath::Abs(-P.Z + ZDown) < 300) {
			float DensityZP = 1 / (1 + exp((ZUp - P.Z - 600 + NZP - CLC_CAVE2_PILLAR_TOP(Pillar)) / D));
			float DensityZN = 1 / (1 + exp((-ZDown + P.Z + 300 - NZN - CLC_CAVE2_PILLAR_BOTTOM(Pillar)) / D));

			if (DensityZN < 0) {
				DensityZN = 0;
			}

			if (DensityZP < 0) {
				DensityZP = 0;
			}

			const float DensityZ = (DensityZP + DensityZN);
			R = DensityZ;

			TVoxelIndex I = GetController()->GetZoneIndex(WorldPos);
			if (I.X == 0 && I.Y == 0) {
				int C = R * 255.f;
				//if (R > 0.5) {
					//AsyncTask(ENamedThreads::GameThread, [=, this]() { DrawDebugPoint(GetController()->GetWorld(), WorldPos, 3.f, FColor(C, C, C, 0), true); });
				//}

			}
			//}
		}


	}

	return R;
}

FORCEINLINE float UMainTerrainGeneratorComponent::FuncCaveLayer2BottomLevel(const FVector& P) const {
	return CLC_CAVE2_BOTTOM(P) - CaveLayer2Z - 2000 - 300;
}

FORCEINLINE float UMainTerrainGeneratorComponent::FuncCaveLayer2Pillar(const FVector& P) const {
	static const float scale = 0.00009 / 2;
	float noise_big = NormalizedPerlinNoise(FVector(P.X, P.Y, 0), scale, 1);
	float ttt = 0;
	if (noise_big < 0.3) {
		ttt = 1 - (noise_big * 3.333f);
	}

	return ttt;
}

TMaterialId UMainTerrainGeneratorComponent::FunctionMakeCaveLayer2Material(const TMaterialId MatId, const FVector& WorldPos) const {
	TMaterialId R = MatId;

	const float BottomLevel = FuncCaveLayer2BottomLevel(WorldPos) + CLC_CAVE2_PILLAR_BOTTOM(FuncCaveLayer2Pillar(WorldPos));

	if (WorldPos.Z < -CaveLayer2Z + 2000 && WorldPos.Z > BottomLevel - 100) {

	//if (WorldPos.Z < BottomLevel + 100 && WorldPos.Z > BottomLevel - 100) {
		R = 10; // hardcoded stone moss

		//TVoxelIndex I2 = GetController()->GetZoneIndex(WorldPos);
		//if (I2.X == 0 && I2.Y == 0) {
			//FVector VVV(WorldPos.X, WorldPos.Y, BottomLevel);
			//AsyncTask(ENamedThreads::GameThread, [=, this]() { DrawDebugPoint(GetController()->GetWorld(), VVV, 3.f, FColor(255, 255, 255, 0), true); });
		//}

	}

	return R;
}

float UMainTerrainGeneratorComponent::DensityFunctionExt(float Density, const TVoxelIndex& ZoneIndex, const FVector& WorldPos, const FVector& LocalPos) const {

	float R = Density;

	// test cavern
	if (ZoneIndex.X == 0 && ZoneIndex.Y == 0 && ZoneIndex.Z == -2) {

		FVector P = LocalPos;
		float r = std::sqrt(P.X * P.X + P.Y * P.Y + P.Z * P.Z);
		const float MaxR = 500;
		float t = 1 - exp(-pow(r, 2) / (MaxR * 100));

		//float D = 1 / (1 + exp((r - MaxR) / 10));
		R *= t;
	}

	if (IsCaveLayer1Zone(ZoneIndex.Z)) {
		R = FunctionMakeCaveLayer1(R, WorldPos);
	}

	if (IsCaveLayer2Zone(ZoneIndex.Z)) {
		R = FunctionMakeCaveLayer2Density(R, WorldPos);
	}

	return R;
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


		if (CheckZoneTag(ZoneIndex, "no_tree", "Y")) {
			Foliage.Probability = 0;
			return Foliage;
		}

		if (IsSpawnArea(WorldPos)) {
			Foliage.Probability *= 0.01;
			return Foliage;
		}

		if (Biome.IsForest()) {
			//forest

			if (CheckZoneTag(ZoneIndex, "wood_logs", "Y")) {
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
			if (CheckZoneTag(ZoneIndex, "fern", "Y")) {
				if (FoliageTypeId == 3) { //fern
					Foliage.Probability *= 10;
					Foliage.SpawnStep = 25;
				}
			}

			if (FoliageTypeId == 6) { //nettle
				Foliage.Probability = 0;
			}

			if (CheckZoneTag(ZoneIndex, "nettle", "Y")) {
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

FRotator SelectRotation() {
	const auto DirIndex = FMath::RandRange(0, 3);
	static const FRotator Direction[4] = { FRotator(0), FRotator(0, 0, 90), FRotator(0, 0, -90), FRotator(0, 0, 180) };
	const FRotator Rotation = Direction[DirIndex];
	return Rotation;
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
			GenerateRandomInstMesh(ZoneInstanceMeshMap, 900, Rnd, ZoneIndex, Vd, 2, 3);
		}
	} else if (ZoneType == TZoneGenerationType::Other) {
		if (ZoneIndex.Z < -2) {
			float D = 0.9f;

			if (CheckZoneTag(ZoneIndex, "cave", "layer2bottom")) {
				D = 0.2f;
			} else if (CheckZoneTag(ZoneIndex, "cave", "layer2medium")) {
				D = 0.1f;
			} else if (CheckZoneTag(ZoneIndex, "cave", "layer2top")) {
				D = 0.f;
			}

			if (D > 0) {
				float Chance = Rnd.GetFraction();
				if (Chance < D) {
					static const int RockTypeV[] = { 900, 905, 906, 907, 908, 912, 913 };
					int I = Rnd.RandRange(0, 6);
					GenerateRandomInstMesh(ZoneInstanceMeshMap, RockTypeV[I], Rnd, ZoneIndex, Vd, 2, 6);
				}
			}
		}
	
	}


	// blue crystals (demo)
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
	} else if (CheckZoneTag(ZoneIndex, "cave", "layer2top")) {
		FVector Pos(0);
		FVector Normal(0);
		float Chance = Rnd.GetFraction();
		if (Chance < 0.2f) {
			if (SelectRandomSpawnPoint(Rnd, ZoneIndex, Vd, Pos, Normal)) {
				FVector Scale = FVector(1, 1, 1);
				FRotator Rotation = Normal.Rotation();
				Rotation.Pitch -= 90;
				FTransform NewTransform(Rotation, Pos, Scale);
				AsyncTask(ENamedThreads::GameThread, [=]() { TerrainController->SpawnSandboxObject(301, NewTransform); });
			}
		}
	}

	if (IsCaveLayer1Zone(ZoneIndex.Z) || CheckZoneTag(ZoneIndex, "cave", "layer2bottom")) {
		const int Min = 20;
		const int Max = 40;
		const int MeshTypeId = 999; // blue mushroom as terrain instance mesh (not foliage)
		//TODO refactor

		FVector WorldPos(0);
		FVector Normal(0);

		int Num = (Min != Max) ? Rnd.RandRange(Min, Max) : 1;
		for (int I = 0; I < Num; I++) {
			if (SelectRandomSpawnPoint(Rnd, ZoneIndex, Vd, WorldPos, Normal)) {
				if (Normal.Z > 0.5) {
					const float NoiseScale = 0.0004f; // medium
					const FVector V(WorldPos.X * NoiseScale, WorldPos.Y * NoiseScale, 0); 
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

	if (CheckZoneTag(ZoneIndex, "cave", "layer2bottom") || CheckZoneTag(ZoneIndex, "cave", "layer2medium")) {
		const int FoliageType1Id = 201; // cave shrub
		const int FoliageType2Id = 202; // cave green mushroom nucleus
		//TODO refactor

		FVector WorldPos(0);
		FVector Normal(0);

		for (int I = 0; I < 400; I++) {
			if (SelectRandomSpawnPoint(Rnd, ZoneIndex, Vd, WorldPos, Normal)) {
				if (Normal.Z > 0.5) {
					const float NoiseScale = 0.0004f; // medium
					const FVector V(WorldPos.X * NoiseScale, WorldPos.Y * NoiseScale, 0);

					const float Angle = Rnd.FRandRange(0.f, 360.f);
					const float Scale = Rnd.FRandRange(0.5f, 1.f);
					const FVector LocalPos = WorldPos - ZonePos;
					FTransform NewTransform(FRotator(0, Angle, 0), LocalPos, FVector(Scale, Scale, Scale));

					int MeshVariantId = Rnd.RandRange(0, 14);
					const auto FoliageType = GetController()->GetFoliageById(FoliageType1Id);

					SpawnFoliageAsInstanceMesh(NewTransform, FoliageType1Id, MeshVariantId, FoliageType, ZoneInstanceMeshMap);
				}
			}
		}

		for (int I = 0; I < 200; I++) {
			if (SelectRandomSpawnPoint(Rnd, ZoneIndex, Vd, WorldPos, Normal)) {
				if (Normal.Z > 0.5) {
					const float NoiseScale = 0.0004f; // medium
					const FVector V(WorldPos.X * NoiseScale, WorldPos.Y * NoiseScale, 0);

					const float Angle = Rnd.FRandRange(0.f, 360.f);
					const float Scale = Rnd.FRandRange(0.5f, 1.f);
					const FVector LocalPos = WorldPos - ZonePos;
					FTransform NewTransform(FRotator(0, Angle, 0), LocalPos, FVector(Scale, Scale, Scale));

					int MeshVariantId = Rnd.RandRange(0, 3);
					const auto FoliageType = GetController()->GetFoliageById(FoliageType2Id);

					SpawnFoliageAsInstanceMesh(NewTransform, FoliageType2Id, MeshVariantId, FoliageType, ZoneInstanceMeshMap);
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

	if (CheckZoneTag(ZoneIndex, "wood_logs", "Y")) {
		GenerateRandomInstMesh(ZoneInstanceMeshMap, 904, Rnd, ZoneIndex, Vd, 1, 7); // chopped woods
		GenerateRandomInstMesh(ZoneInstanceMeshMap, 902, Rnd, ZoneIndex, Vd, 1, 5); // logs
		GenerateRandomInstMesh(ZoneInstanceMeshMap, 903, Rnd, ZoneIndex, Vd);	// stump
	}

	if (CheckZoneTag(ZoneIndex, "blue_crystalls", "Y")) {
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

	// landscape only
	if (CheckZoneTag(ZoneIndex, "zombies", "Y")) { 
		const FVector ZoneOrigin = TerrainController->GetZonePos(ZoneIndex);
		const float Z = GroundLevelFunction(ZoneIndex, ZoneOrigin);
		const FVector Location(ZoneOrigin.X, ZoneOrigin.Y, Z + 80);

		FCharacterLoadInfo Info;
		Info.TypeId = 3;
		Info.Location = Location;
		Info.Rotation = FRotator(0);
		LevelController->SpawnCharacter(Info);
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

	if (ZoneGenerationType == TZoneGenerationType::Other) {
		TZoneOreDataPtr ZoneOreData = nullptr;

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
				//AsyncTask(ENamedThreads::GameThread, [=, this]() { DrawDebugBox(GetWorld(), ZonePos, FVector(USBT_ZONE_SIZE / 2), FColor(255, 255, 255, 0), true); });

				ZoneOreData = std::make_shared<TZoneOreData>();
				ZoneOreData->ZoneIndex = ZoneIndex;
				ZoneOreData->Origin = GetController()->GetZonePos(ZoneIndex);
				//ZoneOreData->MatId = 6;
				ZoneOreData->MeshTypeId = 910;
			}
		}

		//test cavern
		if (ZoneIndex.X == 0 && ZoneIndex.Y == 0 && ZoneIndex.Z == -2) {
			ZoneOreData = std::make_shared<TZoneOreData>();
			ZoneOreData->ZoneIndex = ZoneIndex;
			ZoneOreData->Origin = GetController()->GetZonePos(ZoneIndex);
			ZoneOreData->MeshTypeId = 910;

			//TZoneGenerationType::Other or FullSolidOneMaterial to FullSolidMultipleMaterials
			//return FullSolidMultipleMaterials;
		}

		if (ZoneOreData) {
			VdGenerationData.OreData = ZoneOreData;
		}

		if (IsCaveLayer2MediumZone(ZoneIndex.Z)) {
			SetZoneTag(ZoneIndex, "cave", "layer2medium");
			//if (ZoneIndex.X == 0 && ZoneIndex.Y == 0) { AsyncTask(ENamedThreads::GameThread, [=, this]() { DrawDebugBox(GetWorld(), GetController()->GetZonePos(ZoneIndex), FVector(USBT_ZONE_SIZE / 2), FColor(0, 255, 0, 0), true); });}
		}

		if (IsCaveLayer2BottomZone(ZoneIndex.Z)) {
			SetZoneTag(ZoneIndex, "cave", "layer2bottom");
			//if (ZoneIndex.X == 0 && ZoneIndex.Y == 0) { AsyncTask(ENamedThreads::GameThread, [=, this]() { DrawDebugBox(GetWorld(), GetController()->GetZonePos(ZoneIndex), FVector(USBT_ZONE_SIZE / 2), FColor(0, 255, 0, 0), true); });}
		}

		if (IsCaveLayer2TopZone(ZoneIndex.Z)) {
			SetZoneTag(ZoneIndex, "cave", "layer2top");
			//if (ZoneIndex.X == 0 && ZoneIndex.Y == 0) { AsyncTask(ENamedThreads::GameThread, [=, this]() { DrawDebugBox(GetWorld(), GetController()->GetZonePos(ZoneIndex), FVector(USBT_ZONE_SIZE / 2), FColor(255, 255, 0, 0), true); }); }
		}
	}

	if (ZoneGenerationType == TZoneGenerationType::Landscape) {
		TBiome Biome = ClcBiome(ZonePos);
		int32 Hash = ZoneHash(ZoneIndex);
		FRandomStream Rnd = FRandomStream();
		Rnd.Initialize(Hash);
		Rnd.Reset();

		FRandomStream RndNpc = FRandomStream();
		RndNpc.Initialize(Hash);
		RndNpc.Reset();

		if (!IsSpawnArea(ZonePos)) {
			float Zp = RndNpc.FRandRange(0.f, 1.f);
			const float Zc = 0.0125 * (Biome.IsForest() ? 1 : 0.333f); // zombie spawn chance per zone
			if (Zp < Zc) {
				SetZoneTag(ZoneIndex, "zombies", "Y");
			}
		}

		if (Biome.IsForest()) {
			float Probability = Rnd.FRandRange(0.f, 1.f);
			if (Probability < 0.025) {
				SetZoneTag(ZoneIndex, "wood_logs", "Y");

				if (Rnd.FRandRange(0.f, 1.f) < 0.3) {
					SetZoneTag(ZoneIndex, "blue_crystalls", "Y");
				}
			}

			Probability = Rnd.FRandRange(0.f, 1.f);
			if (Probability < 0.05) {
				SetZoneTag(ZoneIndex, "fern", "Y");
			} else {
				Probability = Rnd.FRandRange(0.f, 1.f);
				if (Probability < 0.025) {
					SetZoneTag(ZoneIndex, "nettle", "Y");
				}
			}

		}
	}
}

TMaterialId UMainTerrainGeneratorComponent::MaterialFuncionExt(const TGenerateVdTempItm* GenItm, const TMaterialId MatId, const FVector& WorldPos) const {
	TMaterialId R = MatId;

	auto ZoneIndex = GenItm->ZoneIndex;
	if (GenItm->OreData != nullptr) {
		const TZoneOreData* ZoneOreData = GenItm->OreData.get();
		if (ZoneOreData->MatId > 0) {

			// TODO: customize
			//FVector Tmp = WorldPos - ZoneOreData->Origin;
			//float R = std::sqrt(Tmp.X * Tmp.X + Tmp.Y * Tmp.Y + Tmp.Z * Tmp.Z);
			//if (R < 250) {
			//	return ZoneOreData->MatId;
			//}
		}
	}

	if (IsCaveLayer2Zone(GenItm->ZoneIndex.Z)) {
		R = FunctionMakeCaveLayer2Material(R, WorldPos);
	}
	
	return R;
}
