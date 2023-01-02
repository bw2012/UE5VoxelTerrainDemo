#include "SandboxTerrainController.h"
#include "Async/Async.h"
#include "DrawDebugHelpers.h"
#include "kvdb.hpp"

#include "TerrainZoneComponent.h"
#include "VoxelMeshComponent.h"

#include "TerrainServerComponent.h"
#include "TerrainClientComponent.h"

#include <cmath>
#include <list>

#include "serialization.hpp"
#include "utils.hpp"
#include "VoxelDataInfo.hpp"
#include "TerrainData.hpp"
#include "TerrainAreaHelper.hpp"
#include "TerrainEdit.hpp"
#include "ThreadPool.hpp"

#include "memstat.h"


// ====================================
// FIXME 
// ====================================
bool bIsGameShutdown;

bool IsGameShutdown() {
	return bIsGameShutdown;
}
// ====================================


//======================================================================================================================================================================
// Terrain Controller
//======================================================================================================================================================================


void ASandboxTerrainController::InitializeTerrainController() {
	PrimaryActorTick.bCanEverTick = true;
	MapName = TEXT("World 0");
	ServerPort = 6000;
    AutoSavePeriod = 20;
    TerrainData = new TTerrainData();
    CheckAreaMap = new TCheckAreaMap();
	bSaveOnEndPlay = true;
	BeginServerTerrainLoadLocation = FVector(0);
	bSaveAfterInitialLoad = false;
	bReplicates = false;
}

void ASandboxTerrainController::BeginDestroy() {
    Super::BeginDestroy();
	bIsGameShutdown = true;
}

void ASandboxTerrainController::FinishDestroy() {
	Super::FinishDestroy();
	delete TerrainData;
	delete CheckAreaMap;

	//UE_LOG(LogSandboxTerrain, Warning, TEXT("vd -> %d, md -> %d, cd -> %d"), vd::tools::memory::getVdCount(), md_counter.load(), cd_counter.load());
}

ASandboxTerrainController::ASandboxTerrainController(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
	InitializeTerrainController();
}

ASandboxTerrainController::ASandboxTerrainController() {
	InitializeTerrainController();
}

void ASandboxTerrainController::PostLoad() {
	Super::PostLoad();
	UE_LOG(LogSandboxTerrain, Log, TEXT("ASandboxTerrainController::PostLoad()"));

	bIsGameShutdown = false;

#if WITH_EDITOR
	//spawnInitialZone();
#endif

}

UTerrainGeneratorComponent* ASandboxTerrainController::NewTerrainGenerator() {
	return NewObject<UTerrainGeneratorComponent>(this, TEXT("TerrainGenerator"));
}

UTerrainGeneratorComponent* ASandboxTerrainController::GetTerrainGenerator() {
	return this->GeneratorComponent;
}

void ASandboxTerrainController::BeginPlay() {
	Super::BeginPlay();
	UE_LOG(LogSandboxTerrain, Log, TEXT("ASandboxTerrainController::BeginPlay()"));

	ThreadPool = new TThreadPool(5);
	Conveyor = new TConveyour();

	GeneratorComponent = NewTerrainGenerator();
	GeneratorComponent->RegisterComponent();

	bIsGameShutdown = false;
    
	FoliageMap.Empty();
	if (FoliageDataAsset) {
		FoliageMap = FoliageDataAsset->FoliageMap;
	}

	InstMeshMap.Empty();
	MaterialMap.Empty();
	if (TerrainParameters) {
		MaterialMap = TerrainParameters->MaterialMap;

		for (const auto& InstMesh : TerrainParameters->InstanceMeshes) {
			uint64 MeshTypeCode = InstMesh.GetMeshTypeCode();
			UE_LOG(LogSandboxTerrain, Log, TEXT("MeshTypeCode -> %lld"), MeshTypeCode);
			InstMeshMap.Add(MeshTypeCode, InstMesh);
		}
	}

	if (!GetWorld()) return;
	bIsLoadFinished = false;

	if (GetNetMode() == NM_Client) {
		UE_LOG(LogSandboxTerrain, Warning, TEXT("================== CLIENT =================="));
		BeginPlayClient();
	} else {
		if (GetNetMode() == NM_DedicatedServer) {
			UE_LOG(LogSandboxTerrain, Warning, TEXT("================== DEDICATED SERVER =================="));
		} 
		
		if (GetNetMode() == NM_ListenServer) {
			UE_LOG(LogSandboxTerrain, Warning, TEXT("================== LISTEN SERVER =================="));
		}

		BeginPlayServer();
	}
}

void ASandboxTerrainController::ShutdownThreads() {
	bIsWorkFinished = true;

	UE_LOG(LogSandboxTerrain, Warning, TEXT("Shutdown thread pool..."));
	ThreadPool->shutdownAndWait();

	UE_LOG(LogSandboxTerrain, Warning, TEXT("Conveyor -> %d threads. Waiting for finish..."), Conveyor->size());
	while (true) {
		std::function<void()> Function;
		if (Conveyor->pop(Function)) {
			Function();
		} else {
			break;
		}
	}
}

