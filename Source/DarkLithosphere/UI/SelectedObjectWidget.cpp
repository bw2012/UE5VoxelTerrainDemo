
#include "SelectedObjectWidget.h"
#include "../TerrainController.h"
#include "../MainPlayerController.h"
#include "../MainPlayerControllerComponent.h"
#include "TerrainZoneComponent.h"
#include "../LevelController.h"
#include "../Objects/BaseObject.h"


FString USelectedObjectWidget::GetSandboxSelectedObjectText() {
	MakeTextList();

	AMainPlayerController* MainPlayerController = Cast<AMainPlayerController>(GetOwningPlayer());
	if (MainPlayerController->HasOpenContainer()) {
		return TEXT("");
	}

	if (MainPlayerController && MainPlayerController->MainPlayerControllerComponent) {
		FSelectedObject SelectedObject = MainPlayerController->MainPlayerControllerComponent->SelectedObject;

		if (SelectedObject.SandboxObj || SelectedObject.TerrainMesh) {
			FString Res = SelectedObject.Name;

			if (SelectedObject.ExtText1 != "") {
				Res += TEXT(" (");
				Res += SelectedObject.ExtText1;
				Res += TEXT(")");
			}

			return Res;
		}
	}

	return TEXT("");
}

void USelectedObjectWidget::MakeTextList() {
	List.clear();

	AMainPlayerController* MainPlayerController = Cast<AMainPlayerController>(GetOwningPlayer());
	if (MainPlayerController->HasOpenContainer()) {
		return;
	}

	if (MainPlayerController && MainPlayerController->MainPlayerControllerComponent) {
		FSelectedObject SelectedObject = MainPlayerController->MainPlayerControllerComponent->SelectedObject;

		if (SelectedObject.ObjType == ESelectedObjectType::InstancedMesh && SelectedObject.bCanTake) {
			List.push_back(std::tuple("R", "Pick-up"));
		} else if (SelectedObject.ObjType == ESelectedObjectType::SandboxObject) {
			if (SelectedObject.SandboxObj) {
				if (SelectedObject.SandboxObj->IsInteractive()) {
					List.push_back(std::tuple("E", "Interact"));
				} else {
					ABaseObject* BaseObj = Cast<ABaseObject>(SelectedObject.SandboxObj);
					if (BaseObj && BaseObj->IsContainer()) {
						List.push_back(std::tuple("E", "Open"));
					}
				}

				if (SelectedObject.SandboxObj->CanTake(nullptr)) {
					List.push_back(std::tuple("R", "Pick-up"));
				}

			}
		}
	}

}

FString  USelectedObjectWidget::GetSandboxKeyDescription() {
	if (List.size() > 0) {
		const auto& P = List[0];
		return FString(std::get<1>(P));
	}

	return TEXT("");
}

FString  USelectedObjectWidget::GetSandboxKeyDescription2() {
	if (List.size() > 1) {
		const auto& P = List[1];
		return FString(std::get<1>(P));
	}

	return TEXT("");
}

FString USelectedObjectWidget::GetSandboxKeyText() {
	if (List.size() > 0) {
		const auto& P = List[0];
		return FString(std::get<0>(P));
	}

	return TEXT("");
}


FString USelectedObjectWidget::GetSandboxKeyText2() {
	if (List.size() > 1) {
		const auto& P = List[1];
		return FString(std::get<0>(P));
	}

	return TEXT("");
}



