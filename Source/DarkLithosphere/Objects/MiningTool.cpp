
#include "MiningTool.h"
#include "VoxelMeshComponent.h"
#include "TerrainZoneComponent.h"
#include "ConstructionObject.h"

#define Max_Size 3

#define Dig_Cube_Size 110 //60
#define Dig_Snap_To_Grid 100 //50


static float ToolSizeArray[] = {100, 150, 200};

FVector SnapToGrid(const FVector& Location, const float GridRange) {
	FVector Tmp(Location);
	Tmp /= GridRange;
	Tmp.Set(std::round(Tmp.X), std::round(Tmp.Y), std::round(Tmp.Z));
	Tmp *= GridRange;
	return FVector((int)Tmp.X, (int)Tmp.Y, (int)Tmp.Z);
}

AMiningTool::AMiningTool() {
	DiggingToolMode = 0;
}

int AMiningTool::GetSandboxTypeId() const {
	return SandboxType_Tool;
}

void AMiningTool::ToggleToolMode() {
	DiggingToolMode++;
	DiggingToolMode = DiggingToolMode % 2;
};

void AMiningTool::DigSmall(const FHitResult& Hit, ATerrainController* Terrain, AMainPlayerController* MainController) {
	TVoxelIndex ZoneIndex = Terrain->GetZoneIndex(Hit.ImpactPoint);

	const float Radius = 60;
	const float F = 0;
	const FVector P = Hit.Normal * Radius * F + Hit.Location;
	const FVector EffectLocation = Hit.Normal * 50 + Hit.Location;

	MainController->ServerRpcDigTerrain(0, P, EffectLocation, Radius, ZoneIndex.X, ZoneIndex.Y, ZoneIndex.Z, Hit.FaceIndex);
}


void AMiningTool::Dig(const FHitResult& Hit, ATerrainController* Terrain, AMainPlayerController* MainController) {
	TVoxelIndex ZoneIndex = Terrain->GetZoneIndex(Hit.ImpactPoint);

	if (DiggingToolMode == 0) {
		const float Radius = ToolSizeArray[DiggingToolSize];
		const float F = 0;
		const FVector P = Hit.Normal * Radius * F + Hit.Location;
		const FVector EffectLocation = Hit.Normal * 50 + Hit.Location;

		MainController->ServerRpcDigTerrain(DiggingToolMode, P, EffectLocation, Radius, ZoneIndex.X, ZoneIndex.Y, ZoneIndex.Z, Hit.FaceIndex);
	}

	if (DiggingToolMode == 1) {
		FVector P = SnapToGrid(Hit.Location, Dig_Snap_To_Grid);
		FVector EffectLocation = Hit.Normal * 50 + P;
		MainController->ServerRpcDigTerrain(DiggingToolMode, P, EffectLocation, Dig_Cube_Size, ZoneIndex.X, ZoneIndex.Y, ZoneIndex.Z, Hit.FaceIndex);
	}
}

void SpawnWoods(AMainPlayerController* MainController, const FVector& Location) {
	FRotator Rotation(0, 0, 90);

	FVector Pos = Location;
	Pos.Z += 250;
	FTransform Transform(Rotation, Pos, FVector(1));

	MainController->ServerRpcSpawnObject(150, Transform, true);

	const auto Num = FMath::RandRange(1, 3);
	for (int I = 1; I < Num; I++) {
		Pos.Z += 201;
		MainController->ServerRpcSpawnObject(12, FTransform(Rotation, Pos, FVector(1)), true);
	}
}

void SpawnStones(AMainPlayerController* MainController, const FVector& Location, uint16 MatId) {
	FVector Pos = Location;

	UE_LOG(LogTemp, Warning, TEXT("MatId: %d"), MatId);

	if (MatId == 1 || MatId == 2 || MatId == 0) { // hardcoded dirt
		const auto Chance = FMath::RandRange(0.f, 1.f);
		if (Chance < 0.4f) {
			const static int ObjectIds[2] = { 9, 8 };
			const auto Angle = FMath::RandRange(0.f, 359.f);
			const auto ObjectId = FMath::RandRange(0, 1);
			FRotator Rotation(0, Angle, 0);
			MainController->ServerRpcSpawnObject(ObjectIds[ObjectId], FTransform(Rotation, Pos, FVector(1)), true);
		}
	}

	if (MatId == 4) { // hardcoded stone (basalt)
		const auto Chance = FMath::RandRange(0.f, 1.f);
		if (Chance < 0.4f) {
			const auto Angle = FMath::RandRange(0.f, 359.f);
			const auto ObjectId = 25;
			FRotator Rotation(0, Angle, 0);
			MainController->ServerRpcSpawnObject(ObjectId, FTransform(Rotation, Pos, FVector(1)), true);
		}
	}

}