void ASandboxTerrainController::EndPlay(const EEndPlayReason::Type EndPlayReason) {
	Super::EndPlay(EndPlayReason);
	UE_LOG(LogSandboxTerrain, Warning, TEXT("ASandboxTerrainController::EndPlay"));

	ShutdownThreads();

	if (bSaveOnEndPlay || GetNetMode() == NM_DedicatedServer) {
		Save();
	}

	CloseFile();
    TerrainData->Clean();
	GetTerrainGenerator()->Clean();

	SaveTerrainMetadata();
	ModifiedVdMap.Empty();

	delete ThreadPool;
	delete Conveyor;
}

void ASandboxTerrainController::Tick(float DeltaTime) {
	Super::Tick(DeltaTime);

	double ConvTime = 0;
	while (ConvTime < ConveyorMaxTime) {
		std::function<void()> Function;
		if (Conveyor->pop(Function)) {
			double Start = FPlatformTime::Seconds();
			Function();
			double End = FPlatformTime::Seconds();
			ConvTime += (End - Start);
		} else {
			break;
		}
	}
}

//======================================================================================================================================================================
// Swapping terrain area according player position
//======================================================================================================================================================================

void ASandboxTerrainController::StartPostLoadTimers() {
	if (AutoSavePeriod > 0) {
		GetWorld()->GetTimerManager().SetTimer(TimerAutoSave, this, &ASandboxTerrainController::AutoSaveByTimer, AutoSavePeriod, true);
	}
}

void ASandboxTerrainController::StartCheckArea() {
	GetWorld()->GetTimerManager().SetTimer(TimerSwapArea, this, &ASandboxTerrainController::PerformCheckArea, 0.25, true);
}

void ASandboxTerrainController::PerformCheckArea() {
    if(!bEnableAreaStreaming){
        return;
    }
    
    double Start = FPlatformTime::Seconds();
        
	bool bPerformSoftUnload = false;
    for (auto Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator) {
        APlayerController* PlayerController = Iterator->Get();
        if (PlayerController){
            const auto PlayerId = PlayerController->GetUniqueID();
            const auto Pawn = PlayerController->GetPawn();
			if (!Pawn) {
				continue;
			}

            const FVector PlayerLocation = Pawn->GetActorLocation();
            const FVector PrevLocation = CheckAreaMap->PlayerStreamingPosition.FindOrAdd(PlayerId);
            const float Distance = FVector::Distance(PlayerLocation, PrevLocation);
            const float Threshold = PlayerLocationThreshold;
            if(Distance > Threshold) {
                CheckAreaMap->PlayerStreamingPosition[PlayerId] = PlayerLocation;
                TVoxelIndex LocationIndex = GetZoneIndex(PlayerLocation);
                FVector Tmp = GetZonePos(LocationIndex);
                                
                if(CheckAreaMap->PlayerStreamingHandler.Contains(PlayerId)){
                    // cancel old
                    std::shared_ptr<TTerrainLoadHelper> HandlerPtr2 = CheckAreaMap->PlayerStreamingHandler[PlayerId];
                    HandlerPtr2->Cancel();
                    CheckAreaMap->PlayerStreamingHandler.Remove(PlayerId);
                }
                
                // start new
                std::shared_ptr<TTerrainLoadHelper> HandlerPtr = std::make_shared<TTerrainLoadHelper>();
				CheckAreaMap->PlayerStreamingHandler.Add(PlayerId, HandlerPtr);

                if(bShowStartSwapPos){
                    DrawDebugBox(GetWorld(), PlayerLocation, FVector(100), FColor(255, 0, 255, 0), false, 15);
                    static const float Len = 1000;
                    DrawDebugCylinder(GetWorld(), FVector(Tmp.X, Tmp.Y, Len), FVector(Tmp.X, Tmp.Y, -Len), DynamicLoadArea.Radius, 128, FColor(255, 0, 255, 128), false, 30);
                }
                
				TTerrainAreaLoadParams Params;
                Params.Radius = DynamicLoadArea.Radius;
                Params.TerrainSizeMinZ = LocationIndex.Z + DynamicLoadArea.TerrainSizeMinZ;
                Params.TerrainSizeMaxZ = LocationIndex.Z + DynamicLoadArea.TerrainSizeMaxZ;
                HandlerPtr->SetParams(TEXT("Player_Swap_Terrain_Task"), this, Params);
                               
                AddAsyncTask([=]() {
                    HandlerPtr->LoadArea(PlayerLocation);
                });

				bPerformSoftUnload = true;
            }

			if (bPerformSoftUnload || bForcePerformHardUnload) {
				UnloadFarZones(PlayerLocation, DynamicLoadArea.Radius);
			}
        }
    }
    
    double End = FPlatformTime::Seconds();
    double Time = (End - Start) * 1000;
    //UE_LOG(LogSandboxTerrain, Log, TEXT("PerformCheckArea -> %f ms"), Time);
}

void ASandboxTerrainController::ForcePerformHardUnload() {
	bForcePerformHardUnload = true;
}

void RemoveAllChilds(UTerrainZoneComponent* ZoneComponent) {
	TArray<USceneComponent*> ChildList;
	ZoneComponent->GetChildrenComponents(true, ChildList);
	for (USceneComponent* Child : ChildList) {
		Child->DestroyComponent(true);
	}
}

