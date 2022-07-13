// Fill out your copyright notice in the Description page of Project Settings.

#include "MainTerrainGeneratorComponent.h"
#include "TerrainController.h"
#include "UnrealSandboxTerrain.h"
//#include "SpawnHelper.h" // TODO


void UMainTerrainGeneratorComponent::BeginPlay() {
	Super::BeginPlay();
	UE_LOG(LogTemp, Warning, TEXT("UCudaTerrainGeneratorComponent::BeginPlay"));
}

//====================================================================================
// Terrain density and material generator
//====================================================================================

static const float CaveLayerZ = 7000.f;

bool IsCaveLayerZone(float Z) {
	static constexpr int const ZoneCaveLevel = -(CaveLayerZ / 1000);
	return Z <= ZoneCaveLevel + 1 && Z >= ZoneCaveLevel - 1;
}

void StructureHotizontalBoxTunnel(UMainTerrainGeneratorComponent* Generator, const FBox TunnelBox) {
	const auto Function = [=](const float InDensity, const TMaterialId InMaterialId, const TVoxelIndex& VoxelIndex, const FVector& LocalPos, const FVector& WorldPos) {
		const float Density = Generator->FunctionMakeBox(InDensity, WorldPos, TunnelBox);
		const TMaterialId MaterialId = InMaterialId;
		return std::make_tuple(Density, MaterialId);
	};

	TZoneStructureHandler Str;
	Str.Function = Function;
	//Str.Box = TunnelBox;

	const TVoxelIndex MinIndex = Generator->GetController()->GetZoneIndex(TunnelBox.Min);
	const TVoxelIndex MaxIndex = Generator->GetController()->GetZoneIndex(TunnelBox.Max);

	for (auto X = MinIndex.X; X <= MaxIndex.X; X++) {
		for (auto Y = MinIndex.Y; Y <= MaxIndex.Y; Y++) {
			Generator->AddZoneStructure(TVoxelIndex(X, Y, MinIndex.Z), Str);

			AsyncTask(ENamedThreads::GameThread, [=]() {
				const FVector Pos0 = Generator->GetController()->GetZonePos(TVoxelIndex(X, Y, MinIndex.Z));
				//DrawDebugBox(Generator->GetWorld(), Pos0, FVector(USBT_ZONE_SIZE / 2), FColor(255, 255, 255, 0), true);
			});
		}
	}
}

void StructureVerticalCylinderTunnel(UMainTerrainGeneratorComponent* Generator, const FVector& Origin, const float Radius, const float Top, const float Bottom) {
	const auto Function = [=](const float InDensity, const TMaterialId InMaterialId, const TVoxelIndex& VoxelIndex, const FVector& LocalPos, const FVector& WorldPos) {
		const float Density = Generator->FunctionMakeVerticalCylinder(InDensity, WorldPos, Origin, 300.f, 2000.f, -3000.f);
		return std::make_tuple(Density, InMaterialId);
	};

	const auto Function2 = [=](const TVoxelIndex& ZoneIndex, const FVector& WorldPos, const FVector& LocalPos) {
		const FVector P = WorldPos - Origin;
		const float R = std::sqrt(P.X * P.X + P.Y * P.Y);
		if (R < Radius) {
			return false;
		}
		return true;
	};

	TZoneStructureHandler Str;
	Str.Function = Function;
	Str.LandscapeFoliageFilter = Function2;
	Str.Pos = Origin;

	FVector Min(Origin);
	Min.Z += Bottom;

	FVector Max(Origin);
	Max.Z += Top;

	const TVoxelIndex MinIndex = Generator->GetController()->GetZoneIndex(Min);
	const TVoxelIndex MaxIndex = Generator->GetController()->GetZoneIndex(Max);

	for (auto Z = MinIndex.Z; Z <= MaxIndex.Z; Z++) {
		Generator->AddZoneStructure(TVoxelIndex(MinIndex.X, MinIndex.Y, Z), Str);
	}
}

