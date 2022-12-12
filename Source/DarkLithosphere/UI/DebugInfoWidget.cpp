// Fill out your copyright notice in the Description page of Project Settings.

#include "DebugInfoWidget.h"
#include "../TerrainController.h"
#include "../Globals.h"

FString UDebugInfoWidget::SandboxDebugInfo1Text() {

	if (!TerrainController) {
		for (TActorIterator<ATerrainController> ActorItr(GetWorld()); ActorItr; ++ActorItr) {
			ATerrainController* TerrainCtrl = Cast<ATerrainController>(*ActorItr);
			if (TerrainCtrl) {
				UE_LOG(LogTemp, Log, TEXT("Found ATerrainController -> %s"), *TerrainCtrl->GetName());
				TerrainController = TerrainCtrl;
				break;
			}
		}
	}

	if (TerrainController) {
		FTerrainDebugInfo Memstat = TerrainController->GetMemstat();

		FString Res = FString::Printf(TEXT("vd: %d md: %d cd: %d"), Memstat.CountVd, Memstat.CountMd, Memstat.CountCd);
		return Res;
	}

	return FString(TEXT("no terrain"));
}

FString UDebugInfoWidget::SandboxDebugInfo2Text() {
	if (TerrainController) {
		FTerrainDebugInfo Memstat = TerrainController->GetMemstat();

		FString Res = FString::Printf(TEXT("conv: %d"), Memstat.ConveyorSize);
		return Res;
	}

	return FString(TEXT(""));
}

FString UDebugInfoWidget::SandboxDebugInfo3Text() {
	if (TerrainController) {
		FTerrainDebugInfo Memstat = TerrainController->GetMemstat();

		FString Res = FString::Printf(TEXT("tasks: %d"), Memstat.TaskPoolSize);
		return Res;
	}

	return FString(TEXT(""));
}



