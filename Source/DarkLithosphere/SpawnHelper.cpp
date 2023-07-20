
#pragma once

#include "SpawnHelper.h"
#include "SandboxObject.h"
#include "SandboxTerrainController.h"


bool IsCursorPositionValid(const FHitResult& Hit) {
	if (Hit.bBlockingHit) {
		ACharacter* Character = Cast<ACharacter>(Hit.GetActor());
		if (Character) {
			return false;
		}

		ASandboxObject* Obj = Cast<ASandboxObject>(Hit.GetActor());
		if (Obj && !Obj->bCanPlaceSandboxObject) {
			return false;
		}

		return true;
	}

	return false;
}


void SpawnStones(ALevelController* LevelController, const FVector& Location, uint16 MatId, int Min, int Max) {
	FVector Pos = Location;

	if (MatId == 1) { // hardcoded dirt
		const auto Chance = FMath::RandRange(0.f, 1.f);
		if (Chance < 0.4f) {
			const static int ObjectIds[2] = { 9, 8 };
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