void StructureDiagonalCylinderTunnel(UMainTerrainGeneratorComponent* Generator, const FVector& Origin, const float Radius, const float Top, const float Bottom, const int Dir) {
	static const FRotator DirRotation[2] = { FRotator (-45, 0, 0), FRotator(0, 0, -45) };
	static const TVoxelIndex DirIndex[2] = { TVoxelIndex(1, 0, 0), TVoxelIndex(0, -1, 0) };

	const auto Function = [=](const float InDensity, const TMaterialId InMaterialId, const TVoxelIndex& VoxelIndex, const FVector& LocalPos, const FVector& WorldPos) {
		const FRotator& Rotator = DirRotation[Dir];
		FVector Tmp = Rotator.RotateVector(WorldPos - Origin);
		Tmp += Origin;

		const static float Sqrt2 = 1.414213;

		const float Density = Generator->FunctionMakeVerticalCylinder(InDensity, Tmp, Origin, Radius, Top, Bottom * Sqrt2 - 350, 0.66);
		return std::make_tuple(Density, InMaterialId);
	};

	const auto Function2 = [=](const TVoxelIndex& ZoneIndex, const FVector& WorldPos, const FVector& LocalPos) {
		const FRotator& Rotator = DirRotation[Dir];
		FVector Tmp = Rotator.RotateVector(WorldPos - Origin);
		Tmp += Origin;

		static const float E = 0;
		if (Tmp.Z < Top + E && Tmp.Z > Bottom - E) {
			const FVector P = Tmp - Origin;
			const float R = std::sqrt(P.X * P.X + P.Y * P.Y);
			if (R < Radius + E) {
				return false;
			}
		}

		return true;
	};

	TZoneStructureHandler Str;
	Str.Function = Function;
	Str.LandscapeFoliageFilter = Function2;
	Str.Pos = Origin;

	FVector Min(Origin);
	Min.Z += Bottom;

	FVector Max(Origin);
	//Max.Z += Top;

	const TVoxelIndex MinIndex = Generator->GetController()->GetZoneIndex(Min);
	const TVoxelIndex MaxIndex = Generator->GetController()->GetZoneIndex(Max);

	int T = -1;
	for (auto Z = MaxIndex.Z + 1; Z >= MinIndex.Z; Z--) {
		const TVoxelIndex TV = DirIndex[Dir] * T;
		const TVoxelIndex TV1 = (DirIndex[Dir] * T) + DirIndex[Dir];
		const TVoxelIndex TV2 = (DirIndex[Dir] * T) - DirIndex[Dir];

		const TVoxelIndex Index0(MinIndex.X + TV.X, MinIndex.Y + TV.Y, Z + TV.Z);
		const TVoxelIndex Index1(MinIndex.X + TV1.X, MinIndex.Y + TV1.Y, Z + TV1.Z);
		const TVoxelIndex Index2(MinIndex.X + TV2.X, MinIndex.Y + TV2.Y, Z + TV2.Z);

		Generator->AddZoneStructure(Index0, Str);
		Generator->AddZoneStructure(Index1, Str);
		Generator->AddZoneStructure(Index2, Str);

		AsyncTask(ENamedThreads::GameThread, [=]() {
			const FVector Pos0 = Generator->GetController()->GetZonePos(Index0);
			const FVector Pos1 = Generator->GetController()->GetZonePos(Index1);
			const FVector Pos2 = Generator->GetController()->GetZonePos(Index2);

			//DrawDebugBox(Generator->GetWorld(), Pos0, FVector(USBT_ZONE_SIZE / 2), FColor(255, 255, 255, 0), true);
			//DrawDebugBox(Generator->GetWorld(), Pos1, FVector(USBT_ZONE_SIZE / 2), FColor(255, 255, 255, 0), true);
			//DrawDebugBox(Generator->GetWorld(), Pos2, FVector(USBT_ZONE_SIZE / 2), FColor(255, 255, 255, 0), true);
		});

		T++;
	}
}

