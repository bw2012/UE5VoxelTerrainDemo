
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

		return true;
	}

	return false;
}

