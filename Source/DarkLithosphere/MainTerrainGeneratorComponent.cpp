
#include "MainTerrainGeneratorComponent.h"
#include "TerrainController.h"
#include "UnrealSandboxTerrain.h"


TMainChunk::TMainChunk(int Size) : TChunkData(Size) {
	CaveLayer2Top = std::make_shared<TChunkFloatMatrix>(Size);
	CaveLayer2Bottom = std::make_shared<TChunkFloatMatrix>(Size);
	CaveLayer2Pillar = std::make_shared<TChunkFloatMatrix>(Size);
}


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
		return true;
	}

	if (IsCaveLayer2Zone(ZoneIndex.Z)) {
		if (ZoneIndex.X == 0 && ZoneIndex.Y == 0) {
			//AsyncTask(ENamedThreads::GameThread, [=, this]() { DrawDebugBox(GetWorld(), GetController()->GetZonePos(ZoneIndex), FVector(USBT_ZONE_SIZE / 2), FColor(255, 255, 255, 0), true); });
		}
		return true;
	}
	
	//test cavern
	if (ZoneIndex.X == 0 && ZoneIndex.Y == 0 && ZoneIndex.Z == -2) {
		return true;
	}

	if (ZoneIndex.X == 0 && ZoneIndex.Y == 0 && ZoneIndex.Z == -20) {
		return true;
	}

	return Super::IsForcedComplexZone(ZoneIndex);
}



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

	float OutDensity = Density1 + Density2;

	if (OutDensity < 0) {
		OutDensity = 0;
	}

	if (OutDensity > 1) {
		OutDensity = 1;
	}

	return OutDensity;
}

#define CLC_CAVE2_BOTTOM(P) (NormalizedPerlinNoise(FVector(P.X, P.Y, 0), 0.0005f, 1000) + NormalizedPerlinNoise(FVector(P.X, P.Y, 0), 0.0005f * 3, 300))
#define CLC_CAVE2_PILLAR_TOP(F) (F * 2000)
#define CLC_CAVE2_PILLAR_BOTTOM(F) (F * 4000)
#define CLC_CAVE2_BOTTOM_LEVEL(F) (F - CaveLayer2Z - 2000 - 300)

void UMainTerrainGeneratorComponent::GenerateChunkDataExt(TChunkDataPtr ChunkData, const TVoxelIndex& Index, int X, int Y, const FVector& WorldPos) const {
	auto ChunkPtr = std::static_pointer_cast<TMainChunk>(ChunkData);

	const float Cavelayer2BottomVal = CLC_CAVE2_BOTTOM(WorldPos);
	const float Cavelayer2TopVal = NormalizedPerlinNoise(FVector(WorldPos.X, WorldPos.Y, 0), 0.00061f, 1000);
	const float Cavelayer2PillarVal = FuncCaveLayer2Pillar(WorldPos);

	ChunkPtr->CaveLayer2Bottom->SetVal(X, Y, Cavelayer2BottomVal);
	ChunkPtr->CaveLayer2Top->SetVal(X, Y, Cavelayer2TopVal);
	ChunkPtr->CaveLayer2Pillar->SetVal(X, Y, Cavelayer2PillarVal);
}

