// Fill out your copyright notice in the Description page of Project Settings.

#include "TerrainController.h"
#include "Objects/BaseObject.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "TerrainZoneComponent.h"
#include "MainTerrainGeneratorComponent.h"
#include "NotificationHelper.h"
#include "TerrainZoneComponent.h"
#include "MainGameInstance.h"
#include "NotificationHelper.h"
#include "SandboxCharacter.h"
#include "Objects/BaseObject.h"

#include "AIController.h"



//======================================================================================================================================================================
// Terrain generator 
//======================================================================================================================================================================

UTerrainGeneratorComponent* ATerrainController::NewTerrainGenerator() {
	UMainTerrainGeneratorComponent* Generator = NewObject<UMainTerrainGeneratorComponent>(this, TEXT("TerrainGenerator"));
	return Generator;
}

void ATerrainController::OnOverlapActorTerrainEdit(const FOverlapResult& OverlapResult, const FVector& Pos) {
	ASandboxObject* Object = Cast<ASandboxObject>(OverlapResult.GetActor());
	if (Object) {
		Object->OnTerrainChange();
	}

	ISandboxCoreCharacter* SandboxCharacter = Cast<ISandboxCoreCharacter>(OverlapResult.GetActor());
	if (SandboxCharacter && SandboxCharacter->GetState() < 0) {
		OverlapResult.GetActor()->Destroy();
	}
}

void ATerrainController::BeginPlay() {
	Super::BeginPlay();
	TNotificationHelper::AddObserver(TCHAR_TO_UTF8(*GetName()), "finish_register_player", std::bind(&ATerrainController::OnFinishRegisterPlayer, this));
}

void ATerrainController::BeginPlayServer() {
	if (LevelController) {
		LevelController->LoadMap();

		bool bFirst = true; // temp solution
		const TArray<FCharacterLoadInfo>& TempCharacterList = LevelController->GetTempCharacterList();
		for (const FCharacterLoadInfo& TempCharacterInfo : TempCharacterList) {
			const TVoxelIndex ZoneIndex = GetZoneIndex(TempCharacterInfo.Location);
			UE_LOG(LogTemp, Log, TEXT("Server PlayerId -> %s "), *TempCharacterInfo.SandboxPlayerUid);

			if (bFirst) {
				UE_LOG(LogTemp, Log, TEXT("BeginServerTerrainLoadLocation -> %f %f %f"), TempCharacterInfo.Location.X, TempCharacterInfo.Location.Y, TempCharacterInfo.Location.Z);
				BeginServerTerrainLoadLocation = TempCharacterInfo.Location;
				bFirst = false;
			}

			for (int X = -1; X <= 1; X++) {
				for (int Y = -1; Y <= 1; Y++) {
					for (int Z = -5; Z <= 1; Z++) {
						const TVoxelIndex TmpZoneIndex(ZoneIndex.X + X, ZoneIndex.Y + Y, ZoneIndex.Z + Z);
						//UE_LOG(LogTemp, Log, TEXT("AddInitialZone -> %d %d %d"), TmpZoneIndex.X, TmpZoneIndex.Y, TmpZoneIndex.Z);
						AddInitialZone(TmpZoneIndex); 
						SpawnFromStash(TmpZoneIndex);
					}
				}
			}
		}
	}

	Super::BeginPlayServer();

	if (LevelController) {
		// потом доделаю
		// disconnect == character sleep
		//LevelController->SpawnTempCharacterList();
	}
}

void ATerrainController::BeginPlayClient() {
	Super::BeginPlayClient();
}

void ATerrainController::EndPlay(const EEndPlayReason::Type EndPlayReason) {
	Super::EndPlay(EndPlayReason);

	TNotificationHelper::RemoveObserver(TCHAR_TO_UTF8(*GetName()));

	if (GetNetMode() == NM_DedicatedServer) {
		if (LevelController) {
			UE_LOG(LogTemp, Warning, TEXT("Save dedicated server level"));
			LevelController->SaveMap();
		}
	}
}

