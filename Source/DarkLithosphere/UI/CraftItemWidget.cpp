
#include "CraftItemWidget.h"
#include "../MainPlayerController.h"


void UCraftRecipeItemWidget::SetPlayerControllerExtMode(int Index) {
	AMainPlayerController* MainPlayerController = Cast<AMainPlayerController>(GetOwningPlayer());
	if (MainPlayerController) {
		MainPlayerController->SetExtMode(MainPlayerController->GetSandboxModeExtByPage(Index));
	}
}

UTexture2D* UCraftRecipeItemWidget::GetCraftReceipeIcon(int ReceipeIndex) {
	AMainPlayerController* MainPlayerController = Cast<AMainPlayerController>(GetOwningPlayer());
	if (MainPlayerController) {
		auto* CraftRecipeData = MainPlayerController->GetCraftRecipeData(MainPlayerController->GetSandboxModeExtByPage(ReceipeIndex));
		if (CraftRecipeData) {
			return CraftRecipeData->IconTexture;
		}
	}

	return nullptr;
}

FString UCraftRecipeItemWidget::GetCraftReceipeName(int ReceipeIndex) {
	AMainPlayerController* MainPlayerController = Cast<AMainPlayerController>(GetOwningPlayer());
	if (MainPlayerController) {
		auto* CraftRecipeData = MainPlayerController->GetCraftRecipeData(MainPlayerController->GetSandboxModeExtByPage(ReceipeIndex));
		if (CraftRecipeData) {
			return CraftRecipeData->Name;
		}
	}

	return TEXT("");
}

FString UCraftRecipeItemWidget::GetCraftReceipePart(int ReceipeIndex, int PartId) {
	AMainPlayerController* MainPlayerController = Cast<AMainPlayerController>(GetOwningPlayer());
	if (MainPlayerController) {
		const auto* CraftRecipeData = MainPlayerController->GetCraftRecipeData(MainPlayerController->GetSandboxModeExtByPage(ReceipeIndex));
		if (CraftRecipeData) {
			if (CraftRecipeData->Parts.Num() > PartId) {
				const auto Part = CraftRecipeData->Parts[PartId];
				if (Part.Amount > 0) {
					return FString::Printf(TEXT("x%d %s"), Part.Amount, *Part.Name);
				}
			}
		}
	}

	return TEXT("");
}

FLinearColor UCraftRecipeItemWidget::GetCraftReceipeColor(int ReceipeIndex) {
	AMainPlayerController* MainPlayerController = Cast<AMainPlayerController>(GetOwningPlayer());
	if (MainPlayerController) {

		bool bEnable = MainPlayerController->ValidateCraftItems(MainPlayerController->GetSandboxModeExtByPage(ReceipeIndex));

		if (bEnable) {
			return FLinearColor(255, 255, 255, 255);
		}
	}

	return FLinearColor(255, 0, 0, 255);
}

FLinearColor UCraftRecipeItemWidget::GetCraftReceipePartColor(int ReceipeIndex, int PartId) {
	AMainPlayerController* MainPlayerController = Cast<AMainPlayerController>(GetOwningPlayer());
	if (MainPlayerController) {

		bool bEnable = MainPlayerController->ValidateCraftItemPart(MainPlayerController->GetSandboxModeExtByPage(ReceipeIndex), PartId);

		if (bEnable) {
			return FLinearColor(255, 255, 255, 255);
		}
	}

	return FLinearColor(255, 0, 0, 255);
}



