
#include "MiningTool.h"
#include "VoxelMeshComponent.h"
#include "TerrainZoneComponent.h"
#include "../MainPlayerController.h"

#define Max_Size 3

#define Dig_Sphere_Size 80 //legacy

#define Dig_Cube_Size 110 //60
#define Dig_Snap_To_Grid 100 //50

#define Dig_Cylinder_Length 100
#define Dig_Cylinder_Radius 100

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
	bShowEffects = false;
	bSpawnStones = false;
}

int AMiningTool::GetSandboxTypeId() const {
	return 100;
}

void AMiningTool::ToggleToolMode() {
	DiggingToolMode++;
	DiggingToolMode = DiggingToolMode % 2;
};

void AMiningTool::PlaySound(const FSandboxTerrainMaterial& MatInfo, const FVector& Location, UWorld* World) {
	if (this->DiggingRockSound && MatInfo.Type == FSandboxTerrainMaterialType::Rock) {
		UGameplayStatics::PlaySoundAtLocation(World, this->DiggingRockSound, Location, FRotator(0));
	}

	if (this->DiggingSoilSound && MatInfo.Type == FSandboxTerrainMaterialType::Soil) {
		UGameplayStatics::PlaySoundAtLocation(World, this->DiggingSoilSound, Location, FRotator(0));
	}
}

void SpawnWoods(ALevelController* LevelController, const FVector& Location) {
	FRotator Rotation(0, 0, 90);

	FVector Pos = Location;
	Pos.Z += 250;

	ASandboxObject* WoodLogBig = LevelController->SpawnSandboxObject(150, FTransform(Rotation, Pos, FVector(1)));
	if (WoodLogBig) {
		WoodLogBig->SandboxRootMesh->SetSimulatePhysics(true);
	}

	const auto Num = FMath::RandRange(1, 3);
	for (int I = 1; I < Num; I++) {
		Pos.Z += 201;
		ASandboxObject* WoodLog = LevelController->SpawnSandboxObject(12, FTransform(Rotation, Pos, FVector(1)));
		if (WoodLog) {
			WoodLog->SandboxRootMesh->SetSimulatePhysics(true);
		}
	}
}

void SpawnStones(ALevelController* LevelController, const FVector& Location, uint16 MatId, int Min, int Max) {
	FVector Pos = Location;

	if (MatId == 1) { // hardcoded dirt
		const auto Chance = FMath::RandRange(0.f, 1.f);
		if (Chance < 0.4f) {
			const static int ObjectIds[2] = {9, 8};
			const auto Num = FMath::RandRange(Min, Max);
			for (int I = 1; I < Num; I++) {
				const auto Angle = FMath::RandRange(0.f, 359.f);
				const auto ObjectId = FMath::RandRange(0, 1);
				FRotator Rotation(0, Angle, 0);
				ASandboxObject* Stone = LevelController->SpawnSandboxObject(ObjectIds[ObjectId], FTransform(Rotation, Pos, FVector(1)));
				if (Stone) {
					Stone->SandboxRootMesh->SetSimulatePhysics(true);
				}
			}
		}
	}

	if (MatId == 4) { // hardcoded stone (basalt)
		const auto Chance = FMath::RandRange(0.f, 1.f);
		if (Chance < 0.4f) {
			const auto Angle = FMath::RandRange(0.f, 359.f);
			const auto ObjectId = 25;
			FRotator Rotation(0, Angle, 0);
			ASandboxObject* Stone = LevelController->SpawnSandboxObject(ObjectId, FTransform(Rotation, Pos, FVector(1)));
			if (Stone) {
				Stone->SandboxRootMesh->SetSimulatePhysics(true);
			}
		}
	}

	if (MatId == 6) { // hardcoded iron ore
		const auto Chance = FMath::RandRange(0.f, 1.f);
		if (Chance < 0.9f) {
			const auto Num = FMath::RandRange(1, 3);
			for (int I = 1; I < Num; I++) {
				const auto Angle = FMath::RandRange(0.f, 359.f);
				const auto ObjectId = 22;
				FRotator Rotation(0, Angle, 0);
				ASandboxObject* Stone = LevelController->SpawnSandboxObject(ObjectId, FTransform(Rotation, Pos, FVector(1)));
				if (Stone) {
					Stone->SandboxRootMesh->SetSimulatePhysics(true);
				}
			}
		}
	}
}