void ATerrainController::ShutdownAndSaveMap() {
	UMainGameInstance* GI = Cast<UMainGameInstance>(UGameplayStatics::GetGameInstance(GetWorld()));
	if (GI) {
		GI->Mute();
	}

	ShutdownThreads();

	if (LevelController) {
		LevelController->SaveMap();
	}

	if (GetNetMode() == NM_Client) {
		//AsyncTask(ENamedThreads::GameThread, [=]() { OnFinishSaveTerrain(); });
		//return; // TODO save to client cache
	}

	UE_LOG(LogTemp, Log, TEXT("Start save terrain manual"));
	auto SaveFunction = [&]() {
		std::function<void(uint32, uint32)> OnProgress = [=, this](uint32 Processed, uint32 Total) {
			if (Processed == Total) {
				UMainGameInstance* GI = Cast<UMainGameInstance>(UGameplayStatics::GetGameInstance(GetWorld()));
				if (GI) {
					GI->SetProgressString(TEXT("100%"));
					GI->SetProgress(1.f);
				}

			} else if (Processed % 10 == 0) {
				UMainGameInstance* GI = Cast<UMainGameInstance>(UGameplayStatics::GetGameInstance(GetWorld()));
				if (GI) {
					float Progress = (float)Processed / (float)Total;
					GI->SetProgressString(FString::Printf(TEXT("%.0f%%"), Progress * 100));
					GI->SetProgress(Progress);
				}
			}
		};

		std::function<void(uint32)> OnFinish = [=, this](uint32 Processed) {
			UMainGameInstance* GI = Cast<UMainGameInstance>(UGameplayStatics::GetGameInstance(GetWorld()));
			if (GI) {
				GI->SetProgressString(TEXT("100%"));
				GI->SetProgress(1.f);
			}

			AsyncTask(ENamedThreads::GameThread, [=, this]() { OnFinishSaveTerrain(); });
		};

		Save(OnProgress, OnFinish);
	};

	FFunctionGraphTask::CreateAndDispatchWhenReady(MoveTemp(SaveFunction), TStatId(), nullptr, ENamedThreads::AnyBackgroundThreadNormalTask);
}

void ATerrainController::OnStartBackgroundSaveTerrain() {
	TNotificationHelper::SendNotification("start_background_save");

	AsyncTask(ENamedThreads::GameThread, [=, this]() {
		EventStartBackgroundSave();
	});
}

void ATerrainController::OnFinishBackgroundSaveTerrain() {
	UMainGameInstance* GI = Cast<UMainGameInstance>(UGameplayStatics::GetGameInstance(GetWorld()));
	if (GI) {
		GI->SetBkgProgressString(TEXT(""));
	}

	TNotificationHelper::SendNotification("finish_background_save");

	AsyncTask(ENamedThreads::GameThread, [=, this]() {
		EventFinishBackgroundSave();
	});
}

void ATerrainController::OnProgressBackgroundSaveTerrain(float Progress) {
	UMainGameInstance* GI = Cast<UMainGameInstance>(UGameplayStatics::GetGameInstance(GetWorld()));
	if (GI)	{
		GI->SetBkgProgressString(FString::Printf(TEXT("%.0f%%"), Progress * 100));
	}

	AsyncTask(ENamedThreads::GameThread, [=, this]() {
		EventProgressBackgroundSave(Progress);
	});
}

void ATerrainController::OnFinishGenerateNewZone(const TVoxelIndex& Index) {
	UMainTerrainGeneratorComponent* Generator = (UMainTerrainGeneratorComponent*)GetTerrainGenerator();
	Generator->GenerateZoneSandboxObject(Index);
}