void MakeLandscapeHill(UMainTerrainGeneratorComponent* Generator, const FVector& Origin, const int TypeIdx = 0) {
	FVector O(Origin.X, Origin.Y, 0);
	TVoxelIndex Index = Generator->GetController()->GetZoneIndex(O);

	struct TSizeType {
		TSizeType(float MaxR_, float H_, int S_) : MaxR(MaxR_), H(H_), S(S_) { };
		float MaxR;
		float H;
		int S;
	};

	const TSizeType SizeType[] = { TSizeType(4800, 6, 2), TSizeType(4800*20, 6*6, 9) };

	const float MaxR = SizeType[TypeIdx].MaxR; // max radius
	const float H = SizeType[TypeIdx].H; // height

	const auto Function = [=](const float InLvl, const TVoxelIndex& VoxelIndex, const FVector& WorldPos) {
		FVector Tmp = WorldPos - O;
		float R = std::sqrt(Tmp.X * Tmp.X + Tmp.Y * Tmp.Y);
		//float t = 1 - exp(-pow(R, 2) / ( MaxR * 100)); // hollow
		float T = exp(-pow(R, 2) / (MaxR * 100)) * H; // hill
		return InLvl + (T * 100);
	};

	const int S = SizeType[TypeIdx].S; //3;

	for (auto X = -S; X <= S; X++) {
		for (auto Y = -S; Y <= S; Y++) {
			TLandscapeZoneHandler LandscapeZoneHandler;
			LandscapeZoneHandler.ZoneIndex = TVoxelIndex(Index.X + X, Index.Y + Y, 0);
			LandscapeZoneHandler.Function = Function;
			Generator->AddLandscapeStructure(LandscapeZoneHandler);

			FVector Pos = Generator->GetController()->GetZonePos(LandscapeZoneHandler.ZoneIndex);
			AsyncTask(ENamedThreads::GameThread, [=]() {
				//DrawDebugBox(Generator->GetWorld(), Pos, FVector(USBT_ZONE_SIZE / 2), FColor(255, 255, 255, 0), true);
			});
		}
	}
}

