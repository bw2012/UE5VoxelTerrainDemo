
#include "MiningTool.h"
#include "VoxelMeshComponent.h"
#include "TerrainZoneComponent.h"
#include "../MainPlayerController.h"
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
			MainController->ServerRpcDestroyActor(Index.X, Index.Y, Index.Z, Construction->GetName(), Hit.Location);
		}
	}

	ASandboxTerrainController* Terrain = Cast<ASandboxTerrainController>(Hit.GetActor());
	if (Terrain) {
		TVoxelIndex ZoneIndex = Terrain->GetZoneIndex(Hit.ImpactPoint);
		FVector ZoneIndexTmp(ZoneIndex.X, ZoneIndex.Y, ZoneIndex.Z);
		UVoxelMeshComponent* ZoneMesh = Cast<UVoxelMeshComponent>(Hit.Component.Get());
		if (ZoneMesh) {

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

		UTerrainInstancedStaticMesh* TerrainInstMesh = Cast<UTerrainInstancedStaticMesh>(Hit.Component.Get());
		if (TerrainInstMesh) {

			FTransform InstanceTransform;
			TerrainInstMesh->GetInstanceTransform(Hit.Item, InstanceTransform, true);

			if (TerrainInstMesh->IsFoliage()) {
				FVector Location = Hit.Normal * 25 + Hit.Location;

				TVoxelIndex Index = Terrain->GetZoneIndex(TerrainInstMesh->GetComponentLocation());
				MainController->ServerRpcDestroyTerrainMesh(Index.X, Index.Y, Index.Z, TerrainInstMesh->MeshTypeId, TerrainInstMesh->MeshVariantId, Hit.Item, 2, Hit.Location);
			}

			SpawnItems(MainController, TerrainInstMesh->MeshTypeId, InstanceTransform);
		}
	}
}

void AMiningTool::SpawnItems(APlayerController* Controller, uint32 MeshTypeId, const FTransform& InstanceTransform) {

	AMainPlayerController* MainController = (AMainPlayerController*)Controller;

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
			PlayerCharacter->CursorToWorld->SetVisibility(false);

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
			SetDecalParams(PlayerCharacter, Res, ToolSizeArray[DiggingToolSize]);
			return true;
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