void ATerrainController::MakeFlattenGrassTrace(const FHitResult& Overlap) {
	UTerrainInstancedStaticMesh* FoliageMesh = Cast<UTerrainInstancedStaticMesh>(Overlap.GetComponent());
	if (FoliageMesh) {
		//UE_LOG(LogTemp, Warning, TEXT("InstancedMesh: %s"), *InstancedMesh->GetName());
		//InstancedMesh->RemoveInstance(Overlap.Item);

		FSandboxFoliage FoliageType = GetFoliageById(FoliageMesh->MeshTypeId);
		if (FoliageType.Type == ESandboxFoliageType::Grass) {
			FTransform Transform;
			FoliageMesh->GetInstanceTransform(Overlap.Item, Transform, false);
			FVector Scale = Transform.GetScale3D();

			static const float m = 0.9;
			if (Scale.Z > m) {
				Scale.Z = m;
			}

			static const float s = 0.3;
			if (Scale.Z > s) {
				//Scale.Z -= 0.1;
				if (Scale.Z < s) {
					Scale.Z = s;
				}
			}

			Transform.SetScale3D(Scale);
			FoliageMesh->UpdateInstanceTransform(Overlap.Item, Transform, false);

			TArray<USceneComponent*> Parents;
			FoliageMesh->GetParentComponents(Parents);
			if (Parents.Num() > 0) {
				UTerrainZoneComponent* Zone = Cast<UTerrainZoneComponent>(Parents[0]);
				if (Zone) {
					FVector ZonePos = Zone->GetComponentLocation();
					TVoxelIndex ZoneIndex = GetZoneIndex(ZonePos);
					MarkZoneNeedsToSaveObjects(ZoneIndex);
				}
			}
		}
	} 
}

bool ATerrainController::OnZoneSoftUnload(const TVoxelIndex& ZoneIndex) {
	auto* ZonePtr = ObjectsByZoneMap.Find(ZoneIndex);
	if (ZonePtr) {
		for (auto& Elem : ZonePtr->WorldObjectMap) {
			uint64 NetUid = Elem.Key;
			ASandboxObject* Object = Elem.Value;

			FSandboxObjectDescriptor SandboxObjectDescriptor = FSandboxObjectDescriptor::MakeObjDescriptor(Object);
			ZonePtr->Stash.Add(NetUid, SandboxObjectDescriptor);

			//UE_LOG(LogTemp, Warning, TEXT("DestroyObject -> %s"), *Name);
			Object->Destroy();
		}

		ZonePtr->WorldObjectMap.Empty();
	}

	return true;
}

const TMap<TVoxelIndex, FSandboxObjectsByZone>& ATerrainController::GetObjectsByZoneMap() const {
	return ObjectsByZoneMap;
}

void ATerrainController::RegisterSandboxObject(ASandboxObject* SandboxObject) {
	if (SandboxObject) {
		const FVector ObjectPos = SandboxObject->GetActorLocation();
		const TVoxelIndex ZoneIndex = GetZoneIndex(ObjectPos);
		auto& ZoneObjects = ObjectsByZoneMap.FindOrAdd(ZoneIndex);
		uint64 NetUid = SandboxObject->GetSandboxNetUid();

		if (ZoneObjects.WorldObjectMap.Contains(NetUid)) {
			UE_LOG(LogTemp, Warning, TEXT("Duplicate object -> %s - %llu"), *SandboxObject->GetName(), NetUid);
		}

		ZoneObjects.WorldObjectMap.Add(NetUid, SandboxObject);

		ABaseObject* BObj = Cast<ABaseObject>(SandboxObject);
		if (BObj && BObj->IsZoneAnchor()) {
			ZoneAnchorsMap.Add(SandboxObject->GetName(), BObj);

		}
	}
}

void ATerrainController::AddToStash(const FSandboxObjectDescriptor& ObjDesc) {
	const FVector Pos = ObjDesc.Transform.GetLocation();
	const TVoxelIndex ZoneIndex = GetZoneIndex(Pos);
	auto& ZoneObjects = ObjectsByZoneMap.FindOrAdd(ZoneIndex);
	ZoneObjects.Stash.Add(ObjDesc.NetUid, ObjDesc);
}

void ATerrainController::UnRegisterSandboxObject(ASandboxObject* SandboxObject) {
	if (SandboxObject) {
		const FVector ObjectPos = SandboxObject->GetActorLocation();
		const TVoxelIndex ZoneIndex = GetZoneIndex(ObjectPos);
		auto& ZoneObjects = ObjectsByZoneMap.FindOrAdd(ZoneIndex);
		FString ObjectName = SandboxObject->GetName();
		ZoneObjects.WorldObjectMap.Remove(SandboxObject->GetSandboxNetUid());

		ABaseObject* BObj = Cast<ABaseObject>(SandboxObject);
		if (BObj) {
			ZoneAnchorsMap.Remove(ObjectName);

		}
	}
}