void MakeDungeon1(UMainTerrainGeneratorComponent* Generator) {
	const int StartX = 4000;
	const int StartY = 0;
	const int StartZ = 0;

	const float Width = 800;
	const float Height = 800;

	const float W = Width / 2;
	const float H = Height / 2;

	const static float Deep = 4000;

	MakeLandscapeHill(Generator, FVector(StartX + 500, StartY, 0));

	const TVoxelIndex Index = Generator->GetController()->GetZoneIndex(FVector(StartX, StartY, StartZ));
	float G = Generator->GroundLevelFunction(Index, FVector(StartX, StartY, 0));

	FVector V(StartX, StartY, G);

	const float D = G - StartZ;

	StructureDiagonalCylinderTunnel(Generator, FVector(StartX, StartY, StartZ), 200.f, D, -Deep, 0);

	const FVector O(Deep + StartX, StartY, -Deep);

	const int ZoneLen = 25;
	const float Len = (ZoneLen * 1000) - 200;

	const FVector Min(O.X + 200, O.Y - W, O.Z - H);
	const FVector Max(O.X + Len, O.Y + W, O.Z + H);
	FBox Box(Min, Max);
	StructureHotizontalBoxTunnel(Generator, Box);

	{
		const float Len2 = (8 * 1000) + 620; 
		const FVector Min2(O.X - W, O.Y - Len2 / 2, O.Z - H);
		const FVector Max2(O.X + W, O.Y + Len2 / 2, O.Z + H);
		FBox Box2(Min2, Max2);

		StructureHotizontalBoxTunnel(Generator, Box2.ShiftBy(FVector(2000.f, 0.f, 0)));
		StructureHotizontalBoxTunnel(Generator, Box2.ShiftBy(FVector(-4000.f, 0.f, 0)));

		const float Len3 = (6 * 1000) - 450;
		const FVector Min3(O.X - Len3 / 2, O.Y - Width / 2, O.Z - Height / 2);
		const FVector Max3(O.X + Len3 / 2, O.Y + Width / 2, O.Z + Height / 2);
		FBox Box3(Min3, Max3);

		StructureHotizontalBoxTunnel(Generator, Box3.ShiftBy(FVector(-1000.f, 4000.f, 0)));
		StructureHotizontalBoxTunnel(Generator, Box3.ShiftBy(FVector(-1000.f, -4000.f, 0)));
	}

	{
		const FVector Min1(O.X - 25000, O.Y - Width / 2, O.Z - Height / 2);
		const FVector Max1(O.X - 10000, O.Y + Width / 2, O.Z + Height / 2);
		FBox Box1(Min1, Max1);

		StructureHotizontalBoxTunnel(Generator, Box1);
	}

	{
		StructureDiagonalCylinderTunnel(Generator, O + FVector(-15000, 0.f, 0), 200.f, 0, -2000, 1);

		const float Len1 = (23 * 1000) - 200; //1400

		const FVector Min2(O.X - Width / 2, O.Y - Width / 2 - 500, O.Z - Height / 2);
		const FVector Max2(O.X + Width / 2, O.Y + Width / 2 + 500, O.Z + Height / 2);
		FBox Box2(Min2, Max2);

		StructureHotizontalBoxTunnel(Generator, Box2.ShiftBy(FVector(-15000, -3000, -2000)));

	}

	const int MaxDungeonLevels = 7;

	//AsyncTask(ENamedThreads::GameThread, [=]() { DrawDebugPoint(Generator->GetWorld(), O, 5.f, FColor(255, 255, 255, 0), true); });


	{
		//StructureDiagonalCylinderTunnel(Generator, O + FVector(2000.f, 0.f, 0), 200.f, O.Z, O.Z - 850, 0);
		const FVector Min1(O.X + 4200, O.Y - Width / 2, O.Z - Height / 2 - 2000);
		const FVector Max1(O.X + Len, O.Y + Width / 2, O.Z + Height / 2 - 2000);
		FBox Box1(Min1, Max1);
		//StructureHotizontalBoxTunnel(Generator, Box1);
	}


	for (int Level = 0; Level < MaxDungeonLevels; ++Level) {
		float ZOffset = -Level * 2000;

		{
			const float Len1 = (23 * 1000) - 200; //1400

			const FVector Min2(O.X - Width / 2, O.Y - Len1 / 2, O.Z - Height / 2);
			const FVector Max2(O.X + Width / 2, O.Y + Len1 / 2, O.Z + Height / 2);
			FBox Box2(Min2, Max2);

			StructureHotizontalBoxTunnel(Generator, Box2.ShiftBy(FVector(2000.f, 0.f, ZOffset)));
			StructureHotizontalBoxTunnel(Generator, Box2.ShiftBy(FVector(10000.f, 0.f, ZOffset)));
			StructureHotizontalBoxTunnel(Generator, Box2.ShiftBy(FVector(20000.f, 0.f, ZOffset)));

			StructureHotizontalBoxTunnel(Generator, Box2.ShiftBy(FVector(-4000.f, 0.f, ZOffset)));
			StructureHotizontalBoxTunnel(Generator, Box2.ShiftBy(FVector(-10000.f, 0.f, ZOffset)));
			StructureHotizontalBoxTunnel(Generator, Box2.ShiftBy(FVector(-25000.f, 0.f, ZOffset)));
		}

		{
			const float Len2 = (50 * 1000) - 200; //1400
			const FVector Min2(O.X - Len2 / 2, O.Y - Width / 2, O.Z - Height / 2);
			const FVector Max2(O.X + Len2 / 2, O.Y + Width / 2, O.Z + Height / 2);
			FBox Box2(Min2, Max2);

			
			if (Level > 0) {
				StructureHotizontalBoxTunnel(Generator, Box2.ShiftBy(FVector(0, 0, ZOffset)));
			}

			if (Level == 1) {

				const FVector Min1(O.X - 25000, O.Y - Width / 2, O.Z - Height / 2);
				const FVector Max1(O.X - 10000, O.Y + Width / 2, O.Z + Height / 2);
				FBox Box1(Min1, Max1);

				StructureHotizontalBoxTunnel(Generator, Box2.ShiftBy(FVector(0, -4000, ZOffset)));
			}
						
			StructureHotizontalBoxTunnel(Generator, Box2.ShiftBy(FVector(0, 10000, ZOffset)));
			StructureHotizontalBoxTunnel(Generator, Box2.ShiftBy(FVector(0, -10000, ZOffset)));
		}

	}
}

