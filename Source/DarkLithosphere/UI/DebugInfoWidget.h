// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Runtime/UMG/Public/UMG.h"
#include "Runtime/UMG/Public/UMGStyle.h"
#include "Runtime/UMG/Public/Slate/SObjectWidget.h"
#include "Runtime/UMG/Public/IUMGModule.h"
#include "Runtime/UMG/Public/Blueprint/UserWidget.h"

#include "../TerrainController.h"

#include "Blueprint/UserWidget.h"
#include "DebugInfoWidget.generated.h"

/**
 * 
 */
UCLASS()
class DARKLITHOSPHERE_API UDebugInfoWidget : public UUserWidget {
	GENERATED_BODY()
	
protected:

	UFUNCTION(BlueprintCallable, Category = "Sandbox System info")
	FString SandboxPlayerCrdText();

	UFUNCTION(BlueprintCallable, Category = "Sandbox System info")
	FString SandboxZoneIndexText();

	UFUNCTION(BlueprintCallable, Category = "Sandbox System info")
	FString SandboxDebugInfo1Text();

	UFUNCTION(BlueprintCallable, Category = "Sandbox System info")
	FString SandboxDebugInfo2Text();

	UFUNCTION(BlueprintCallable, Category = "Sandbox System info")
	FString SandboxDebugInfo3Text();

	UFUNCTION(BlueprintCallable, Category = "Sandbox System info")
	FString SandboxDebugInfo4Text();
	
private:

	ATerrainController* TerrainController = nullptr;

};