void ASandboxTerrainController::UnloadFarZones(FVector PlayerLocation, float Radius) {
	double Start = FPlatformTime::Seconds();

	// hard unload far zones
	TArray<UTerrainZoneComponent*> Components;
	GetComponents<UTerrainZoneComponent>(Components);
	for (UTerrainZoneComponent* ZoneComponent : Components) {
		FVector ZonePos = ZoneComponent->GetComponentLocation();
		const TVoxelIndex ZoneIndex = GetZoneIndex(ZonePos);
		float ZoneDistance = FVector::Distance(ZonePos, PlayerLocation);
		if (ZoneDistance > Radius * 1.5f) {
			ZoneSoftUnload(ZoneComponent, ZoneIndex);
			if (bForcePerformHardUnload) {
				ZoneHardUnload(ZoneComponent, ZoneIndex);
			}
		} else {
			if (ZoneDistance < Radius) {
				// restore soft unload
				TVoxelDataInfoPtr VoxelDataInfoPtr = GetVoxelDataInfo(ZoneIndex);
				if (VoxelDataInfoPtr->IsSoftUnload()) {
					VoxelDataInfoPtr->ResetSoftUnload();
					OnRestoreZoneSoftUnload(ZoneIndex);
				}
			}
		}
	}

	if (bForcePerformHardUnload) {
		bForcePerformHardUnload = false;
	}

	double End = FPlatformTime::Seconds();
	double Time = (End - Start) * 1000;
	//UE_LOG(LogSandboxTerrain, Log, TEXT("UnloadFarZones --> %f ms"), Time);
}

void ASandboxTerrainController::ZoneHardUnload(UTerrainZoneComponent* ZoneComponent, const TVoxelIndex& ZoneIndex) {
	TVoxelDataInfoPtr VdInfoPtr = TerrainData->GetVoxelDataInfo(ZoneIndex);
	TVdInfoLockGuard Lock(VdInfoPtr);

	FVector ZonePos = ZoneComponent->GetComponentLocation();
	if (VdInfoPtr->IsSoftUnload() && !VdInfoPtr->IsNeedObjectsSave()) {
		if (ZoneComponent->bIsSpawnFinished) {
			RemoveAllChilds(ZoneComponent);
			TerrainData->RemoveZone(ZoneIndex);
			ZoneComponent->DestroyComponent(true);
		}
	}
}

void ASandboxTerrainController::ZoneSoftUnload(UTerrainZoneComponent* ZoneComponent, const TVoxelIndex& ZoneIndex) {
	TVoxelDataInfoPtr VdInfoPtr = TerrainData->GetVoxelDataInfo(ZoneIndex);
	if (!VdInfoPtr->IsSoftUnload()) {
		// TODO: lock zone + double check locking
		bool bCanUnload = OnZoneSoftUnload(ZoneIndex);
		if (bCanUnload) {
			// soft unload
			VdInfoPtr->SetSoftUnload();
		}
	} 
}

//======================================================================================================================================================================
// begin play
//======================================================================================================================================================================

bool LoadDataFromKvFile(TKvFile& KvFile, const TVoxelIndex& Index, std::function<void(TValueDataPtr)> Function);

void ASandboxTerrainController::BeginPlayServer() {
	if (!OpenFile()) {
		// TODO error message
		return;
	}

	LoadJson();
	LoadTerrainMetadata();

	BeginServerTerrainLoad();

	if (GetNetMode() == NM_DedicatedServer || GetNetMode() == NM_ListenServer) {
		TerrainServerComponent = NewObject<UTerrainServerComponent>(this, TEXT("TerrainServer"));
		TerrainServerComponent->RegisterComponent();
		TerrainServerComponent->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepRelativeTransform, NAME_None);
	}	
}

void ASandboxTerrainController::BeginClientTerrainLoad(const TVoxelIndex& ZoneIndex, const TSet<TVoxelIndex>& Ignore) {
	TTerrainAreaLoadParams Params;
	Params.Radius = InitialLoadArea.Radius;
	Params.TerrainSizeMinZ = InitialLoadArea.TerrainSizeMinZ;
	Params.TerrainSizeMaxZ = InitialLoadArea.TerrainSizeMaxZ;
	Params.Ignore = Ignore;

	AddAsyncTask([=]() {
		const TVoxelIndex Index = ZoneIndex;
		UE_LOG(LogSandboxTerrain, Warning, TEXT("Client: Begin terrain load at location: %f %f %f"), ZoneIndex.X, ZoneIndex.Y, ZoneIndex.Z);
		TTerrainLoadHelper Loader(TEXT("test_job"), this, Params);
		Loader.LoadArea(Index);
	});
}