void MakeBigHill(UMainTerrainGeneratorComponent* Generator, const FVector& Origin) {
	MakeLandscapeHill(Generator, Origin, 1);

	const float Width = 1000 * 3 - 200;
	const float Height = 800;

	const float W = Width / 2;
	const float H = Height / 2;

	const FVector Min(Origin.X - W, Origin.Y - W, Origin.Z - H);
	const FVector Max(Origin.X + W, Origin.Y + W, Origin.Z + H);
	FBox Box(Min, Max);
	StructureHotizontalBoxTunnel(Generator, Box);

	AsyncTask(ENamedThreads::GameThread, [=]() {
		//DrawDebugBox(GetWorld(), O, FVector(USBT_ZONE_SIZE / 2), FColor(255, 0, 0, 0), true);
	});
}


void UMainTerrainGeneratorComponent::PrepareMetaData() {
	UE_LOG(LogTemp, Warning, TEXT("UMainTerrainGeneratorComponent::PrepareMetaData()"));

	MakeDungeon1(this);

	//FVector O(0, 15000, 0);
	//MakeLandscapeHill(this, O, 1);

	const int32 WorldSeed = 13666;

	FRandomStream Rnd = FRandomStream();
	Rnd.Initialize(WorldSeed);
	Rnd.Reset();

	bool bIsValid;
	int32 X = 0;
	int32 Y = 0;

	const int RegionSize = 100;

	for (int I = 0; I < 20; I++) {
		do {
			X = Rnd.RandRange(-RegionSize, RegionSize);
			Y = Rnd.RandRange(-RegionSize, RegionSize);
			const float R = std::sqrt(X * X + Y * Y);

			bIsValid = R > 15; // ignore spawn area

			UE_LOG(LogTemp, Warning, TEXT("TEST -> %d %d -> %f"), X, Y, R);

			FVector Origin(X * 1000, Y * 1000, 0);
			MakeBigHill(this, Origin);

		} while (!bIsValid);
	}
}

// TODO not needed anymore? ZoneGenType override
bool UMainTerrainGeneratorComponent::IsForcedComplexZone(const TVoxelIndex& ZoneIndex) {
	if (IsCaveLayerZone(ZoneIndex.Z)) {
		//return true;
	}
	
	if (ZoneIndex.X == 0 && ZoneIndex.Y == 0 && ZoneIndex.Z == -1) {
		//return true;
	}

	return Super::IsForcedComplexZone(ZoneIndex);
}