float UMainTerrainGeneratorComponent::FunctionMakeCaveLayer2Density(float InDensity, const TFunctionIn& In) const {
	if (InDensity < 0.5) {
		return InDensity;
	}

	const TVoxelIndex& ZoneIndex = std::get<0>(In);
	const TVoxelIndex& VoxelIndex = std::get<1>(In);
	const FVector& WorldPos = std::get<2>(In);
	const FVector& LocalPos = std::get<3>(In);
	TConstChunkData ChunkData = std::get<4>(In);

	//float R = Density;

	const float BaseLevel = -CaveLayer2Z;

	const FVector P = WorldPos;

	const float ZUp = 2000 + BaseLevel;
	const float ZDown = -2000 + BaseLevel;

	static float D = 100;
	static const float NoisePositionScale = 0.005f * 3;
	static const float NoiseValueScale1 = 0.17;

	const auto ChunkPtr = std::static_pointer_cast<const TMainChunk>(ChunkData);

	const float NZN = ChunkPtr->CaveLayer2Bottom->GetVal(VoxelIndex.X, VoxelIndex.Y);
	const float NZP = ChunkPtr->CaveLayer2Top->GetVal(VoxelIndex.X, VoxelIndex.Y);

	//const float NZN = NormalizedPerlinNoise(FVector(P.X, P.Y, 0), 0.0005f, 1000) + NormalizedPerlinNoise(FVector(P.X, P.Y, 0), 0.0005f * 3, 300);
	//const float NZP = NormalizedPerlinNoise(FVector(P.X, P.Y, 0), 0.00061f, 1000);
	//const float Pillar = FuncCaveLayer2Pillar(P);

	const float Pillar = ChunkPtr->CaveLayer2Pillar->GetVal(VoxelIndex.X, VoxelIndex.Y);

	if (P.Z < ZUp && P.Z > ZDown) {
		const float BottomLevel = CLC_CAVE2_BOTTOM_LEVEL(NZN) + CLC_CAVE2_PILLAR_BOTTOM(Pillar);
		const float TopLevel = NZP - CaveLayer2Z + 2000 - 470 - CLC_CAVE2_PILLAR_TOP(Pillar);

		if (Pillar > 0.561 && P.Z < BottomLevel) {
			// появляются ступеньки около столбов. потом разберусь (может быть)
			// либо полости в столбах
			// переделать на структуры?
			//return InDensity; 
		} 
		
		if (P.Z < BottomLevel - 100) {
			//return InDensity; 
		} 
		
		if (P.Z > TopLevel - 100) {
			//return InDensity;
		} 
		
		float DensityZP = 1 / (1 + exp((ZUp - P.Z - 600 + NZP - CLC_CAVE2_PILLAR_TOP(Pillar)) / D));
		float DensityZN = 1 / (1 + exp((-ZDown + P.Z + 300 - NZN - CLC_CAVE2_PILLAR_BOTTOM(Pillar)) / D));

		if (DensityZP > 1.f) {
			DensityZP = 1.f;
		}

		if (DensityZN > 1.f) {
			DensityZN = 1.f;
		}

		if (DensityZP < 0.f) {
			DensityZP = 0.f;
		}

		if (DensityZN < 0.f) {
			DensityZN = 0.f;
		}

		const float DensityZ = (DensityZP + DensityZN);

		TVoxelIndex I2 = GetController()->GetZoneIndex(WorldPos);
		if (I2.X == -11 && I2.Y == -11 && I2.Z == -39) {
			//FVector VVV(WorldPos.X, WorldPos.Y, BottomLevel);

			if (DensityZ < 0.5) {
				//AsyncTask(ENamedThreads::GameThread, [=, this]() { DrawDebugPoint(GetController()->GetWorld(), WorldPos, 3.f, FColor(255, 255, 255, 0), true); });
			} else {
				//AsyncTask(ENamedThreads::GameThread, [=, this]() { DrawDebugPoint(GetController()->GetWorld(), WorldPos, 3.f, FColor(255, 0, 0, 0), true); });
			}

		}

		return DensityZ;

	}

	return InDensity;
}

FORCEINLINE float UMainTerrainGeneratorComponent::FuncCaveLayer2BottomLevel(const FVector& P) const {
	return CLC_CAVE2_BOTTOM(P) - CaveLayer2Z - 2000 - 300;
}

FORCEINLINE float UMainTerrainGeneratorComponent::FuncCaveLayer2Pillar(const FVector& P) const {
	static const float Scale = 0.00009 / 2;
	const float NoiseBig = NormalizedPerlinNoise(FVector(P.X, P.Y, 0), Scale, 1);
	return (NoiseBig < 0.3) ? 1 - (NoiseBig * 3.333f) : 0;
}

