
#include "CraftItemWidget.h"
#include "../MainPlayerController.h"



void UCraftRecipeItemWidget::SetPlayerControllerExtMode(int Index) {
	AMainPlayerController* MainPlayerController = Cast<AMainPlayerController>(GetOwningPlayer());
	if (MainPlayerController) {
		MainPlayerController->SetExtMode(Index);
	}
}

UTexture2D* UCraftRecipeItemWidget::GetCraftReceipeIcon(int ReceipeIndex) {
	AMainPlayerController* MainPlayerController = Cast<AMainPlayerController>(GetOwningPlayer());
	if (MainPlayerController) {
		auto* CraftRecipeData = MainPlayerController->GetCraftRecipeData(ReceipeIndex);
		if (CraftRecipeData) {
			return CraftRecipeData->IconTexture;
		}
	}

	return nullptr;
}


FString UCraftRecipeItemWidget::GetCraftReceipeName(int ReceipeIndex) {
	AMainPlayerController* MainPlayerController = Cast<AMainPlayerController>(GetOwningPlayer());
	if (MainPlayerController) {
		auto* CraftRecipeData = MainPlayerController->GetCraftRecipeData(ReceipeIndex);
		if (CraftRecipeData) {
			return CraftRecipeData->Name;
		}
	}

	return TEXT("");
}