void AMiningTool::OnAltAction(const FHitResult& Hit, ABaseCharacter* PlayerCharacter) {
	UWorld* World = PlayerCharacter->GetWorld();
	ASandboxTerrainController* Terrain = Cast<ASandboxTerrainController>(Hit.GetActor());
	ALevelController* LevelController = nullptr;
	AMainPlayerController* MainController = Cast<AMainPlayerController>(PlayerCharacter->GetController());
	if (MainController) {
		LevelController = MainController->GetLevelController();
	}

	if (Terrain) {
		TVoxelIndex ZoneIndex = Terrain->GetZoneIndex(Hit.ImpactPoint);
		FVector ZoneIndexTmp(ZoneIndex.X, ZoneIndex.Y, ZoneIndex.Z);
		UVoxelMeshComponent* ZoneMesh = Cast<UVoxelMeshComponent>(Hit.Component.Get());
		if (ZoneMesh) {
			uint16 MatId = ZoneMesh->GetMaterialIdFromCollisionFaceIndex(Hit.FaceIndex);
			UE_LOG(LogTemp, Warning, TEXT("MatId -> %d"), MatId);

			if (MatId > 0) {
				FSandboxTerrainMaterial MatInfo;
				if (Terrain->GetTerrainMaterialInfoById(MatId, MatInfo)) {
					PlaySound(MatInfo, Hit.Location, World);
				}
			}

			//UE_LOG(LogTemp, Warning, TEXT("zIndex -> %f %f %f"), ZoneIndexTmp.X, ZoneIndexTmp.Y, ZoneIndexTmp.Z);
			if (DiggingToolMode == 0) {
				// Strength / Mat.RockHardness > 0.1
				const float Radius = ToolSizeArray[DiggingToolSize];

				const float F = 0;
				const FVector P = Hit.Normal * Radius * F + Hit.Location;
				//DrawDebugSphere(World, P, Radius, 24, FColor(255, 255, 255, 100));

				Terrain->DigTerrainRoundHole(P, Radius);

				FVector Location = Hit.Normal * 50 + Hit.Location;

				if (EffectActor && bShowEffects) {
					//DrawDebugPoint(PlayerCharacter->GetWorld(), Location, 5.f, FColor(255, 0, 0, 0), false, 1);
					FRotator Rotation(0, 0, 0);
					World->SpawnActor(EffectActor, &Location, &Rotation);
				}

				if (LevelController && bSpawnStones) {
					SpawnStones(LevelController, Location, MatId, 1, 2);
				}
			}

			if (DiggingToolMode == 1) {
				FVector Location = SnapToGrid(Hit.Location, Dig_Snap_To_Grid);
				//FRotator Rotation2(-26.5f, 0, 0);
				//FRotator Rotation2(0, -26.5f, 0);
				//FRotator Rotation2(0, 0, 0);
				//float test = Dig_Cube_Size*2;
				//FVector Min(-test);
				//FVector Max(test);
				//FBox Box(Min, Max);
				//Terrain->DigTerrainCubeHole(Location, Box, 100.f, Rotation2); // FIXME: why inverse?

				Terrain->DigTerrainCubeHole(Location, Dig_Cube_Size);

				if (EffectActor && bShowEffects) {
					FVector Location2 = Hit.Normal * 50 + Location;
					//DrawDebugPoint(PlayerCharacter->GetWorld(), Location, 5.f, FColor(255, 0, 0, 0), false, 1);
					FRotator Rotation(0, 0, 0);
					World->SpawnActor(EffectActor, &Location2, &Rotation);
				}

				if (LevelController && bSpawnStones) {
					SpawnStones(LevelController, Hit.Location, MatId, 2, 4);
				}
			}
		}

		UTerrainInstancedStaticMesh* TerrainInstMesh = Cast<UTerrainInstancedStaticMesh>(Hit.Component.Get());
		if (TerrainInstMesh) {
			if (TerrainInstMesh->IsFoliage()) {
				FVector Location = Hit.Normal * 25 + Hit.Location;

				if (EffectActorWood) {
					FRotator Rotation(0, 0, 0);
					World->SpawnActor(EffectActorWood, &Location, &Rotation);
				}

				if (HitWoodSound) {
					UGameplayStatics::PlaySoundAtLocation(World, this->HitWoodSound, Location, FRotator(0));
				}

				FTransform InstanceTransform;
				TerrainInstMesh->GetInstanceTransform(Hit.Item, InstanceTransform, true);
				Terrain->RemoveInstanceAtMesh(TerrainInstMesh, Hit.Item);

				if (LevelController) {
					SpawnWoods(LevelController, InstanceTransform.GetLocation());
				}
			}
		}
	}
}