float UMainTerrainGeneratorComponent::FunctionMakeVerticalCylinder(const float InDensity, const FVector& V, const FVector& Origin, const float Radius, const float Top, const float Bottom, const float NoiseFactor) const {
	static const float E = 50;
	static const float NoisePositionScale = 0.007f;
	static const float NoiseValueScale = 0.1 * NoiseFactor;

	if (InDensity > 0.5f) {
		if (V.Z < (Origin.Z + Top + E) && V.Z > (Origin.Z + Bottom - E)) {
			const FVector P = V - Origin;
			const float R = std::sqrt(P.X * P.X + P.Y * P.Y);
			if (R < Radius + E) {
				if (R < Radius - E) {
					return 0.f;
				} else {
					//AsyncTask(ENamedThreads::GameThread, [=]() { DrawDebugPoint(GetWorld(), V, 2.f, FColor(255, 255, 255, 0), true); });

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
	const float BaseCaveLevel = CaveLayerZ;

	float Result = Density;

	static const float scale0 = 0.005f; // small
	static const float scale1 = 0.001f; // small
	static const float scale2 = 0.0003f; // big

	const FVector v0(WorldPos.X * scale0, WorldPos.Y * scale0, WorldPos.Z * 0.0002); // Stalactite
	const FVector v1(WorldPos.X * scale1, WorldPos.Y * scale1, WorldPos.Z * scale1); // just noise
	//const FVector v2(WorldPos.X * scale2, WorldPos.Y *scale2, WorldPos.Z * scale2); // just noise big
	const float Noise0 = PerlinNoise(v0);
	const float Noise1 = PerlinNoise(v1);
	//const float Noise2 = PerlinNoise(v2);
	//const float Noise2 = PerlinNoise(v2);

	//const FVector v4(WorldPos.X * scale3, WorldPos.Y * scale3, 10); // extra cave level
	//const float Noise4 = PerlinNoise(v4);

	//float NormalizedPerlin = (NoiseMedium + 0.87) / 1.73;
	//float Z = WorldPos.Z + NoiseMedium * 100;
	//float DensityByGroundLevel = 1 - (1 / (1 + exp(-Z)));

	// cave height
	static const float scale3 = 0.001f;
	const FVector v3(WorldPos.X * scale3, WorldPos.Y * scale3, 0); // extra cave height
	const float Noise3 = PerlinNoise(v3);
	const float BaseCaveHeight = 400;
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
	const float CaveHeightF = CaveHeight * 160; // 80000 -> 473
	float CaveLayer = 1 - exp(-pow((WorldPos.Z + CaveLevel), 2) / CaveHeightF); // 80000 -> 473 = 473 * 169.13

	if (WorldPos.Z < -(BaseCaveLevel - 1200) && WorldPos.Z > -(BaseCaveLevel + 1200)) {
		const float CaveLayerN = 1 - CaveLayer;
		//Result *= CaveLayer + CaveLayerN * Noise0 * 0.5 + Noise1 * 0.75 + Noise2 * 0.5;
		Result *= CaveLayer + CaveLayerN * Noise0 * 0.5 + Noise1 * 0.75;
		//Result *= CaveLayer;
	}

	//Result = Density * (NoiseMedium * 0.5 + t);

	if (Result < 0) {
		Result = 0;
	}

	if (Result > 1) {
		Result = 1;
	}

	return Result;
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

		return Result;
		
	}

	if (IsCaveLayerZone(ZoneIndex.Z)) {
		//return FunctionMakeCaveLayer(Density, WorldPos);
	}

	return Density;
}

//==========================================
// Foliage (mushrooms)
//==========================================

bool UMainTerrainGeneratorComponent::UseCustomFoliage(const TVoxelIndex& ZoneIndex) {
	if (ZoneIndex.Z <= -2 && ZoneIndex.Z >= -4) {
		return true;
	}

	return false;
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

	if (Biome.IsForest()) {
		AsyncTask(ENamedThreads::GameThread, [=]() {
			FVector Pos = WorldPos;
			Pos.Z = 500;
			//DrawDebugPoint(GetWorld(), Pos, 5.f, FColor(0, 0, 255, 0), true);
		});
	}

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
		}

	}

	// grass
	if (Foliage.Type == ESandboxFoliageType::Grass) {
		const float R = std::sqrt(WorldPos.X * WorldPos.X + WorldPos.Y * WorldPos.Y + WorldPos.Z * WorldPos.Z);

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
	//static const FRotator Direction[7] = { FRotator(0), FRotator(90, 0, 0), FRotator(-90, 0, 0), FRotator(0, 90, 0), FRotator(0, -90, 0), FRotator(0, 0, 90), FRotator(0, 0, -90) };
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
			AsyncTask(ENamedThreads::GameThread, [=]() {
				//DrawDebugPoint(GetWorld(), WorldPos, 5.f, FColor(255, 255, 255, 0), true);
			});

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
		if (Chance < 0.05f) {
			GenerateRandomInstMesh(ZoneInstanceMeshMap, 900, Rnd, ZoneIndex, Vd);
		}
	}

	// green crystals
	if (ZonePos.Z <= -3000) {
		FVector Pos(0);
		FVector Normal(0);
		if (SelectRandomSpawnPoint(Rnd, ZoneIndex, Vd, Pos, Normal)) {
			FVector Scale = FVector(1, 1, 1);
			FRotator Rotation = Normal.Rotation();
			Rotation.Pitch -= 90;
			FTransform NewTransform(Rotation, Pos, Scale);
			AsyncTask(ENamedThreads::GameThread, [=]() {
				//TerrainController->SpawnSandboxObject(300, NewTransform);
			});
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
		GenerateRandomInstMesh(ZoneInstanceMeshMap, 904, Rnd, ZoneIndex, Vd, 1, 7); // chopped woodss
		GenerateRandomInstMesh(ZoneInstanceMeshMap, 902, Rnd, ZoneIndex, Vd, 1, 5); // logs
		GenerateRandomInstMesh(ZoneInstanceMeshMap, 903, Rnd, ZoneIndex, Vd);	// stump

		AsyncTask(ENamedThreads::GameThread, [=]() {
			FVector ZonePos = GetController()->GetZonePos(ZoneIndex);
			//DrawDebugBox(GetWorld(), ZonePos, FVector(USBT_ZONE_SIZE / 2), FColor(255, 255, 255, 0), true);
		});
	}
}


bool UMainTerrainGeneratorComponent::SpawnCustomFoliage(const TVoxelIndex& Index, const FVector& WorldPos, int32 FoliageTypeId, FSandboxFoliage FoliageType, FRandomStream& Rnd, FTransform& Transform) {

	/*
	ATerrainController* TerrainController = (ATerrainController*)GetController();
	ALevelController* LevelController = TerrainController->LevelController;

	if (!LevelController) {
		return false;
	}

	FVector Pos = WorldPos;

	// cave height
	static const float scale3 = 0.001f;
	const FVector v3(WorldPos.X * scale3, WorldPos.Y * scale3, 0); // extra cave height
	const float Noise3 = PerlinNoise(v3);
	const float BaseCaveHeight = 400;
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
	const float BaseCaveLevel = 3000;
	const float ExtraCaveLevel = 1000 * Noise4;
	const float CaveLevel = BaseCaveLevel + ExtraCaveLevel;

	static const float scale1 = 0.001f; // small
	const FVector v1(WorldPos.X * scale1, WorldPos.Y * scale1, 0); // just noise
	const float Noise1 = PerlinNoise(v1);

	float test = (-CaveLevel - CaveHeight / 2);
	if (WorldPos.Z < test - 200 && WorldPos.Z > test - 220) { // && WorldPos.Z > test - 105
		//FVector rrr = WorldPos;
		//rrr.Z += 10;
		//if (Noise1 > 0) {
		//	Foliage.Probability = 0;
		//}


		float Chance = Rnd.FRandRange(0.f, 1.f);
		Pos.Z += 20;

		float Probability = FoliageType.Probability;
		if (Chance <= Probability / 20) {
			TSubclassOf<ASandboxObject> Obj = LevelController->GetSandboxObjectByClassId(200);
			float Pitch = Rnd.FRandRange(0.f, 10.f);
			float Roll = Rnd.FRandRange(0.f, 360.f);
			float Yaw = Rnd.FRandRange(0.f, 10.f);
			float ScaleZ = Rnd.FRandRange(FoliageType.ScaleMaxZ, FoliageType.ScaleMaxZ * 2);
			FVector Scale = FVector(ScaleZ, ScaleZ, ScaleZ);
			FRotator Rotation(Pitch, Roll, Yaw);
			FTransform NewTransform(Rotation, Pos, Scale);

			UWorld* World = TerrainController->GetWorld();

			AsyncTask(ENamedThreads::GameThread, [=]() {
				World->SpawnActor(Obj->ClassDefaultObject->GetClass(), &NewTransform);
			});

			return false;
		}

		if (Chance <= Probability) {
			float Angle = Rnd.FRandRange(0.f, 360.f);
			float ScaleZ = Rnd.FRandRange(FoliageType.ScaleMinZ, FoliageType.ScaleMaxZ);
			FVector Scale = FVector(ScaleZ, ScaleZ, ScaleZ);
			Transform = FTransform(FRotator(0, Angle, 0), Pos, Scale);
			return true;
		}
	}
	*/

	return false;
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
	float Lvl = Super::GroundLevelFunction(ZoneIndex, WorldPos);

	if (LandscapeStructureMap.find(ZoneIndex) != LandscapeStructureMap.end()) {
		const auto& StructureList = LandscapeStructureMap.at(ZoneIndex);
		if (StructureList.size() > 0) {
			for (const auto& LandscapeHandler : StructureList) {
				if (LandscapeHandler.Function) {
					Lvl = LandscapeHandler.Function(Lvl, ZoneIndex, WorldPos);
				}
			}
		}
	}

	return Lvl;
}

void UMainTerrainGeneratorComponent::AddLandscapeStructure(const TLandscapeZoneHandler& Structure) {
	auto& StructureList = LandscapeStructureMap[Structure.ZoneIndex];
	StructureList.push_back(Structure);
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

			AsyncTask(ENamedThreads::GameThread, [=]() {
				FVector ZonePos = GetController()->GetZonePos(ZoneIndex);
				//DrawDebugBox(GetWorld(), ZonePos, FVector(USBT_ZONE_SIZE / 2), FColor(255, 255, 255, 0), true);
			});
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
			}
		}
	}
}


