#include "MainTerrainGeneratorComponent.h"
#include "TerrainController.h"
#include "UnrealSandboxTerrain.h"


class TStructureSphereCavern : public TMetaStructure {

private:

	float Radius = 1500;

public:

	TStructureSphereCavern(TVoxelIndex OriginIndex_, float Radius_) : Radius(Radius_) {
		OriginIndex = OriginIndex_;
	};

	TArray<TVoxelIndex> GetRelevantZones(TStructuresGenerator* Generator) const {
		TArray<TVoxelIndex> Res;

		const FVector Origin = Generator->GetController()->GetZonePos(OriginIndex);

		const float R = Radius + 100;

		const FVector Max(Origin.X + R, Origin.Y + R, Origin.Z + R);
		const FVector Min(Origin.X - R, Origin.Y - R, Origin.Z - R);

		const TVoxelIndex MinIndex = Generator->GetController()->GetZoneIndex(Min);
		const TVoxelIndex MaxIndex = Generator->GetController()->GetZoneIndex(Max);

		for (auto X = MinIndex.X; X <= MaxIndex.X; X++) {
			for (auto Y = MinIndex.Y; Y <= MaxIndex.Y; Y++) {
				for (auto Z = MinIndex.Z; Z <= MaxIndex.Z; Z++) {
					Res.Add(TVoxelIndex(X, Y, Z));
				}
			}
		}

		return Res;
	}

	void MakeMetaData(TStructuresGenerator* Generator) const {
		const FVector Origin = Generator->GetController()->GetZonePos(OriginIndex);

		const float Radius2 = Radius;
		const UMainTerrainGeneratorComponent* Generator2 = (UMainTerrainGeneratorComponent*)Generator->GetGeneratorComponent();

		const auto Function = [=](const float InDensity, const TMaterialId InMaterialId, const TVoxelIndex& VoxelIndex, const FVector& LocalPos, const FVector& WorldPos) {
			const float Density = Generator2->FunctionMakeSphere(InDensity, WorldPos, Origin, Radius2, 1);
			return std::make_tuple(Density, InMaterialId);
		};

		const auto Function2 = [=](const TVoxelIndex& ZoneIndex, const FVector& WorldPos, const FVector& LocalPos) {
			const FVector P = WorldPos - Origin;
			const float R = std::sqrt(P.X * P.X + P.Y * P.Y + P.Z * P.Z);
			if (R < Radius2) {
				return false;
			}
			return true;
		};

		TZoneStructureHandler Str;
		Str.Function = Function;
		Str.LandscapeFoliageFilter = Function2;

		for (TVoxelIndex Index : GetRelevantZones(Generator)) {
			Generator->AddZoneStructure(Index, Str);
		}
	}
};


class TStructureGeoSphere : public TMetaStructure {

private:

	float Radius = 1500;

public:

	TStructureGeoSphere(TVoxelIndex OriginIndex_, float Radius_) : Radius(Radius_) {
		OriginIndex = OriginIndex_;
	};

	TArray<TVoxelIndex> GetRelevantZones(TStructuresGenerator* Generator) const {
		TArray<TVoxelIndex> Res;

		const FVector Origin = Generator->GetController()->GetZonePos(OriginIndex);

		const float R = Radius;

		const FVector Max(Origin.X + R, Origin.Y + R, Origin.Z + R);
		const FVector Min(Origin.X - R, Origin.Y - R, Origin.Z - R);

		const TVoxelIndex MinIndex = Generator->GetController()->GetZoneIndex(Min);
		const TVoxelIndex MaxIndex = Generator->GetController()->GetZoneIndex(Max);

		for (auto X = MinIndex.X; X <= MaxIndex.X; X++) {
			for (auto Y = MinIndex.Y; Y <= MaxIndex.Y; Y++) {
				for (auto Z = MinIndex.Z; Z <= MaxIndex.Z; Z++) {
					Res.Add(TVoxelIndex(X, Y, Z));
				}
			}
		}

		return Res;
	}

