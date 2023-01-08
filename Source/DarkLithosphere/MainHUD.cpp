// Fill out your copyright notice in the Description page of Project Settings.

#include "MainHUD.h"
#include "Runtime/UMG/Public/Blueprint/UserWidget.h"

void AMainHUD::BeginPlay() {

}

void AMainHUD::ShowInGameInventory() {
	if (InventoryWidget) {
		UUserWidget* Wigdet = CreateWidget<UUserWidget>(GetWorld(), InventoryWidget);
		if (Wigdet) {
			Wigdet->AddToViewport();
		}
	}
}

void AMainHUD::OpenWidget(FString Name, FString Tag) {
	if (WidgetMap.Contains(Name)) {
		if (!ActiveWidgetMap.Contains(Name)) {
			TSubclassOf<class UUserWidget> WidgetClass = WidgetMap[Name];
			UUserWidget* Wigdet = CreateWidget<UUserWidget>(GetWorld(), WidgetClass);
			if (Wigdet) {
				Wigdet->AddToViewport();
				ActiveWidgetMap.Add(Name, FActiveWidgetInfo{ Wigdet , Tag });
			}
		}
	}
}

void AMainHUD::CloseWidget(FString Name) {
	if (ActiveWidgetMap.Contains(Name)) {
		FActiveWidgetInfo ActiveWidgetInfo = ActiveWidgetMap[Name];
		ActiveWidgetInfo.Widget->RemoveFromViewport();
		ActiveWidgetMap.Remove(Name);
	}
}

void AMainHUD::CloseAllWidgets(FString Tag) {
	for (auto& Itm : ActiveWidgetMap) {
		FActiveWidgetInfo ActiveWidgetInfo = Itm.Value;
		if (ActiveWidgetInfo.Tag == Tag || Tag == TEXT("")) {
			ActiveWidgetInfo.Widget->RemoveFromViewport();
		}
	}

	ActiveWidgetMap.Empty();
}

