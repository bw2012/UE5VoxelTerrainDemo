// Fill out your copyright notice in the Description page of Project Settings.

#include "TerrainController.h"
#include "Objects/BaseObject.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "TerrainZoneComponent.h"
#include "MainTerrainGeneratorComponent.h"
#include "NotificationHelper.h"


//======================================================================================================================================================================
// Terrain generator 
//======================================================================================================================================================================

UTerrainGeneratorComponent* ATerrainController::NewTerrainGenerator() {
	UMainTerrainGeneratorComponent* Generator = NewObject<UMainTerrainGeneratorComponent>(this, TEXT("TerrainGenerator"));
	UE_LOG(LogTemp, Warning, TEXT("NewTerrainGenerator -> %s"), *Generator->GetClass()->GetName());
	return Generator;
}

void ATerrainController::OnOverlapActorTerrainEdit(const FOverlapResult& OverlapResult, const FVector& Pos) {
	ASandboxObject* Object = Cast<ASandboxObject>(OverlapResult.GetActor());
	if (Object) {
		Object->OnTerrainChange();
	}
}

void ATerrainController::BeginPlay() {
	Super::BeginPlay();
}

void ATerrainController::BeginPlayServer() {
	if (LevelController) {
		LevelController->LoadMap();

		bool bFirst = true; // temp solution
		const TArray<FTempCharacterLoadInfo>& TempCharacterList = LevelController->GetTempCharacterList();
		for (const FTempCharacterLoadInfo& TempCharacterInfo : TempCharacterList) {
			const TVoxelIndex ZoneIndex = GetZoneIndex(TempCharacterInfo.Location);
			UE_LOG(LogTemp, Log, TEXT("Server PlayerId -> %s "), *TempCharacterInfo.SandboxPlayerUid);

			if (bFirst) {
				UE_LOG(LogTemp, Log, TEXT("BeginServerTerrainLoadLocation -> %f %f %f"), TempCharacterInfo.Location.X, TempCharacterInfo.Location.Y, TempCharacterInfo.Location.Z);
				BeginServerTerrainLoadLocation = TempCharacterInfo.Location;
				bFirst = false;
			}

			for (int X = -1; X <= 1; X++) {
				for (int Y = -1; Y <= 1; Y++) {
					for (int Z = -1; Z <= 1; Z++) {
						const TVoxelIndex TmpZoneIndex(ZoneIndex.X + X, ZoneIndex.Y + Y, ZoneIndex.Z + Z);
						UE_LOG(LogTemp, Log, TEXT("AddInitialZone -> %d %d %d"), TmpZoneIndex.X, TmpZoneIndex.Y, TmpZoneIndex.Z);
						AddInitialZone(TmpZoneIndex);
						SpawnFromStash(TmpZoneIndex);
					}
				}
			}
		}
	}

	Super::BeginPlayServer();

	if (LevelController) {
		LevelController->SpawnTempCharacterList();
	}
}

void ATerrainController::BeginPlayClient() {
	Super::BeginPlayClient();
}


void ATerrainController::EndPlay(const EEndPlayReason::Type EndPlayReason) {
	Super::EndPlay(EndPlayReason);

	if (GetNetMode() == NM_DedicatedServer) {
		if (LevelController) {
			UE_LOG(LogTemp, Warning, TEXT("Save dedicated server lavel"));
			LevelController->SaveMap();
		}
	}
}

void ATerrainController::ShutdownAndSaveMap() {
	ShutdownThreads();

	if (LevelController) {
		LevelController->SaveMap();
	}

	if (GetNetMode() == NM_Client) {
		AsyncTask(ENamedThreads::GameThread, [=]() { OnFinishSaveTerrain(); });
		return; // TODO save to client cache
	}

	UE_LOG(LogTemp, Log, TEXT("Start save terrain manual"));
	auto SaveFunction = [&]() {
		std::function<void(uint32, uint32)> OnProgress = [=](uint32 Processed, uint32 Total) {
			if (Processed == Total) {
				AsyncTask(ENamedThreads::GameThread, [=]() { OnProgressSaveTerrain(1.f); });
			} else if (Processed % 10 == 0) {
				float Progress = (float)Processed / (float)Total;
				//UE_LOG(LogTemp, Log, TEXT("Save terrain: %d / %d - %f%%"), Processed, Total, Progress);
				AsyncTask(ENamedThreads::GameThread, [=]() { OnProgressSaveTerrain(Progress); });
			}
		};

		std::function<void(uint32)> OnFinish = [=](uint32 Processed) {
			AsyncTask(ENamedThreads::GameThread, [=]() { OnFinishSaveTerrain(); });
		};

		Save(OnProgress, OnFinish);
	};

	FFunctionGraphTask::CreateAndDispatchWhenReady(MoveTemp(SaveFunction), TStatId(), nullptr, ENamedThreads::AnyBackgroundThreadNormalTask);
}

