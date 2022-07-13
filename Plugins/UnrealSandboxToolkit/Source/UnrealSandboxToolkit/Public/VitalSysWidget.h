// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Runtime/UMG/Public/UMG.h"
#include "Runtime/UMG/Public/UMGStyle.h"
#include "Runtime/UMG/Public/Slate/SObjectWidget.h"
#include "Runtime/UMG/Public/IUMGModule.h"
#include "Runtime/UMG/Public/Blueprint/UserWidget.h"

#include "Blueprint/UserWidget.h"
#include "VitalSysWidget.generated.h"


class UVitalSystemComponent;

/**
 * 
 */
UCLASS()
class UNREALSANDBOXTOOLKIT_API USandboxVitalSysWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:	

	UFUNCTION(BlueprintCallable, Category = "SandboxVitalSys")
	float GetHealth();

	UFUNCTION(BlueprintCallable, Category = "SandboxVitalSys")
	float GetMaxHealth();

	UFUNCTION(BlueprintCallable, Category = "Sandbox Widget")
	float GetHealthInPercent();

	UFUNCTION(BlueprintCallable, Category = "Sandbox Widget")
	float GetStaminaInPercent();

private:

	UVitalSystemComponent* GetVitalSystemComponent();
};
