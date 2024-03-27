// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameFramework/HUD.h"
#include "MainHUD.generated.h"


USTRUCT()
struct FActiveWidgetInfo {
	GENERATED_BODY()

	UPROPERTY()
	UUserWidget* Widget;

	UPROPERTY()
	FString Tag;
}; 


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
	TSubclassOf<class UUserWidget> InventoryWidget;

	UPROPERTY(EditAnywhere, Category = "Sandbox")
	TMap<FString, TSubclassOf<class UUserWidget>> WidgetMap;

	void OpenWidget(FString Name, FString Tag = TEXT(""));

	void CloseWidget(FString Name);

	void CloseAllWidgets(FString Tag = TEXT(""));

	void ShowInGameInventory();

	bool IsWidgedOpened(FString Name);

private:

	TMap<FString, FActiveWidgetInfo> ActiveWidgetMap;

	UUserWidget* MainGameWigdet;
	
};
