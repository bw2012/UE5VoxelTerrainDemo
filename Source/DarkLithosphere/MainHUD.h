// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameFramework/HUD.h"
#include "MainHUD.generated.h"


/**
 * 
 */
UCLASS()
class DARKLITHOSPHERE_API AMainHUD : public AHUD
{
	GENERATED_BODY()
	
public:
	void BeginPlay();


public:
	UPROPERTY(EditAnywhere, Category = "Sandbox")
	TSubclassOf<class UUserWidget> HudInventoryClass;

	UPROPERTY(EditAnywhere, Category = "Sandbox")
	TMap<FString, TSubclassOf<class UUserWidget>> WidgetMap;

	void OpenWidget(FString Name);

	void CloseWidget(FString Name);

	void CloseAllWidgets();

	void ShowInGameInventory();

private:

	TMap<FString, UUserWidget*> ActiveWidgetMap;
	
};
