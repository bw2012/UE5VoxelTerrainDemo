
#include "HoverObjectWidget.h"
#include "../MainPlayerController.h"
#include "../LevelController.h"


FString UHoverObjectWidget::GetSandboxHoverObjectText() {
	AMainPlayerController* MainPlayerController = Cast<AMainPlayerController>(GetOwningPlayer());
	if (MainPlayerController) {
		return MainPlayerController->UiHoverObject.CommonName;
	}

	return TEXT("");
}

FString UHoverObjectWidget::GetSandboxHoverObjectTextExt1() {
	AMainPlayerController* MainPlayerController = Cast<AMainPlayerController>(GetOwningPlayer());
	if (MainPlayerController) {
		return MainPlayerController->UiHoverObject.ChemicalFormula;
	}

	return TEXT("");
}

FString UHoverObjectWidget::GetSandboxHoverObjectDescription() {
	AMainPlayerController* MainPlayerController = Cast<AMainPlayerController>(GetOwningPlayer());
	if (MainPlayerController) {
		return MainPlayerController->UiHoverObject.Description;
	}

	return TEXT("");
}