void ASandboxTerrainController::BeginServerTerrainLoad() {
	SpawnInitialZone();
    
    if (!bGenerateOnlySmallSpawnPoint) {
		TVoxelIndex B = GetZoneIndex(BeginServerTerrainLoadLocation);

        // async loading other zones
		TTerrainAreaLoadParams Params;
		Params.Radius = InitialLoadArea.Radius;
		Params.TerrainSizeMinZ = InitialLoadArea.TerrainSizeMinZ + B.Z;
		Params.TerrainSizeMaxZ = InitialLoadArea.TerrainSizeMaxZ + B.Z;

        AddAsyncTask([=]() {            
			UE_LOG(LogSandboxTerrain, Warning, TEXT("Server: Begin terrain load at location: %f %f %f"), BeginServerTerrainLoadLocation.X, BeginServerTerrainLoadLocation.Y, BeginServerTerrainLoadLocation.Z);

			TTerrainLoadHelper Loader(TEXT("Initial_Load_Task"), this, Params);
            Loader.LoadArea(BeginServerTerrainLoadLocation);

			GEngine->AddOnScreenDebugMessage(-1, 10, FColor::White, TEXT("Finish initial terrain load"));
			UE_LOG(LogSandboxTerrain, Warning, TEXT("======= Finish initial terrain load ======="));

			if (!bIsWorkFinished) {
				AsyncTask(ENamedThreads::GameThread, [&] {
					UE51MaterialIssueWorkaround();

					AddTaskToConveyor([=] { 
						OnFinishInitialLoad(); 
						if (bSaveAfterInitialLoad) {
							SaveMapAsync();
						}
					});

					AsyncTask(ENamedThreads::GameThread, [&] {
						StartPostLoadTimers();
						StartCheckArea();
					});
				});
			}
        });
	} else {
		OnFinishInitialLoad();
	}
}

void ASandboxTerrainController::BeginPlayClient() {
	LoadTerrainMetadata();

	TerrainClientComponent = NewObject<UTerrainClientComponent>(this, TEXT("TerrainClient"));
	TerrainClientComponent->RegisterComponent();
	TerrainClientComponent->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepRelativeTransform, NAME_None);
}

void ASandboxTerrainController::OnStartBackgroundSaveTerrain() {

}

void ASandboxTerrainController::OnFinishBackgroundSaveTerrain() {

}

void ASandboxTerrainController::OnProgressBackgroundSaveTerrain(float Progress) {

}

void ASandboxTerrainController::SaveMapAsync() {
	UE_LOG(LogSandboxTerrain, Log, TEXT("Start save terrain async"));
	AddAsyncTask([&]() {
		std::function<void(uint32, uint32)> OnProgress = [=](uint32 Processed, uint32 Total) {
			if (Processed % 10 == 0) {
				float Progress = (float)Processed / (float)Total;
				//UE_LOG(LogSandboxTerrain, Log, TEXT("Save terrain: %d / %d - %f%%"), Processed, Total, Progress * 100);
				AsyncTask(ENamedThreads::GameThread, [=]() { OnProgressBackgroundSaveTerrain(Progress); });
			}
		};

		AsyncTask(ENamedThreads::GameThread, [=]() { OnStartBackgroundSaveTerrain(); });
		Save(OnProgress);
		bForcePerformHardUnload = true;
		AsyncTask(ENamedThreads::GameThread, [=]() { OnFinishBackgroundSaveTerrain(); });

		UE_LOG(LogSandboxTerrain, Log, TEXT("Finish save terrain async"));
	});
}

void ASandboxTerrainController::AutoSaveByTimer() {
    UE_LOG(LogSandboxTerrain, Log, TEXT("Start auto save..."));
	SaveMapAsync();
}

//======================================================================================================================================================================
//  spawn zone
//======================================================================================================================================================================

uint32 ASandboxTerrainController::GetZoneVoxelResolution() {
	int L = LOD_ARRAY_SIZE - 1;
	int R = 1 << L;
	int RRR = R + 1;
	return RRR;
}

float ASandboxTerrainController::GetZoneSize() {
	return USBT_ZONE_SIZE;
}

TVoxelData* ASandboxTerrainController::NewVoxelData() {
	return new TVoxelData(GetZoneVoxelResolution(), GetZoneSize());
}

std::list<TChunkIndex> ASandboxTerrainController::MakeChunkListByAreaSize(const uint32 AreaRadius) {
	return ReverseSpiralWalkthrough(AreaRadius);
}

void ASandboxTerrainController::SpawnZone(const TVoxelIndex& Index, const TTerrainLodMask TerrainLodMask) {
	// voxel data must exist in this point
	TVoxelDataInfoPtr VoxelDataInfoPtr = GetVoxelDataInfo(Index);

	bool bMeshExist = false;
	auto ExistingZone = GetZoneByVectorIndex(Index);
	if (ExistingZone) {
		if (ExistingZone->GetTerrainLodMask() <= TerrainLodMask) {
			return;
		} else {
			bMeshExist = true;
		}
	}

	// if mesh data exist in file - load, apply and return
	TMeshDataPtr MeshDataPtr = nullptr;
	TInstanceMeshTypeMap& ZoneInstanceObjectMap = *TerrainData->GetOrCreateInstanceObjectMap(Index);
	LoadMeshAndObjectDataByIndex(Index, MeshDataPtr, ZoneInstanceObjectMap);
	if (MeshDataPtr && VoxelDataInfoPtr->DataState != TVoxelDataState::GENERATED) {
		if (bMeshExist) {
			// just change lod mask
			//TerrainData->PutMeshDataToCache(Index, MeshDataPtr);
			ExecGameThreadZoneApplyMesh(Index, ExistingZone, MeshDataPtr);
			return;
		} else {
			// spawn new zone with mesh
			//TerrainData->PutMeshDataToCache(Index, MeshDataPtr);
			ExecGameThreadAddZoneAndApplyMesh(Index, MeshDataPtr);
			return;
		}
	}
}