TMaterialId UMainTerrainGeneratorComponent::MaterialFuncionExt(const TGenerateVdTempItm* GenItm, const TMaterialId MatId, const FVector& WorldPos) const {

	auto ZoneIndex = GenItm->ZoneIndex;
	if (ZoneIndex.X == 0 && ZoneIndex.Y == 0 && ZoneIndex.Z == -2) {
//		UE_LOG(LogTemp, Warning, TEXT("MaterialFuncionExt"));
	}

	if (GenItm->OreData != nullptr) {
		const TZoneOreData* ZoneOreData = GenItm->OreData.get();
		FVector Tmp = WorldPos - ZoneOreData->Origin;
		float R = std::sqrt(Tmp.X * Tmp.X + Tmp.Y * Tmp.Y + Tmp.Z * Tmp.Z);
		if (R < 250) {
			return ZoneOreData->MatId;
		}
	}
	
	
	/*
	const TZoneOreData* ZoneOreItmPtr = ZoneOreMap.Find(GenItm->ZoneIndex);
	if (ZoneOreItmPtr) {
		FVector Tmp = WorldPos - ZoneOreItmPtr->Origin;
		float R = std::sqrt(Tmp.X * Tmp.X + Tmp.Y * Tmp.Y + Tmp.Z * Tmp.Z);

		if (R < 250) {
			//return 6;
		}
	}
	*/
	
	return MatId;
}