	void MakeMetaData(TStructuresGenerator* Generator) const {
		const FVector Origin = Generator->GetController()->GetZonePos(OriginIndex);

		const float Radius2 = Radius;
		const UMainTerrainGeneratorComponent* Generator2 = (UMainTerrainGeneratorComponent*)Generator->GetGeneratorComponent();

		const auto Function = [=](const float InDensity, const TMaterialId InMaterialId, const TVoxelIndex& VoxelIndex, const FVector& LocalPos, const FVector& WorldPos) {
			return Generator2->FunctionMakeSolidSphere(InDensity, InMaterialId, WorldPos, Origin, Radius2, 90);
		};

		const auto Function2 = [=](const TVoxelIndex& ZoneIndex, const FVector& WorldPos, const FVector& LocalPos) {
			const FVector P = WorldPos - Origin;
			const float R = std::sqrt(P.X * P.X + P.Y * P.Y + P.Z * P.Z);
			if (R < Radius2) {
				return false;
			}
			return true;
		};

		TZoneStructureHandler Str;
		Str.Function = Function;
		Str.LandscapeFoliageFilter = Function2;

		for (TVoxelIndex Index : GetRelevantZones(Generator)) {
			Generator->AddZoneStructure(Index, Str);
		}
	}
};


class TStructureTunnel1 : public TMetaStructure {

private:

	FBox TunnelBox;

public:

	TStructureTunnel1(const FBox TunnelBox_) {
		TunnelBox = TunnelBox_;
	};

	TArray<TVoxelIndex> GetRelevantZones(TStructuresGenerator* Generator) const {
		TArray<TVoxelIndex> Res;

		const TVoxelIndex MinIndex = Generator->GetController()->GetZoneIndex(TunnelBox.Min);
		const TVoxelIndex MaxIndex = Generator->GetController()->GetZoneIndex(TunnelBox.Max);

		for (auto X = MinIndex.X; X <= MaxIndex.X; X++) {
			for (auto Y = MinIndex.Y; Y <= MaxIndex.Y; Y++) {
				Res.Add(TVoxelIndex(X, Y, MinIndex.Z));
			}
		}

		return Res;
	}

	void MakeMetaData(TStructuresGenerator* Generator) const {
		const FBox Box = TunnelBox;
		const UMainTerrainGeneratorComponent* Generator2 = (UMainTerrainGeneratorComponent*)Generator->GetGeneratorComponent();

		const auto Function = [=](const float InDensity, const TMaterialId InMaterialId, const TVoxelIndex& VoxelIndex, const FVector& LocalPos, const FVector& WorldPos) {
			const float Density = Generator2->FunctionMakeBox(InDensity, WorldPos, Box);
			const TMaterialId MaterialId = InMaterialId;
			return std::make_tuple(Density, MaterialId);
		};

		TZoneStructureHandler Str;
		Str.Function = Function;

		for (TVoxelIndex Index : GetRelevantZones(Generator)) {
			Generator->AddZoneStructure(Index, Str);
		}
	}
};

void MakeLandscapeHill(TStructuresGenerator* Generator, const FVector& Origin, const int TypeIdx = 0) {
	FVector O(Origin.X, Origin.Y, 0);
	TVoxelIndex Index = Generator->GetController()->GetZoneIndex(O);

	struct TSizeType {
		TSizeType(float MaxR_, float H_, int S_) : MaxR(MaxR_), H(H_), S(S_) { };
		float MaxR;
		float H;
		int S;
	};

	const TSizeType SizeType[] = { TSizeType(4800, 6, 2), TSizeType(4800 * 20, 6 * 6, 9) };

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
		}
	}
}

void MakeLandscapeHollow(TStructuresGenerator* Generator, const FVector& Origin, const int TypeIdx = 0) {
	FVector O(Origin.X, Origin.Y, 0);
	TVoxelIndex Index = Generator->GetController()->GetZoneIndex(O);

	const UMainTerrainGeneratorComponent* Generator2 = (UMainTerrainGeneratorComponent*)Generator->GetGeneratorComponent();

	struct TSizeType {
		TSizeType(float MaxR_, float H_, int S_) : MaxR(MaxR_), H(H_), S(S_) { };
		float MaxR;
		float H;
		int S;
	};

	const TSizeType SizeType[] = { TSizeType(4800, 6, 2), TSizeType(4800 * 20, 6 * 6 * 1.8, 9) };

	const float MaxR = SizeType[TypeIdx].MaxR; // max radius
	const float H = SizeType[TypeIdx].H; // height

	const auto Function = [=](const float InLvl, const TVoxelIndex& VoxelIndex, const FVector& WorldPos) {
		FVector Tmp = WorldPos - O;
		float R = std::sqrt(Tmp.X * Tmp.X + Tmp.Y * Tmp.Y);
		//float T = (1 - exp(-pow(R, 2) / ( MaxR * 100))); // hollow
		float T = exp(-pow(R, 2) / (MaxR * 100)) * H; // hill
		return InLvl - (T * 100);
	};

	const int S = SizeType[TypeIdx].S; //3;

	for (auto X = -S; X <= S; X++) {
		for (auto Y = -S; Y <= S; Y++) {
			TLandscapeZoneHandler LandscapeZoneHandler;
			LandscapeZoneHandler.ZoneIndex = TVoxelIndex(Index.X + X, Index.Y + Y, 0);
			LandscapeZoneHandler.Function = Function;
			Generator->AddLandscapeStructure(LandscapeZoneHandler);


			AsyncTask(ENamedThreads::GameThread, [=]() {
				const FVector Pos0 = Generator->GetController()->GetZonePos(LandscapeZoneHandler.ZoneIndex);
				//DrawDebugBox(Generator->GetController()->GetWorld(), Pos0, FVector(USBT_ZONE_SIZE / 2), FColor(255, 255, 255, 0), true);
			});
		}
	}
}