TMaterialId UMainTerrainGeneratorComponent::FunctionMakeCaveLayer2Material(const TGenerateVdTempItm* GenItm, const TMaterialId MatId, const FVector& WorldPos, const TVoxelIndex& VoxelIndex) const {
	TMaterialId R = MatId;

	const auto ChunkPtr = std::static_pointer_cast<const TMainChunk>(GenItm->ChunkData);

	const float NZN = ChunkPtr->CaveLayer2Bottom->GetVal(VoxelIndex.X, VoxelIndex.Y);
	const float NZP = ChunkPtr->CaveLayer2Top->GetVal(VoxelIndex.X, VoxelIndex.Y);
	const float Pillar = ChunkPtr->CaveLayer2Pillar->GetVal(VoxelIndex.X, VoxelIndex.Y);

	const float BottomLevel = CLC_CAVE2_BOTTOM_LEVEL(NZN) + CLC_CAVE2_PILLAR_BOTTOM(Pillar);

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

float UMainTerrainGeneratorComponent::DensityFunctionExt(float InDensity, const TFunctionIn& In) const {

	float R = InDensity;

	const TVoxelIndex& ZoneIndex = std::get<0>(In);
	const TVoxelIndex& VoxelIndex = std::get<1>(In);
	const FVector& WorldPos = std::get<2>(In);
	const FVector& LocalPos = std::get<3>(In);
	TConstChunkData ChunkData = std::get<4>(In);

	// test cavern
	if (ZoneIndex.X == 0 && ZoneIndex.Y == 0 && ZoneIndex.Z == -2) {

		FVector P = LocalPos;
		float r = std::sqrt(P.X * P.X + P.Y * P.Y + P.Z * P.Z);
		const float MaxR = 500;
		float t = 1 - exp(-pow(r, 2) / (MaxR * 100));

		//float D = 1 / (1 + exp((r - MaxR) / 10));
		R *= t;
	}

	// test cavern #2
	if (ZoneIndex.X == 0 && ZoneIndex.Y == 0 && ZoneIndex.Z == -20) {

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
		R = FunctionMakeCaveLayer2Density(R, In);
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
			int Max = 6;
			int Min = 2;

			if (CheckZoneTag(ZoneIndex, "cave", "layer2bottom")) {
				D = 0.2f;
				Max = 3;
				Min = 1;
			} else if (CheckZoneTag(ZoneIndex, "cave", "layer2medium")) {
				D = 0.1f;
				Max = 2;
				Min = 1;
			} else if (CheckZoneTag(ZoneIndex, "cave", "layer2top")) {
				D = 0.f;
			}

			if (D > 0) {
				if (Rnd.GetFraction() < D) {
					static const int RockTypeV[] = { 900, 905, 906, 907, 908, 912, 913 };
					int I = Rnd.RandRange(0, 6);
					GenerateRandomInstMesh(ZoneInstanceMeshMap, RockTypeV[I], Rnd, ZoneIndex, Vd, Min, Max);
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
	const float R = FMath::Sqrt(V.X * V.X + V.Y * V.Y);
	const float T = 1 - exp(-pow(R, 2) / (MaxR * 100));

	const float noise_small = PerlinNoise(V.X * scale1, V.Y * scale1, 0) * 0.5f; // 0.5
	const float noise_medium = PerlinNoise(V.X * scale2, V.Y * scale2, 0) * 5.f;
	const float noise_big = PerlinNoise(V.X * scale3, V.Y * scale3, 0) * 10.f;
	const float noise_huge = PerlinNoise(V.X * scale4, V.Y * scale4, 0) * 50.f;
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

TMaterialId UMainTerrainGeneratorComponent::MaterialFuncionExt(const TGenerateVdTempItm* GenItm, const TMaterialId MatId, const FVector& WorldPos, const TVoxelIndex VoxelIndex) const {
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
		R = FunctionMakeCaveLayer2Material(GenItm, R, WorldPos, VoxelIndex);
	}
	
	return R;
}

TChunkDataPtr UMainTerrainGeneratorComponent::NewChunkData() {
	return TChunkDataPtr(new TMainChunk(ZoneVoxelResolution));
}