void AMiningTool::OnAltAction(const FHitResult& Hit, ABaseCharacter* PlayerCharacter) {
	UWorld* World = PlayerCharacter->GetWorld();
	ALevelController* LevelController = nullptr;

	AMainPlayerController* MainController = Cast<AMainPlayerController>(PlayerCharacter->GetController());
	if (MainController) {
		LevelController = MainController->GetLevelController();
	} else {
		return;
	}

	AConstructionObject* Construction = Cast<AConstructionObject>(Hit.GetActor());
	if (Construction) {
		if (LevelController && LevelController->TerrainController) {
			FVector Location = Construction->GetActorLocation();
			TVoxelIndex Index = LevelController->TerrainController->GetZoneIndex(Construction->GetActorLocation());
			//MainController->ServerRpcDestroyActor(Index.X, Index.Y, Index.Z, Construction->GetName(), Hit.Location, 2);
			MainController->ServerRpcDestroyActorByNetUid(Construction->GetSandboxNetUid(), Hit.Location, 2);
			return;
		}
	}

	ATerrainController* Terrain = Cast<ATerrainController>(Hit.GetActor());
	if (Terrain) {
		TVoxelIndex ZoneIndex = Terrain->GetZoneIndex(Hit.ImpactPoint);
		FVector ZoneIndexTmp(ZoneIndex.X, ZoneIndex.Y, ZoneIndex.Z);
		UVoxelMeshComponent* ZoneMesh = Cast<UVoxelMeshComponent>(Hit.Component.Get());
		if (ZoneMesh) {
			Dig(Hit, Terrain, MainController);

			uint16 MatId = ZoneMesh->GetMaterialIdFromCollisionFaceIndex(Hit.FaceIndex);
			SpawnStones(MainController, Hit.Location, MatId);
		}

		UTerrainInstancedStaticMesh* TerrainInstMesh = Cast<UTerrainInstancedStaticMesh>(Hit.Component.Get());
		if (TerrainInstMesh) {

			FTransform InstanceTransform;
			TerrainInstMesh->GetInstanceTransform(Hit.Item, InstanceTransform, true);

			if (TerrainInstMesh->IsFoliage()) {

				//TODO refactor. create ServerRpcDestroyTerrainMeshAndSpawnObject
				TVoxelIndex Index = Terrain->GetZoneIndex(TerrainInstMesh->GetComponentLocation());
				MainController->ServerRpcDestroyTerrainMesh(Index.X, Index.Y, Index.Z, TerrainInstMesh->MeshTypeId, TerrainInstMesh->MeshVariantId, Hit.Item, 2, Hit.Location);

				auto FoliageType = Terrain->GetFoliageById(TerrainInstMesh->MeshTypeId);
				if (FoliageType.Type == ESandboxFoliageType::Tree) {
					SpawnWoods(MainController, InstanceTransform.GetLocation());
				}

				return;
			}

			const auto* ObjInfo = Terrain->GetInstanceObjStaticInfo(TerrainInstMesh->MeshTypeId);
			if (ObjInfo && ObjInfo->bMiningDestroy) {

				DigSmall(Hit, Terrain, MainController);

				//TODO refactor. create ServerRpcDestroyTerrainMeshAndSpawnObject
				TVoxelIndex Index = Terrain->GetZoneIndex(TerrainInstMesh->GetComponentLocation());
				MainController->ServerRpcDestroyTerrainMesh(Index.X, Index.Y, Index.Z, TerrainInstMesh->MeshTypeId, TerrainInstMesh->MeshVariantId, Hit.Item, ObjInfo->MiningCrashEffectId, Hit.Location);

				FTransform NewTransform(FRotator(0, 0, 33), Hit.Location, FVector(1));
				SpawnItems(MainController, ObjInfo, NewTransform);
				return;
			}
		}
	}

	ASandboxObject* Obj = Cast<ASandboxObject>(Hit.GetActor());
	if (Obj) {
		if (LevelController && LevelController->TerrainController) {
			const auto* ObjData = LevelController->GetSandboxObjectStaticData(Obj->GetSandboxClassId());

			if (ObjData && ObjData->bMiningDestroy) {
				FVector Location = Obj->GetActorLocation();
				TVoxelIndex Index = LevelController->TerrainController->GetZoneIndex(Obj->GetActorLocation());
				uint32 EffectId = (ObjData->MiningCrashEffectId > 0) ? ObjData->MiningCrashEffectId : ObjData->CrashEffectId;
				MainController->ServerRpcDestroyActorByNetUid(Obj->GetSandboxNetUid(), Hit.Location, EffectId);
				return;
			}
		}
	}
}