class TStructureBigHill : public TMetaStructure {

private:

	std::shared_ptr<TMetaStructure> Cavern = nullptr;

public:

	TStructureBigHill(TVoxelIndex OriginIndex_) {
		OriginIndex = OriginIndex_;

		FVector Origin(OriginIndex.X * 1000, OriginIndex.Y * 1000, 0);

		const float Width = 1000 * 3 - 200;
		const float Height = 800;

		const float W = Width / 2;
		const float H = Height / 2;

		const FVector Min(Origin.X - W, Origin.Y - W, Origin.Z - H);
		const FVector Max(Origin.X + W, Origin.Y + W, Origin.Z + H);
		FBox Box(Min, Max);
		Cavern = std::shared_ptr<TMetaStructure>(new TStructureTunnel1(Box));
	};

	TArray<TVoxelIndex> GetRelevantZones(TStructuresGenerator* Generator) const {
		TArray<TVoxelIndex> Res;

		// TODO

		return Res;
	}

	void MakeMetaData(TStructuresGenerator* Generator) const {
		FVector Origin(OriginIndex.X * 1000, OriginIndex.Y * 1000, 0);
		MakeLandscapeHill(Generator, Origin, 1);
		Cavern->MakeMetaData(Generator);
	}

};


class TStructureBigHollow : public TMetaStructure {

public:

	TStructureBigHollow(TVoxelIndex OriginIndex_) {
		OriginIndex = OriginIndex_;
	};

	TArray<TVoxelIndex> GetRelevantZones(TStructuresGenerator* Generator) const {
		TArray<TVoxelIndex> Res;

		// TODO

		return Res;
	}

	void MakeMetaData(TStructuresGenerator* Generator) const {
		FVector Origin(OriginIndex.X * 1000, OriginIndex.Y * 1000, 0);
		MakeLandscapeHollow(Generator, Origin, 1);		
	}

};

// =============================================================================================


void StructureHotizontalBoxTunnel(TStructuresGenerator* Generator, const FBox TunnelBox) {

	const UMainTerrainGeneratorComponent* Generator2 = (UMainTerrainGeneratorComponent*)Generator->GetGeneratorComponent();

	const auto Function = [=](const float InDensity, const TMaterialId InMaterialId, const TVoxelIndex& VoxelIndex, const FVector& LocalPos, const FVector& WorldPos) {
		const float Density = Generator2->FunctionMakeBox(InDensity, WorldPos, TunnelBox);
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
		}
	}
}

