// Fill out your copyright notice in the Description page of Project Settings.

#include "SystemInfoWidget.h"
#include "../TerrainController.h"

FString GetVersionString();

FString USystemInfoWidget::SandboxVersionInfoText() {
	return GetVersionString();
}



