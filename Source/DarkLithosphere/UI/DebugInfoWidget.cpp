// Fill out your copyright notice in the Description page of Project Settings.

#include "DebugInfoWidget.h"
#include "../TerrainController.h"
#include "../Globals.h"
#include "../MainPlayerController.h"

bool IsDatailedInfo() {
	return false;
}

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

	if (!IsDatailedInfo()) {
		return FString(TEXT(""));
	}

	if (TerrainController) {
		FTerrainDebugInfo Memstat = TerrainController->GetMemstat();

		FString Res = FString::Printf(TEXT("vd: %d md: %d cd: %d"), Memstat.CountVd, Memstat.CountMd, Memstat.CountCd);
		return Res;
	}

	return FString(TEXT("no terrain"));
}

FString UDebugInfoWidget::SandboxDebugInfo2Text() {
	if (!IsDatailedInfo()) {
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
	if (!IsDatailedInfo()) {
		return FString(TEXT(""));
	}

	if (TerrainController) {
		FTerrainDebugInfo Memstat = TerrainController->GetMemstat();

		FString Res = FString::Printf(TEXT("tasks: %d"), Memstat.TaskPoolSize);
		return Res;
	}

	return FString(TEXT(""));
}

FString UDebugInfoWidget::SandboxPlayerCrdText() {
	if (!IsDatailedInfo()) {
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
	AMainPlayerController* MainPlayerController = Cast<AMainPlayerController>(GetOwningPlayer());
	if (MainPlayerController) {
		APawn* Pawn = MainPlayerController->GetPawn();
		if (Pawn) {
			FVector Pos = Pawn->GetActorLocation();
			return FString::Printf(TEXT("%d, %d, %d"), (int)(Pos.X / 1000), (int)(Pos.Y / 1000), (int)(Pos.Z / 1000));
		}
	}

	return FString(TEXT(""));
}