void StructureVerticalCylinderTunnel(TStructuresGenerator* Generator, const FVector& Origin, const float Radius, const float Top, const float Bottom) {

	const UMainTerrainGeneratorComponent* Generator2 = (UMainTerrainGeneratorComponent*)Generator->GetGeneratorComponent();

	const auto Function = [=](const float InDensity, const TMaterialId InMaterialId, const TVoxelIndex& VoxelIndex, const FVector& LocalPos, const FVector& WorldPos) {
		const float Density = Generator2->FunctionMakeVerticalCylinder(InDensity, WorldPos, Origin, 300.f, 2000.f, -3000.f);
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

void StructureDiagonalCylinderTunnel(TStructuresGenerator* Generator, const FVector& Origin, const float Radius, const float Top, const float Bottom, const int Dir) {
	static const FRotator DirRotation[2] = { FRotator(-45, 0, 0), FRotator(0, 0, -45) };
	static const TVoxelIndex DirIndex[2] = { TVoxelIndex(1, 0, 0), TVoxelIndex(0, -1, 0) };

	const UMainTerrainGeneratorComponent* Generator2 = (UMainTerrainGeneratorComponent*)Generator->GetGeneratorComponent();

	const auto Function = [=](const float InDensity, const TMaterialId InMaterialId, const TVoxelIndex& VoxelIndex, const FVector& LocalPos, const FVector& WorldPos) {
		const FRotator& Rotator = DirRotation[Dir];
		FVector Tmp = Rotator.RotateVector(WorldPos - Origin);
		Tmp += Origin;

		const static float Sqrt2 = 1.414213;

		const float Density = Generator2->FunctionMakeVerticalCylinder(InDensity, Tmp, Origin, Radius, Top, Bottom * Sqrt2 - 350, 0.66);
		return std::make_tuple(Density, InMaterialId);
	};

	const auto Function2 = [=](const TVoxelIndex& ZoneIndex, const FVector& WorldPos, const FVector& LocalPos) {
		const FRotator& Rotator = DirRotation[Dir];
		FVector Tmp = Rotator.RotateVector(WorldPos - Origin);
		Tmp += Origin;

		static const float E = 25;
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



void MakeDungeon1(TStructuresGenerator* Generator) {
	const int StartX = 4000;
	const int StartY = 0;
	const int StartZ = 0;

	const float Width = 800;
	const float Height = 800;

	const float W = Width / 2;
	const float H = Height / 2;

	const static float Deep = 4000;

	const UMainTerrainGeneratorComponent* Generator2 = (UMainTerrainGeneratorComponent*)Generator->GetGeneratorComponent();

	MakeLandscapeHill(Generator, FVector(StartX + 500, StartY, 0));

	const TVoxelIndex Index = Generator->GetController()->GetZoneIndex(FVector(StartX, StartY, StartZ));
	float G = Generator2->GroundLevelFunction(Index, FVector(StartX, StartY, 0));

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
		// level 1 -> 2
		const FVector Min2(O.X - Width / 2, O.Y - Width / 2 - 500, O.Z - Height / 2);
		const FVector Max2(O.X + Width / 2, O.Y + Width / 2 + 500, O.Z + Height / 2);
		FBox Box2(Min2, Max2);

		StructureDiagonalCylinderTunnel(Generator, O + FVector(-15000, 0.f, 0), 200.f, 0, -2000, 1);
		StructureHotizontalBoxTunnel(Generator, Box2.ShiftBy(FVector(-15000, -3000, -2000)));
	}

	const int MaxDungeonLevels = 7;

	//AsyncTask(ENamedThreads::GameThread, [=]() { DrawDebugPoint(Generator->GetWorld(), O, 5.f, FColor(255, 255, 255, 0), true); });

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


void UMainTerrainGeneratorComponent::RegionGenerateStructures(int RegionX, int RegionY) {

}


void UMainTerrainGeneratorComponent::GenerateStructures() {
	//TStructureGeoSphere TestGeoSphere(TVoxelIndex(0, -2, -1), 1400);
	//TestGeoSphere.MakeMetaData(this);

	//TStructureSphereCavern Test2(TVoxelIndex(0, -2, -1), 1400);
	//Test2.MakeMetaData(this);

	MakeDungeon1(GetStructuresGenerator());

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
			FVector Origin(X * 1000, Y * 1000, 0);

			TStructureBigHill Bighill(TVoxelIndex(X, Y, 0));
			Bighill.MakeMetaData(GetStructuresGenerator());

		} while (!bIsValid);
	}

	for (int I = 0; I < 100; I++) {
		do {
			X = Rnd.RandRange(-RegionSize, RegionSize);
			Y = Rnd.RandRange(-RegionSize, RegionSize);
			const float R = std::sqrt(X * X + Y * Y);

			bIsValid = true;
			FVector Origin(X * 1000, Y * 1000, -10000);

			TStructureSphereCavern Test(TVoxelIndex(X, Y, -10), 1400);
			//Test.MakeMetaData(this);
		} while (!bIsValid);
	}


	TVoxelIndex TestIndex(-200, 0, 0);
	TStructureBigHollow Hollow(TestIndex);
	Hollow.MakeMetaData(GetStructuresGenerator());


	int S = 2;

	for (auto X1 = -S; X1 <= S; X1++) {
		for (auto Y1 = -S; Y1 <= S; Y1++) {
			TVoxelIndex TTT(TestIndex.X + X1, TestIndex.Y + Y1, 0);
			SetChunkTag(TTT, "no_tree", "Y");

			AsyncTask(ENamedThreads::GameThread, [=]() {
				const FVector Pos0 = GetController()->GetZonePos(TTT);
				//DrawDebugBox(GetController()->GetWorld(), Pos0, FVector(USBT_ZONE_SIZE / 2), FColor(255, 255, 255, 0), true);
			});

		}
	}

}