void ATerrainController::OnStartBackgroundSaveTerrain() {
	AsyncTask(ENamedThreads::GameThread, [=]() { 
		EventStartBackgroundSave();
	});
}

void ATerrainController::OnFinishBackgroundSaveTerrain() {
	AsyncTask(ENamedThreads::GameThread, [=]() {
		EventFinishBackgroundSave();
	});
}

void ATerrainController::OnProgressBackgroundSaveTerrain(float Progress) {
	AsyncTask(ENamedThreads::GameThread, [=]() {
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

			//UE_LOG(LogTemp, Warning, TEXT("ttt: %f"), Scale.Z);

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
			FString Name = Elem.Key;
			ASandboxObject* Object = Elem.Value;

			FSandboxObjectDescriptor SandboxObjectDescriptor = FSandboxObjectDescriptor::MakeObjDescriptor(Object);
			ZonePtr->Stash.Add(Name, SandboxObjectDescriptor);

			UE_LOG(LogTemp, Warning, TEXT("DestroyObject -> %s"), *Name);
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
		FString ObjectName = SandboxObject->GetName();

		if (ZoneObjects.WorldObjectMap.Contains(ObjectName)) {
			UE_LOG(LogTemp, Warning, TEXT("Duplicate object -> %s"), *ObjectName);
		}

		ZoneObjects.WorldObjectMap.Add(ObjectName, SandboxObject);
	}
}

void ATerrainController::AddToStash(const FSandboxObjectDescriptor& ObjDesc) {
	const FVector Pos = ObjDesc.Transform.GetLocation();
	const FQuat Quat = ObjDesc.Transform.GetRotation();
	FString Str = FString::FromInt(ObjDesc.ClassId) 
		+ FString("|") + FString::SanitizeFloat(Pos.X) 
		+ FString("|") + FString::SanitizeFloat(Pos.Y) 
		+ FString("|") + FString::SanitizeFloat(Pos.Z)
		+ FString("|") + FString::SanitizeFloat(Quat.W)
		+ FString("|") + FString::SanitizeFloat(Quat.X)
		+ FString("|") + FString::SanitizeFloat(Quat.Y)
		+ FString("|") + FString::SanitizeFloat(Quat.Z);
	const TVoxelIndex ZoneIndex = GetZoneIndex(Pos);
	auto& ZoneObjects = ObjectsByZoneMap.FindOrAdd(ZoneIndex);
	ZoneObjects.Stash.Add(Str, ObjDesc);
}


void ATerrainController::UnRegisterSandboxObject(ASandboxObject* SandboxObject) {
	if (SandboxObject) {
		const FVector ObjectPos = SandboxObject->GetActorLocation();
		const TVoxelIndex ZoneIndex = GetZoneIndex(ObjectPos);
		auto& ZoneObjects = ObjectsByZoneMap.FindOrAdd(ZoneIndex);
		FString ObjectName = SandboxObject->GetName();
		ZoneObjects.WorldObjectMap.Remove(ObjectName);
	}
}

ASandboxObject* ATerrainController::SpawnSandboxObject(const int32 ClassId, const FTransform& Transform) {
	if (IsInGameThread()) {
		TSubclassOf<ASandboxObject> Obj = LevelController->GetSandboxObjectByClassId(ClassId);
		if (Obj) {
			ASandboxObject* Object = (ASandboxObject*)GetWorld()->SpawnActor(Obj->ClassDefaultObject->GetClass(), &Transform);
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
			FString Name = Elem.Key;
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
}


TArray<FVector> ATerrainController::Test(FVector PlayerLocation, float Radius) {
	UE_LOG(LogTemp, Log, TEXT("ATerrainController::Test"));
	TArray<FVector> Result;

	double Start = FPlatformTime::Seconds();

	TArray<UTerrainZoneComponent*> Components;
	GetComponents<UTerrainZoneComponent>(Components);
	for (UTerrainZoneComponent* ZoneComponent : Components) {
		FVector ZonePos = ZoneComponent->GetComponentLocation();
		const TVoxelIndex ZoneIndex = GetZoneIndex(ZonePos);
		float ZoneDistance = FVector::Distance(ZonePos, PlayerLocation);
		if (ZoneDistance > Radius) {
			if (ZoneIndex.Z > -1) { // TODO landscape only
				//DrawDebugBox(GetWorld(), ZonePos, FVector(USBT_ZONE_SIZE / 2), FColor(255, 255, 255, 0), true);
				Result.Add(ZonePos);
			}
		} 
	}

	double End = FPlatformTime::Seconds();
	double Time = (End - Start) * 1000;
	//UE_LOG(LogSandboxTerrain, Log, TEXT("UnloadFarZones --> %f ms"), Time);

	return Result;
}

void ATerrainController::OnFinishInitialLoad() {
	TNotificationHelper::SendNotification("finish_init_map_load");
}