ASandboxObject* ATerrainController::SpawnSandboxObject(const int32 ClassId, const FTransform& Transform) {
	if (IsInGameThread()) {
		TSubclassOf<ASandboxObject> Obj = LevelController->GetSandboxObjectByClassId(ClassId);
		if (Obj) {
			ASandboxObject* Object = LevelController->SpawnSandboxObject(ClassId, Transform);
			RegisterSandboxObject(Object);
			return Object;
		}
	} else {
		// TODO warning
	}

	return nullptr;
}

void ATerrainController::SpawnFromStash(const TVoxelIndex& ZoneIndex) {
	auto* ZonePtr = ObjectsByZoneMap.Find(ZoneIndex);
	if (ZonePtr) {
		for (auto& Elem : ZonePtr->Stash) {
			//uint64 NetUid = Elem.Key;
			const FSandboxObjectDescriptor& ObjDesc = Elem.Value;
			if (LevelController) {
				ASandboxObject* Obj = LevelController->SpawnPreparedObject(ObjDesc);
			}
		}

		ZonePtr->Stash.Empty();
	}
}

void ATerrainController::OnRestoreZoneSoftUnload(const TVoxelIndex& ZoneIndex) {
	SpawnFromStash(ZoneIndex);
}

void ATerrainController::OnFinishLoadZone(const TVoxelIndex& Index) {
	SpawnFromStash(Index);

	if (LevelController) {
		LevelController->SpawnSavedZoneNPC(Index);
	}
}

void ATerrainController::OnFinishInitialLoad() {
	TNotificationHelper::SendNotification("finish_init_map_load");

	UMainGameInstance* GI = Cast<UMainGameInstance>(UGameplayStatics::GetGameInstance(GetWorld()));
	if (GI) {
		GI->Unmute();
	}
}

void ATerrainController::OnDestroyInstanceMesh(UTerrainInstancedStaticMesh* InstancedMeshComp, int32 ItemIndex) {
	//UE_LOG(LogTemp, Log, TEXT("OnDestroyInstanceMesh --> %d"), InstancedMeshComp->MeshTypeId);
	const auto* ObjInfo = GetInstanceObjStaticInfo(InstancedMeshComp->MeshTypeId);

	if (ObjInfo && ObjInfo->bMiningDestroy && ObjInfo->SandboxObjectId) {
		FTransform Transform;
		InstancedMeshComp->GetInstanceTransform(ItemIndex, Transform, true);
		//LevelController->SpawnSandboxObject(ObjInfo->SandboxObjectId, Transform);
	}
}

void ATerrainController::OnFinishRegisterPlayer() {
	if (GetNetMode() == NM_Client) {
		ClientStart();
	}
}

void ATerrainController::GetAnchorObjectsLocation(TArray<FVector>& List) const {
	UE_LOG(LogTemp, Log, TEXT("GetAnchorObjectsLocation:"));
	for (const auto& Itm : ZoneAnchorsMap) {
		List.Add(Itm.Value->GetActorLocation());
		UE_LOG(LogTemp, Log, TEXT("ZoneAnchor: %s"), *Itm.Key);
	}

	if (LevelController) {
		TArray<ACharacter*> NpcList = LevelController->GetNpcList();

		for (const auto& Itm : NpcList) {
			if (Cast<AAIController>(Itm->GetController())) {
				List.Add(Itm->GetActorLocation());
				UE_LOG(LogTemp, Log, TEXT("NPC: %s"), *Itm->GetName());
			}
		}
	}
}

const FTerrainObjectInfo* ATerrainController::GetInstanceObjStaticInfo(const uint32 InstanceMeshTypeId) const {
	if (ObjectStaticData) {
		return ObjectStaticData->FindRow<FTerrainObjectInfo>(*FString::FromInt(InstanceMeshTypeId), "", false);
	}

	return nullptr;
}