void AMiningTool::SpawnItems(AMainPlayerController* MainController, const FTerrainObjectInfo* ObjInfo, const FTransform& InstanceTransform) {
	if (ObjInfo->MiningMode == FTerrainObjectMiningMode::Inventory && ObjInfo->SandboxObjectId > 0) {
		FRotator Rotation(0, 0, 90);
		FVector Pos = InstanceTransform.GetLocation();
		Pos.Z += 250;
		FTransform NewTransform(Rotation, Pos, FVector(1));
		MainController->ServerRpcAddItemOrSpawnObject(ObjInfo->SandboxObjectId, NewTransform);
	}

	/*
	if (MeshTypeId == 100 || MeshTypeId == 101) {

		FRotator Rotation(0, 0, 90);
		FVector Pos = InstanceTransform.GetLocation();
		Pos.Z += 250;
		FTransform NewTransform(Rotation, Pos, FVector(1));

		MainController->ServerRpcSpawnObject(150, NewTransform, true);

		const auto Num = FMath::RandRange(1, 3);
		for (int I = 1; I < Num; I++) {
			Pos.Z += 201;
			MainController->ServerRpcSpawnObject(12, FTransform(Rotation, Pos, FVector(1)), true);
		}
	}
	*/
}

void SetDecalParams(ABaseCharacter* PlayerCharacter, const FHitResult& Res, float Radius) {
	const FVector CursorFV = Res.ImpactNormal;
	const FRotator CursorR = CursorFV.Rotation();
	PlayerCharacter->ResetCursorMesh();
	PlayerCharacter->CursorToWorld->SetWorldLocation(Res.Location);
	PlayerCharacter->CursorToWorld->SetWorldRotation(CursorR);
	PlayerCharacter->CursorToWorld->DecalSize = FVector(100.f, Radius, Radius);
	PlayerCharacter->CursorToWorld->SetVisibility(true);
}

bool AMiningTool::OnTracePlayerActionPoint(const FHitResult& Res, ABaseCharacter* PlayerCharacter) {
	UWorld* World = PlayerCharacter->GetWorld();
	ASandboxTerrainController* TerrainController = Cast<ASandboxTerrainController>(Res.GetActor());
	if (TerrainController) {
		auto* Component = Res.GetComponent();
		FString ComponentName = Component->GetClass()->GetName();

		if (ComponentName == "VoxelMeshComponent") {
			PlayerCharacter->CursorToWorld->SetVisibility(true);

			if (DiggingToolMode == 0) {
				const float Radius = ToolSizeArray[DiggingToolSize];
				FVector MiningPos = Res.Location;
				FVector PlayerPos = PlayerCharacter->GetActorLocation();
				const float F = Radius / 100.f * 2.f;
				PlayerCharacter->SetCursorMesh(Sphere, CursorMaterial, Res.Location, FRotator(0), FVector(F));
				//DrawDebugSphere(World, Res.Location, Radius, 16, FColor(255, 255, 255, 100));
			}

			if (DiggingToolMode == 1) {
				const FVector Location = SnapToGrid(Res.Location, Dig_Snap_To_Grid);
				const float F = Dig_Cube_Size / 100.f * 2.f;
				PlayerCharacter->SetCursorMesh(Cube, CursorMaterial, Location, FRotator(0), FVector(F));
				//DrawDebugBox(World, Location, FVector(Dig_Cube_Size), FColor(255, 255, 255, 100));
			}

			return true;
		}

		if (ComponentName == "TerrainInstancedStaticMesh") {
			return false;
		}
	}

	AConstructionObject* Construction = Cast<AConstructionObject>(Res.GetActor());
	if (Construction) {
		SetDecalParams(PlayerCharacter, Res, ToolSizeArray[DiggingToolSize]);
		return true;
	}

	PlayerCharacter->ResetCursorMesh();
	PlayerCharacter->CursorToWorld->SetVisibility(false);

	return false;
}

bool AMiningTool::VisibleInHand(FTransform& Transform) {
	return true;
}

void AMiningTool::SwitchUp() {
	DiggingToolSize++;
	if (DiggingToolSize > Max_Size - 1) {
		DiggingToolSize = Max_Size - 1;
	}
}

void AMiningTool::SwitchDown() {
	DiggingToolSize--;
	if (DiggingToolSize < 0) {
		DiggingToolSize = 0;
	}
}