void ASandboxTerrainController::BatchGenerateZone(const TArray<TSpawnZoneParam>& GenerationList) {
	TArray<TGenerateZoneResult> NewVdArray;
	GetTerrainGenerator()->BatchGenerateVoxelTerrain(GenerationList, NewVdArray);

	int Idx = 0;
	for (const auto& P : GenerationList) {
		TVoxelDataInfoPtr VdInfoPtr = TerrainData->GetVoxelDataInfo(P.Index);
		TVdInfoLockGuard Lock(VdInfoPtr);

		VdInfoPtr->Vd = NewVdArray[Idx].Vd;
		FVector v = VdInfoPtr->Vd->getOrigin();

		if (NewVdArray[Idx].Method == TGenerationMethod::FastSimple || NewVdArray[Idx].Method == Skip) {
			//AsyncTask(ENamedThreads::GameThread, [=]() { DrawDebugBox(GetWorld(), v, FVector(USBT_ZONE_SIZE / 2), FColor(0, 0, 255, 100), true); });
			VdInfoPtr->DataState = TVoxelDataState::UNGENERATED;
		} else {
			VdInfoPtr->DataState = TVoxelDataState::GENERATED;
		}

		VdInfoPtr->SetChanged();
		TInstanceMeshTypeMap& ZoneInstanceObjectMap = *TerrainData->GetOrCreateInstanceObjectMap(P.Index);
		GeneratorComponent->GenerateInstanceObjects(P.Index, VdInfoPtr->Vd, ZoneInstanceObjectMap);
		Idx++;
	}
}

void ASandboxTerrainController::BatchSpawnZone(const TArray<TSpawnZoneParam>& SpawnZoneParamArray) {
	TArray<TSpawnZoneParam> GenerationList;
	TArray<TSpawnZoneParam> LoadList;

	for (const auto& SpawnZoneParam : SpawnZoneParamArray) {
		const TVoxelIndex Index = SpawnZoneParam.Index;
		const TTerrainLodMask TerrainLodMask = SpawnZoneParam.TerrainLodMask;

		bool bIsNoMesh = false;

		//check voxel data in memory
		bool bNewVdGeneration = false;

		{
			TVoxelDataInfoPtr VdInfoPtr = TerrainData->GetVoxelDataInfo(Index);
			TVdInfoLockGuard Lock(VdInfoPtr);

			if (VdInfoPtr->DataState == TVoxelDataState::UNDEFINED) {
				if (TdFile.isExist(Index)) {
					TValueDataPtr DataPtr = TdFile.loadData(Index);
					usbt::TFastUnsafeDeserializer Deserializer(DataPtr->data());
					TKvFileZodeData ZoneHeader;
					Deserializer >> ZoneHeader;

					bIsNoMesh = ZoneHeader.Is(TZoneFlag::NoMesh);
					bool bIsNoVd = ZoneHeader.Is(TZoneFlag::NoVoxelData);
					if (bIsNoVd) {
						//UE_LOG(LogSandboxTerrain, Log, TEXT("NoVoxelData"));
						VdInfoPtr->DataState = TVoxelDataState::UNGENERATED;
					}
					else {
						//voxel data exist in file
						VdInfoPtr->DataState = TVoxelDataState::READY_TO_LOAD;
					}
				}
				else {
					// generate new voxel data
					VdInfoPtr->DataState = TVoxelDataState::GENERATION_IN_PROGRESS;
					bNewVdGeneration = true;
				}
			}
		}

		if (bNewVdGeneration) {
			GenerationList.Add(SpawnZoneParam);
		} else {
			if (!bIsNoMesh) {
				LoadList.Add(SpawnZoneParam);
			}
		}
	}

	for (const auto& P  : LoadList) {
		SpawnZone(P.Index, P.TerrainLodMask);
	}

	if (GenerationList.Num() > 0) {
		BatchGenerateZone(GenerationList);
	}

	for (const auto& P : GenerationList) {
		TVoxelDataInfoPtr VoxelDataInfoPtr = GetVoxelDataInfo(P.Index);
		TVdInfoLockGuard Lock(VoxelDataInfoPtr);

		if (VoxelDataInfoPtr->Vd && VoxelDataInfoPtr->Vd->getDensityFillState() == TVoxelDataFillState::MIXED) {
			TMeshDataPtr MeshDataPtr = GenerateMesh(VoxelDataInfoPtr->Vd);
			VoxelDataInfoPtr->CleanUngenerated(); //TODO refactor
			TerrainData->PutMeshDataToCache(P.Index, MeshDataPtr);
			ExecGameThreadAddZoneAndApplyMesh(P.Index, MeshDataPtr, true);
		} else {
			VoxelDataInfoPtr->SetNeedTerrainSave();
			TerrainData->AddSaveIndex(P.Index);
		}
	}
}

