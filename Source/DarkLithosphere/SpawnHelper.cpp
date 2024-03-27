
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
