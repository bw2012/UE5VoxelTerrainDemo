// Copyright Epic Games, Inc. All Rights Reserved.

#include "DarkLithosphere.h"
#include "Modules/ModuleManager.h"

IMPLEMENT_PRIMARY_GAME_MODULE( FDarkLithosphereGameModule, DarkLithosphere, "DarkLithosphere" );

TAutoConsoleVariable<int32> CVarDebugInfo(
	TEXT("dl.DebugInfo"),
	0,
	TEXT("Draw debug text\n")
	TEXT(" 0 = Off \n")
	TEXT(" 1 = On \n"),
	ECVF_SetBySystemSettingsIni);


FString GetVersionString();

void FDarkLithosphereGameModule::StartupModule() {
	UE_LOG(LogTemp, Warning, TEXT("Start FDarkLithosphereGameModule %s"), *GetVersionString());

	FNetworkVersion::GetLocalNetworkVersionOverride.BindLambda([]() {
		return GetTypeHash(GetVersionString());
	});

	FString SaveDir = FPaths::ProjectSavedDir() + TEXT("Map/");
	UE_LOG(LogTemp, Log, TEXT("Check directory: %s"), *SaveDir);
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	if (!PlatformFile.DirectoryExists(*SaveDir)) {
		PlatformFile.CreateDirectory(*SaveDir);
		UE_LOG(LogTemp, Log, TEXT("Create directory: %s"), *SaveDir);
		if (!PlatformFile.DirectoryExists(*SaveDir)) {
			UE_LOG(LogTemp, Warning, TEXT("Unable to create directory: %s"), *SaveDir);
		}
	}
}

void FDarkLithosphereGameModule::ShutdownModule() {

}

FString GetVersionString() {
	return TEXT("v0.0.39e-alpha-L");
}

FString GlobalSandboxPlayerId;

FString GetSandboxPlayerId() {
	return GlobalSandboxPlayerId;
}

void SetSandboxPlayerId(const FString& SandboxPlayerId) {
	GlobalSandboxPlayerId = SandboxPlayerId;
}