bool ASandboxTerrainController::IsWorkFinished() { 
	return bIsWorkFinished; 
};

void ASandboxTerrainController::AddInitialZone(const TVoxelIndex& ZoneIndex) {
	InitialLoadSet.insert(ZoneIndex);
}

void ASandboxTerrainController::SpawnInitialZone() {
	const int s = static_cast<int>(TerrainInitialArea);

	if (s > 0) {
		for (auto z = InitialLoadArea.TerrainSizeMaxZ; z >= InitialLoadArea.TerrainSizeMinZ; z--) {
			for (auto x = -s; x <= s; x++) {
				for (auto y = -s; y <= s; y++) {
					AddInitialZone(TVoxelIndex(x, y, z));
				}
			}
		}
	} else {
		AddInitialZone(TVoxelIndex(0, 0, 0));
	}

	TArray<TSpawnZoneParam> SpawnList;
	for (const auto& ZoneIndex : InitialLoadSet) {
		TSpawnZoneParam SpawnZoneParam;
		SpawnZoneParam.Index = ZoneIndex;
		SpawnZoneParam.TerrainLodMask = 0;
		SpawnList.Add(SpawnZoneParam);
	}

	BatchSpawnZone(SpawnList);
}

// always in game thread
UTerrainZoneComponent* ASandboxTerrainController::AddTerrainZone(FVector Pos) {
	if (!IsInGameThread()) {
		return nullptr;
	}
    
    TVoxelIndex Index = GetZoneIndex(Pos);
	if (GetZoneByVectorIndex(Index)) {
		return nullptr; // no duplicate
	}

    FVector IndexTmp(Index.X, Index.Y,Index.Z);
    FString ZoneName = FString::Printf(TEXT("Zone [%.0f, %.0f, %.0f]"), IndexTmp.X, IndexTmp.Y, IndexTmp.Z);
    UTerrainZoneComponent* ZoneComponent = NewObject<UTerrainZoneComponent>(this, FName(*ZoneName));
    if (ZoneComponent) {
		ZoneComponent->SetIsReplicated(true);
        ZoneComponent->RegisterComponent();
        //ZoneComponent->SetRelativeLocation(pos);
        ZoneComponent->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepRelativeTransform, NAME_None);
        ZoneComponent->SetWorldLocation(Pos);
		ZoneComponent->SetMobility(EComponentMobility::Movable);

        FString TerrainMeshCompName = FString::Printf(TEXT("TerrainMesh [%.0f, %.0f, %.0f]"), IndexTmp.X, IndexTmp.Y, IndexTmp.Z);
        UVoxelMeshComponent* TerrainMeshComp = NewObject<UVoxelMeshComponent>(this, FName(*TerrainMeshCompName));
		TerrainMeshComp->SetIsReplicated(true);
        TerrainMeshComp->RegisterComponent();
        TerrainMeshComp->SetMobility(EComponentMobility::Movable);
        TerrainMeshComp->SetCanEverAffectNavigation(true);
        TerrainMeshComp->SetCollisionProfileName(TEXT("InvisibleWall"));
        TerrainMeshComp->AttachToComponent(ZoneComponent, FAttachmentTransformRules::KeepRelativeTransform, NAME_None);
        TerrainMeshComp->ZoneIndex = Index;

        ZoneComponent->MainTerrainMesh = TerrainMeshComp;
    }

    TerrainData->AddZone(Index, ZoneComponent);

	if (bShowZoneBounds) {
		DrawDebugBox(GetWorld(), Pos, FVector(USBT_ZONE_SIZE / 2), FColor(255, 0, 0, 100), true);
	}

    return ZoneComponent;
}

//======================================================================================================================================================================
//
//======================================================================================================================================================================

TVoxelIndex ASandboxTerrainController::GetZoneIndex(const FVector& Pos) {
	FVector Tmp = sandboxGridIndex(Pos, USBT_ZONE_SIZE);
	return TVoxelIndex(Tmp.X, Tmp.Y, Tmp.Z);
}

FVector ASandboxTerrainController::GetZonePos(const TVoxelIndex& Index) {
	return FVector((float)Index.X * USBT_ZONE_SIZE, (float)Index.Y * USBT_ZONE_SIZE, (float)Index.Z * USBT_ZONE_SIZE);
}

UTerrainZoneComponent* ASandboxTerrainController::GetZoneByVectorIndex(const TVoxelIndex& Index) {
	return TerrainData->GetZone(Index);
}

TVoxelDataInfoPtr ASandboxTerrainController::GetVoxelDataInfo(const TVoxelIndex& Index) {
    return TerrainData->GetVoxelDataInfo(Index);
}

//======================================================================================================================================================================
// invoke async
//======================================================================================================================================================================


void ASandboxTerrainController::InvokeSafe(std::function<void()> Function) {
    if (IsInGameThread()) {
        Function();
    } else {
        AsyncTask(ENamedThreads::GameThread, [=]() { Function(); });
    }
}

void ASandboxTerrainController::AddTaskToConveyor(std::function<void()> Function) {
	Conveyor->push(Function);
}

