
#include "CraftTableWidget.h"
#include "../MainPlayerController.h"

void UCraftTableWidget::SetNextCraftPage(int32 Idx) {
	AMainPlayerController* MainPlayerController = Cast<AMainPlayerController>(GetOwningPlayer());
	if (MainPlayerController) {
		int Page = MainPlayerController->GetSandboxExtPage();
		MainPlayerController->SetSandboxExtPage(Page + Idx);
		//UE_LOG(LogTemp, Log, TEXT("Page: %d"), Page);
	}

}