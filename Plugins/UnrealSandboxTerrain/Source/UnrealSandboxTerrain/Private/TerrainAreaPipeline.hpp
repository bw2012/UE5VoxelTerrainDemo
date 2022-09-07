#pragma once

#include "EngineMinimal.h"
#include "VoxelIndex.h"

//======================================================================================================================================================================
//
//======================================================================================================================================================================

typedef struct TTerrainAreaPipelineParams {
	float Radius = 3000;
	float FullLodDistance = 1000;
	int32 TerrainSizeMinZ = 5;
	int32 TerrainSizeMaxZ = 5;

	TSet<TVoxelIndex> Ignore;

	std::function<void(uint32, uint32)> OnProgress = nullptr;

} TTerrainAreaPipelineParams;


class TTerrainAreaPipeline {

public:

	TTerrainAreaPipeline() {}

	virtual ~TTerrainAreaPipeline() { 
		// UE_LOG(LogSandboxTerrain, Log, TEXT("~TTerrainLoadHandler()")); 
	}

	TTerrainAreaPipeline(FString Name_, ASandboxTerrainController* Controller_) :
		Name(Name_), Controller(Controller_) {}

	TTerrainAreaPipeline(FString Name_, ASandboxTerrainController* Controller_, TTerrainAreaPipelineParams Params_) :
		Name(Name_), Controller(Controller_), Params(Params_) {
	}

protected:
	FString Name;
	ASandboxTerrainController* Controller;
	TTerrainAreaPipelineParams Params;
	FVector AreaOrigin;
	TVoxelIndex OriginIndex;
	uint32 Total = 0;
	uint32 Progress = 0;
	uint32 SaveGeneratedZones = 1000;
	bool bIsStopped = false;

protected:

	virtual void PerformZone(const TVoxelIndex& Index) {

	}

	virtual void EndChunk(int x, int y) {

	}

	virtual void BeginChunk(int x, int y) {

	}

private:

	void PerformChunk(int X, int Y) {
		for (int Z = Params.TerrainSizeMinZ; Z <= Params.TerrainSizeMaxZ; Z++) {
			TVoxelIndex Index(X + OriginIndex.X, Y + OriginIndex.Y, Z);

			if (!Params.Ignore.Contains(Index)) {
				PerformZone(Index);
			}

			Progress++;

			if (Params.OnProgress) {
				Params.OnProgress(Progress, Total);
			}

			if (Controller->IsWorkFinished() || bIsStopped) {
				return;
			}
		}
	}

	void AreaWalkthrough() {
		const unsigned int AreaRadius = Params.Radius / 1000;
		Total = (AreaRadius * 2 + 1) * (AreaRadius * 2 + 1) * (Params.TerrainSizeMinZ + Params.TerrainSizeMaxZ + 1);
		auto List = ReverseSpiralWalkthrough(AreaRadius);
		for (auto& Itm : List) {
			int x = Itm.X;
			int y = Itm.Y;

			BeginChunk(x, y);
			PerformChunk(x, y);
			EndChunk(x, y);

			if (Controller->IsWorkFinished() || bIsStopped) {
				return;
			}
		}
	}

public:

	void Cancel() {
		this->bIsStopped = true;
	}

	void SetParams(FString NewName, ASandboxTerrainController* NewController, TTerrainAreaPipelineParams NewParams) {
		this->Name = NewName;
		this->Controller = NewController;
		this->Params = NewParams;
	}

	void LoadArea(const FVector& Origin) {
		if (this->Controller) {
			this->AreaOrigin = Origin;
			this->OriginIndex = Controller->GetZoneIndex(Origin);
			AreaWalkthrough();
		}
	}

	void LoadArea(const TVoxelIndex& ZoneIndex) {
		if (this->Controller) {
			this->AreaOrigin = Controller->GetZonePos(ZoneIndex);
			this->OriginIndex = ZoneIndex;
			AreaWalkthrough();
		}
	}
};


class TTerrainLoadPipeline : public TTerrainAreaPipeline  {

public:

	using TTerrainAreaPipeline::TTerrainAreaPipeline;

protected :

	virtual void PerformZone(const TVoxelIndex& Index) override {
		TTerrainLodMask TerrainLodMask = (TTerrainLodMask)ETerrainLodMaskPreset::All;
		FVector ZonePos = Controller->GetZonePos(Index);
		FVector ZonePosXY(ZonePos.X, ZonePos.Y, 0);
		float Distance = FVector::Distance(AreaOrigin, ZonePosXY);

		/*
		if (Distance > Params.FullLodDistance) {
			float Delta = Distance - Params.FullLodDistance;
			if (Delta > Controller->LodDistance.Distance2) {
				TerrainLodMask = (TTerrainLodMask)ETerrainLodMaskPreset::Medium;
			} if (Delta > Controller->LodDistance.Distance5) {
				TerrainLodMask = (TTerrainLodMask)ETerrainLodMaskPreset::Far;
			}
		}*/

		double Start = FPlatformTime::Seconds();

		TArray<TSpawnZoneParam> SpawnList;
		TSpawnZoneParam SpawnZoneParam;
		SpawnZoneParam.Index = Index;
		SpawnZoneParam.TerrainLodMask = TerrainLodMask;
		SpawnList.Add(SpawnZoneParam);

		// batch with one zone. CPU only
		Controller->BatchSpawnZone(SpawnList);

		double End = FPlatformTime::Seconds();
		double Time = (End - Start) * 1000;
		//UE_LOG(LogSandboxTerrain, Log, TEXT("BatchSpawnZone -> %f ms - %d %d %d"), Time, Index.X, Index.Y, Index.Z);
	}
};

class TCheckAreaMap {
public:
	TMap<uint32, std::shared_ptr<TTerrainLoadPipeline>> PlayerStreamingHandler;
	TMap<uint32, FVector> PlayerStreamingPosition;
};


