
#include "MiningTool.h"
#include "VoxelMeshComponent.h"

#define Dig_Cube_Size 110 //60
#define Dig_Snap_To_Grid 100 //50

#define Dig_Cylinder_Length 100
#define Dig_Cylinder_Radius 100


FVector SnapToGrid(const FVector& Location, const float GridRange) {
	FVector Tmp(Location);
	Tmp /= GridRange;
	Tmp.Set(std::round(Tmp.X), std::round(Tmp.Y), std::round(Tmp.Z));
	Tmp *= GridRange;
	return FVector((int)Tmp.X, (int)Tmp.Y, (int)Tmp.Z);
}

AMiningTool::AMiningTool() {
	DiggingToolMode = 0;
	Strength = 5;
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

void AMiningTool::OnAltAction(const FHitResult& Hit, ABaseCharacter* PlayerCharacter) {
	UWorld* World = PlayerCharacter->GetWorld();
	ASandboxTerrainController* Terrain = Cast<ASandboxTerrainController>(Hit.GetActor());
	if (Terrain) {
		TVoxelIndex ZoneIndex = Terrain->GetZoneIndex(Hit.ImpactPoint);
		FVector ZoneIndexTmp(ZoneIndex.X, ZoneIndex.Y, ZoneIndex.Z);

		UVoxelMeshComponent* ZoneMesh = Cast<UVoxelMeshComponent>(Hit.Component.Get());
		if (ZoneMesh) {
			uint16 MatId = ZoneMesh->GetMaterialIdFromCollisionFaceIndex(Hit.FaceIndex);
			//UE_LOG(LogTemp, Warning, TEXT("MatId -> %d"), MatId);

			if (MatId > 0) {
				FSandboxTerrainMaterial MatInfo;
				if (Terrain->GetTerrainMaterialInfoById(MatId, MatInfo)) {
					PlaySound(MatInfo, Hit.Location, World);
				}
			}


		}

		//UE_LOG(LogTemp, Warning, TEXT("zIndex -> %f %f %f"), ZoneIndexTmp.X, ZoneIndexTmp.Y, ZoneIndexTmp.Z);

		if (DiggingToolMode == 0) {
			// Strength / Mat.RockHardness > 0.1
			Terrain->DigTerrainRoundHole(Hit.ImpactPoint, 80, Strength);
		}

		if (DiggingToolMode == 1) {
			FVector Location = SnapToGrid(Hit.Location, Dig_Snap_To_Grid);
			Terrain->DigTerrainCubeHole(Location, Dig_Cube_Size);
		}
	}
}

bool AMiningTool::OnTracePlayerActionPoint(const FHitResult& Res, ABaseCharacter* PlayerCharacter) {
	UWorld* World = PlayerCharacter->GetWorld();
	ASandboxTerrainController* TerrainController = Cast<ASandboxTerrainController>(Res.GetActor());
	if (TerrainController) {
		if (DiggingToolMode == 0) {
			static const float Radius = 80.f;

			FVector MiningPos = Res.Location;
			FVector PlayerPos = PlayerCharacter->GetActorLocation();
			FVector Dir = PlayerPos - MiningPos;
			Dir.Normalize(0.01);
			Dir *= Radius / 2;
			//DrawDebugLine(World, MiningPos, Dir + MiningPos, FColor(255, 0, 0), false, -1, 0, 2);

			DrawDebugSphere(World, Res.Location, Radius, 24, FColor(255, 255, 255, 100));
		}

		if (DiggingToolMode == 1) {
			const FVector Location = SnapToGrid(Res.Location, Dig_Snap_To_Grid);

			FVector MiningPos = Location;
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

			//FVector NewMiningPos = MiningPos + (NewDir * Dig_Cube_Size * 0.75);
			//DrawDebugLine(World, MiningPos, NewMiningPos, FColor(255, 0, 0), false, -1, 0, 2);
			//DrawDebugBox(World, NewMiningPos, FVector(Dig_Cube_Size), FColor(255, 255, 255, 100));

			DrawDebugBox(World, Location, FVector(Dig_Cube_Size), FColor(255, 255, 255, 100));
		}

		return true;
	}

	return false;
}

bool AMiningTool::VisibleInHand(FTransform& Transform) {
	return true;
}