void ASandboxTerrainController::ExecGameThreadZoneApplyMesh(const TVoxelIndex& Index, UTerrainZoneComponent* Zone, TMeshDataPtr MeshDataPtr) {
	ASandboxTerrainController* Controller = this;

	std::function<void()> Function = [=]() {
		if (!bIsGameShutdown) {
			if (MeshDataPtr) {		
				TVoxelDataInfoPtr VdInfoPtr = TerrainData->GetVoxelDataInfo(Index);
				TVdInfoLockGuard Lock(VdInfoPtr);

				TerrainData->PutMeshDataToCache(Index, MeshDataPtr);

				Zone->ApplyTerrainMesh(MeshDataPtr, 0);
				VdInfoPtr->SetNeedTerrainSave();
				TerrainData->AddSaveIndex(Index);
			}
		} else {
			// TODO remove
			UE_LOG(LogSandboxTerrain, Log, TEXT("ASandboxTerrainController::ExecGameThreadZoneApplyMesh - game shutdown"));
		}
	};

	AddTaskToConveyor(Function);
}

//TODO move to conveyor
void ASandboxTerrainController::ExecGameThreadAddZoneAndApplyMesh(const TVoxelIndex& Index, TMeshDataPtr MeshDataPtr, const bool bIsNewGenerated, const bool bIsChanged) {
	FVector ZonePos = GetZonePos(Index);
	ASandboxTerrainController* Controller = this;

	std::function<void()> Function = [=]() {
		if (!bIsGameShutdown) {
			if (MeshDataPtr) {
				TVoxelDataInfoPtr VdInfoPtr = TerrainData->GetVoxelDataInfo(Index);
				TVdInfoLockGuard Lock(VdInfoPtr);
				TerrainData->PutMeshDataToCache(Index, MeshDataPtr);

				UTerrainZoneComponent* Zone = AddTerrainZone(ZonePos);
				if (Zone) {
					Zone->ApplyTerrainMesh(MeshDataPtr, 0);

					if (bIsChanged) {
						VdInfoPtr->SetNeedTerrainSave();
						TerrainData->AddSaveIndex(Index);
					}

					if (bIsNewGenerated) {
						OnGenerateNewZone(Index, Zone);
					} else {
						OnLoadZone(Index, Zone);
					}
				} 
			}
		} else {
			// TODO remove
			UE_LOG(LogSandboxTerrain, Warning, TEXT("ASandboxTerrainController::ExecGameThreadAddZoneAndApplyMesh - game shutdown"));
		}
	};

	//InvokeSafe(Function);
	AddTaskToConveyor(Function);
}

void ASandboxTerrainController::AddAsyncTask(std::function<void()> Function) {
	ThreadPool->addTask(Function);
}

//======================================================================================================================================================================
// events
//======================================================================================================================================================================

void ASandboxTerrainController::OnGenerateNewZone(const TVoxelIndex& Index, UTerrainZoneComponent* Zone) {
	TVoxelDataInfoPtr VdInfoPtr = TerrainData->GetVoxelDataInfo(Index);

    if (FoliageDataAsset) {
		TInstanceMeshTypeMap& ZoneInstanceObjectMap = *TerrainData->GetOrCreateInstanceObjectMap(Index);
		Zone->SpawnAll(ZoneInstanceObjectMap);
		VdInfoPtr->SetNeedObjectsSave();
    }

	OnFinishGenerateNewZone(Index);
	VdInfoPtr->ClearInstanceObjectMap();
	Zone->bIsSpawnFinished = true;
	VdInfoPtr->SetNeedTerrainSave();
	TerrainData->AddSaveIndex(Index);
}

void ASandboxTerrainController::OnLoadZone(const TVoxelIndex& Index, UTerrainZoneComponent* Zone) {
	TVoxelDataInfoPtr VdInfoPtr = TerrainData->GetVoxelDataInfo(Index);

	if (FoliageDataAsset) {
		TInstanceMeshTypeMap& ZoneInstanceObjectMap = *TerrainData->GetOrCreateInstanceObjectMap(Index);
		Zone->SpawnAll(ZoneInstanceObjectMap);
	}

	OnFinishLoadZone(Index);
	VdInfoPtr->ClearInstanceObjectMap();
	Zone->bIsSpawnFinished = true;
}

void ASandboxTerrainController::OnFinishAsyncPhysicsCook(const TVoxelIndex& ZoneIndex) {

}


FORCEINLINE float ASandboxTerrainController::ClcGroundLevel(const FVector& V) {
	//return Generator->GroundLevelFuncLocal(V);
	return 0;
}

//======================================================================================================================================================================
// virtual functions
//======================================================================================================================================================================


void ASandboxTerrainController::OnOverlapActorTerrainEdit(const FOverlapResult& OverlapResult, const FVector& Pos) {

}

void ASandboxTerrainController::OnFinishGenerateNewZone(const TVoxelIndex& Index) {

}

bool ASandboxTerrainController::OnZoneSoftUnload(const TVoxelIndex& ZoneIndex) {
	return true;
}

void ASandboxTerrainController::OnRestoreZoneSoftUnload(const TVoxelIndex& ZoneIndex) {

}

bool ASandboxTerrainController::OnZoneHardUnload(const TVoxelIndex& ZoneIndex) {
	return true;
}

