// Fill out your copyright notice in the Description page of Project Settings.

#include "DebugInfoWidget.h"
#include "../TerrainController.h"
#include "../EnvironmentController.h"
#include "../Globals.h"
#include "../MainPlayerController.h"


extern TAutoConsoleVariable<int32> CVarDebugInfo;


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

	if (CVarDebugInfo.GetValueOnGameThread() < 2) {
		return FString(TEXT(""));
	}

	if (TerrainController) {
		FTerrainDebugInfo Memstat = TerrainController->GetMemstat();

		FString Res = FString::Printf(TEXT("vd: %d md: %d cd: %d zones: %d"), Memstat.CountVd, Memstat.CountMd, Memstat.CountCd, Memstat.CountZones);
		return Res;
	}

	return FString(TEXT("no terrain"));
}

FString UDebugInfoWidget::SandboxDebugInfo2Text() {
	if (CVarDebugInfo.GetValueOnGameThread() < 2) {
		return FString(TEXT(""));
	}

	if (TerrainController) {
		FTerrainDebugInfo Memstat = TerrainController->GetMemstat();

		FString Res = FString::Printf(TEXT("conv: %d"), Memstat.ConveyorSize);
		return Res;
	}

	return FString(TEXT(""));
}

FString UDebugInfoWidget::SandboxDebugInfo3Text() {
	if (CVarDebugInfo.GetValueOnGameThread() < 2) {
		return FString(TEXT(""));
	}

	if (TerrainController) {
		FTerrainDebugInfo Memstat = TerrainController->GetMemstat();

		FString Res = FString::Printf(TEXT("tasks: %d"), Memstat.TaskPoolSize);
		return Res;
	}

	return FString(TEXT(""));
}

FString UDebugInfoWidget::SandboxDebugInfo4Text() {
	if (CVarDebugInfo.GetValueOnGameThread() < 2) {
		return FString(TEXT(""));
	}

	if (TerrainController) {
		FTerrainDebugInfo Memstat = TerrainController->GetMemstat();

		FString Res = FString::Printf(TEXT("offsync: %d"), Memstat.OutOfSync);
		return Res;
	}

	return FString(TEXT(""));
}

FString UDebugInfoWidget::SandboxPlayerCrdText() {
	if (CVarDebugInfo.GetValueOnGameThread() < 2) {
		return FString(TEXT(""));
	}

	AMainPlayerController* MainPlayerController = Cast<AMainPlayerController>(GetOwningPlayer());
	if (MainPlayerController) {
		APawn* Pawn = MainPlayerController->GetPawn();
		if (Pawn) {
			FVector Pos = Pawn->GetActorLocation();
			return FString::Printf(TEXT("X=%.1f, Y=%.1f, Z=%.1f"), Pos.X, Pos.Y, Pos.Z);
		}
	}

	return FString(TEXT(""));
}

FString UDebugInfoWidget::SandboxZoneIndexText() {
	if (CVarDebugInfo.GetValueOnGameThread() < 1) {
		return FString(TEXT(""));
	}

	AMainPlayerController* MainPlayerController = Cast<AMainPlayerController>(GetOwningPlayer());
	if (MainPlayerController) {
		APawn* Pawn = MainPlayerController->GetPawn();
		if (Pawn) {
			if (TerrainController) {
				FVector Pos = Pawn->GetActorLocation();
				TVoxelIndex Tmp = TerrainController->GetZoneIndex(Pos);
				return FString::Printf(TEXT("%d, %d, %d"), Tmp.X, Tmp.Y, Tmp.Z);
			}
		}
	}

	return FString(TEXT(""));
}


FString UDebugInfoWidget::SandboxRegionIndexText() {
	if (CVarDebugInfo.GetValueOnGameThread() < 1) {
		return FString(TEXT(""));
	}

	AMainPlayerController* MainPlayerController = Cast<AMainPlayerController>(GetOwningPlayer());
	if (MainPlayerController) {
		APawn* Pawn = MainPlayerController->GetPawn();
		if (Pawn) {
			if (TerrainController) {
				FVector Pos = Pawn->GetActorLocation();
				TVoxelIndex ZoneIndex = TerrainController->GetZoneIndex(Pos);
				TVoxelIndex RegionIndex = TerrainController->ClcRegionByZoneIndex(ZoneIndex);
				return FString::Printf(TEXT("region: %d, %d"), RegionIndex.X, RegionIndex.Y);
			}
		}
	}

	return FString(TEXT(""));
}

FString UDebugInfoWidget::SandboxTimeString() {
	if (CVarDebugInfo.GetValueOnGameThread() < 1) {
		return FString(TEXT(""));
	}

	for (TActorIterator<ATerrainController> ActorItr(GetWorld()); ActorItr; ++ActorItr) {
		AEnvironmentController* Env = Cast<AEnvironmentController>(*ActorItr);
		if (Env) {
			return Env->GetCurrentTimeAsString();
			break;
		}
	}

	return FString(TEXT(""));
}


