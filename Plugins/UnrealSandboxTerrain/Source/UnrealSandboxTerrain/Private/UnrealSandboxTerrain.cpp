// Copyright Epic Games, Inc. All Rights Reserved.

#include "UnrealSandboxTerrain.h"

#define LOCTEXT_NAMESPACE "FUnrealSandboxTerrainModule"


float LodScreenSizeArray[LOD_ARRAY_SIZE] = { 1.f, .5f, .25f, .125f, .0625, 0.03125 };

void FUnrealSandboxTerrainModule::StartupModule() {
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module

	UE_LOG(LogSandboxTerrain, Warning, TEXT("FUnrealSandboxTerrainModule::StartupModule()"));

	float LodRatio = 2.f;
	float ScreenSize = 1.f;
	for (auto LodIdx = 0; LodIdx < LOD_ARRAY_SIZE; LodIdx++) {
		UE_LOG(LogSandboxTerrain, Warning, TEXT("Lod %d -> %f"), LodIdx, ScreenSize);
		LodScreenSizeArray[LodIdx] = ScreenSize;
		ScreenSize /= LodRatio;
	}
}

void FUnrealSandboxTerrainModule::ShutdownModule() {

	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FUnrealSandboxTerrainModule, UnrealSandboxTerrain)

DEFINE_LOG_CATEGORY(LogSandboxTerrain);