void ASandboxTerrainController::OnFinishLoadZone(const TVoxelIndex& Index) {

}

void ASandboxTerrainController::OnFinishInitialLoad() {

}

//======================================================================================================================================================================
// Perlin noise according seed
//======================================================================================================================================================================

// TODO use seed
float ASandboxTerrainController::PerlinNoise(const FVector& Pos, const float PositionScale, const float ValueScale) const {
	if (GeneratorComponent) {
		return GeneratorComponent->PerlinNoise(Pos, PositionScale, ValueScale);
	}

	return 0;
}

// range 0..1
float ASandboxTerrainController::NormalizedPerlinNoise(const FVector& Pos, const float PositionScale, const float ValueScale) const {
	return (PerlinNoise(Pos, PositionScale, 1.f) + 0.87f) / 1.73f * ValueScale;
}

//======================================================================================================================================================================
// generate mesh
//======================================================================================================================================================================

std::shared_ptr<TMeshData> ASandboxTerrainController::GenerateMesh(TVoxelData* Vd) {
	double Start = FPlatformTime::Seconds();

	if (!Vd) {
		UE_LOG(LogSandboxTerrain, Log, TEXT("ASandboxTerrainController::GenerateMesh - NULL voxel data!"));
		return nullptr;
	}

	if (Vd->getDensityFillState() == TVoxelDataFillState::ZERO || Vd->getDensityFillState() == TVoxelDataFillState::FULL) {
		return nullptr;
	}

	TVoxelDataParam Vdp;

	if (USBT_ENABLE_LOD) {
		Vdp.bGenerateLOD = true;
		Vdp.collisionLOD = 0;
	} else {
		Vdp.bGenerateLOD = false;
		Vdp.collisionLOD = 0;
	}

	TMeshDataPtr MeshDataPtr = sandboxVoxelGenerateMesh(*Vd, Vdp);

	double End = FPlatformTime::Seconds();
	double Time = (End - Start) * 1000;
	MeshDataPtr->TimeStamp = End;

	//UE_LOG(LogSandboxTerrain, Log, TEXT("generateMesh -------------> %f %f %f --> %f ms"), Vd->getOrigin().X, Vd->getOrigin().Y, Vd->getOrigin().Z, Time);
	return MeshDataPtr;
}

const FSandboxFoliage& ASandboxTerrainController::GetFoliageById(uint32 FoliageId) const {
	return FoliageMap[FoliageId];
}

void ASandboxTerrainController::MarkZoneNeedsToSaveObjects(const TVoxelIndex& ZoneIndex) {
	//TODO check
	TVoxelDataInfoPtr VdInfoPtr = TerrainData->GetVoxelDataInfo(ZoneIndex);
	TVdInfoLockGuard Lock(VdInfoPtr);

	VdInfoPtr->SetChanged();
	VdInfoPtr->SetNeedObjectsSave();
	TerrainData->AddSaveIndex(ZoneIndex);
}

const FTerrainInstancedMeshType* ASandboxTerrainController::GetInstancedMeshType(uint32 MeshTypeId, uint32 MeshVariantId) const {
	uint64 MeshCode = FTerrainInstancedMeshType::ClcMeshTypeCode(MeshTypeId, MeshVariantId);
	const FTerrainInstancedMeshType* MeshType = InstMeshMap.Find(MeshCode);
	return MeshType;
}

float ASandboxTerrainController::GetGroundLevel(const FVector& Pos) {

	// TODO fixme on client side
	const UTerrainGeneratorComponent* Generator = GetTerrainGenerator();
	if (Generator) {
		const TVoxelIndex ZoneIndex = GetZoneIndex(Pos);
		return Generator->GroundLevelFunction(ZoneIndex, Pos);
	}

	return 0;
}

FTerrainDebugInfo ASandboxTerrainController::GetMemstat() {
	return FTerrainDebugInfo{ vd::tools::memory::getVdCount(), md_counter.load(), cd_counter.load(), (int)Conveyor->size(), ThreadPool->size()};
}

void ASandboxTerrainController::UE51MaterialIssueWorkaround() {
	double Start = FPlatformTime::Seconds();

	TArray<UTerrainZoneComponent*> Components;
	GetComponents<UTerrainZoneComponent>(Components);
	for (UTerrainZoneComponent* ZoneComponent : Components) {
		FVector ZonePos = ZoneComponent->GetComponentLocation();
		const TVoxelIndex ZoneIndex = GetZoneIndex(ZonePos);

		TVoxelDataInfoPtr VdInfoPtr = TerrainData->GetVoxelDataInfo(ZoneIndex);
		VdInfoPtr->Lock();

		auto MeshDataPtr = VdInfoPtr->GetMeshDataCache();
		if (MeshDataPtr != nullptr) {
			ZoneComponent->ApplyTerrainMesh(MeshDataPtr, true);
		}

		VdInfoPtr->Unlock();
	}

	double End = FPlatformTime::Seconds();
	double Time = (End - Start) * 1000;
	UE_LOG(LogSandboxTerrain, Log, TEXT("UE51MaterialIssueWorkaround --> %f ms"), Time);
}