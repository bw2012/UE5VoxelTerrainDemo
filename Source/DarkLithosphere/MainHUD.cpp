// Fill out your copyright notice in the Description page of Project Settings.

#include "MainHUD.h"
#include "Runtime/UMG/Public/Blueprint/UserWidget.h"

void AMainHUD::BeginPlay() {

}

void AMainHUD::ShowInGameInventory() {
	if (HudInventoryClass) {
		UUserWidget* Wigdet = CreateWidget<UUserWidget>(GetWorld(), HudInventoryClass);
		if (Wigdet) {
			Wigdet->AddToViewport();

		}
	}
}

void AMainHUD::OpenWidget(FString Name) {
	if (WidgetMap.Contains(Name)) {
		if (!ActiveWidgetMap.Contains(Name)) {
			TSubclassOf<class UUserWidget> WidgetClass = WidgetMap[Name];
			UUserWidget* Wigdet = CreateWidget<UUserWidget>(GetWorld(), WidgetClass);
			if (Wigdet) {
				Wigdet->AddToViewport();
				ActiveWidgetMap.Add(Name, Wigdet);
			}
		}
	}
}

void AMainHUD::CloseWidget(FString Name) {
	if (ActiveWidgetMap.Contains(Name)) {
		UUserWidget* Wigdet = ActiveWidgetMap[Name];
		Wigdet->RemoveFromViewport();
		ActiveWidgetMap.Remove(Name);
	}
}

void AMainHUD::CloseAllWidgets() {
	for (auto& Itm : ActiveWidgetMap) {
		UUserWidget* Wigdet = Itm.Value;
		Wigdet->RemoveFromViewport();
	}

	ActiveWidgetMap.Empty();
}