bool AMiningTool::OnTracePlayerActionPoint(const FHitResult& Res, ABaseCharacter* PlayerCharacter) {
	UWorld* World = PlayerCharacter->GetWorld();
	ASandboxTerrainController* TerrainController = Cast<ASandboxTerrainController>(Res.GetActor());
	if (TerrainController) {
		auto* Component = Res.GetComponent();
		FString ComponentName = Component->GetClass()->GetName();

		//if (ComponentName == "VoxelMeshComponent") {
			if (DiggingToolMode == 0) {
				const float Radius = ToolSizeArray[DiggingToolSize];

				FVector MiningPos = Res.Location;
				FVector PlayerPos = PlayerCharacter->GetActorLocation();
				//FVector Dir = PlayerPos - MiningPos;
				//Dir.Normalize(0.01);
				//Dir *= Radius / 2;
				//DrawDebugLine(World, MiningPos, Dir + MiningPos, FColor(255, 0, 0), false, -1, 0, 2);
				//DrawDebugSphere(World, Res.Location, Radius, 24, FColor(255, 255, 255, 100));

				//FVector P = Res.Normal * Radius + Res.Location;
				//DrawDebugSphere(World, P, Radius, 24, FColor(255, 255, 255, 100));

				const FVector CursorFV = Res.ImpactNormal;
				const FRotator CursorR = CursorFV.Rotation();
				PlayerCharacter->CursorToWorld->SetWorldLocation(Res.Location);
				PlayerCharacter->CursorToWorld->SetWorldRotation(CursorR);
				PlayerCharacter->CursorToWorld->DecalSize = FVector(100.f, Radius, Radius);
				PlayerCharacter->CursorToWorld->SetVisibility(true);
			}

			if (DiggingToolMode == 1) {
				const FVector Location = SnapToGrid(Res.Location, Dig_Snap_To_Grid);

				/*				
				FVector Dir = Res.Normal;
				FVector NewDir(0);

				if (Dir.X > 0.5) {
					NewDir = FVector(1, 0, 0);
				}

				if (Dir.X < -0.5) {
					NewDir = FVector(-1, 0, 0);
				}

				if (Dir.Y > 0.5) {
					NewDir = FVector(0, 1, 0);
				}

				if (Dir.Y < -0.5) {
					NewDir = FVector(0, -1, 0);
				}

				if (Dir.Z > 0.5) {
					NewDir = FVector(0, 0, 1);
				}

				if (Dir.Z < -0.5) {
					NewDir = FVector(0, 0, -1);
				}
				*/

				//FVector NewMiningPos = MiningPos + (NewDir * Dig_Cube_Size * 0.75);
				//DrawDebugLine(World, MiningPos, NewMiningPos, FColor(255, 0, 0), false, -1, 0, 2);
				//DrawDebugBox(World, NewMiningPos, FVector(Dig_Cube_Size), FColor(255, 255, 255, 100));

				DrawDebugBox(World, Location, FVector(Dig_Cube_Size), FColor(255, 255, 255, 100));
				PlayerCharacter->CursorToWorld->SetVisibility(false);
			}
		//}

		//if (ComponentName == "TerrainInstancedStaticMesh") {
		//	DrawDebugPoint(World, Res.Location, 5.f, FColor(255, 255, 255, 0), false, 1);
		//}
		
		return true;
	} else {
		//DrawDebugPoint(World, Res.Location, 5.f, FColor(255, 255, 255, 0), false, 1);

	}

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