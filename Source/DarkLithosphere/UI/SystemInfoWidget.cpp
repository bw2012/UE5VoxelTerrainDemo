// Fill out your copyright notice in the Description page of Project Settings.

#include "SystemInfoWidget.h"
#include "../TerrainController.h"
#include "../Globals.h"

FString USystemInfoWidget::SandboxVersionInfoText() {
	return GetVersionString();
}

FString USystemInfoWidget::SandboxPlayerIdText() {
	return GetSandboxPlayerId();
}


