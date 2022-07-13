
#include "SelectedObjectWidget.h"
#include "../TerrainController.h"
#include "../MainPlayerController.h"
#include "../MainPlayerControllerComponent.h"
#include "TerrainZoneComponent.h"
#include "../LevelController.h"
#include "../Objects/BaseObject.h"


FString USelectedObjectWidget::GetSandboxSelectedObjectText() {
	AMainPlayerController* MainPlayerController = Cast<AMainPlayerController>(GetOwningPlayer());
	if (MainPlayerController && MainPlayerController->MainPlayerControllerComponent) {
		FSelectedObject SelectedObject = MainPlayerController->MainPlayerControllerComponent->SelectedObject;

		if (SelectedObject.ObjType == ESelectedObjectType::InstancedMesh) {
			return SelectedObject.Name;
		}

		if (SelectedObject.ObjType == ESelectedObjectType::SandboxObject) {
			return SelectedObject.Name;
		}


	}

	return TEXT("");
}

FString USelectedObjectWidget::GetSandboxKeyText() {
	AMainPlayerController* MainPlayerController = Cast<AMainPlayerController>(GetOwningPlayer());
	if (MainPlayerController && MainPlayerController->MainPlayerControllerComponent) {
		FSelectedObject SelectedObject = MainPlayerController->MainPlayerControllerComponent->SelectedObject;

		if (SelectedObject.ObjType == ESelectedObjectType::InstancedMesh) {
			return TEXT("R");
		}

		if (SelectedObject.ObjType == ESelectedObjectType::SandboxObject) {
			FString Res(TEXT(""));

			if (SelectedObject.SandboxObj) {
				if (SelectedObject.SandboxObj->IsInteractive()) {
					Res += TEXT("E");
				} else {
					ABaseObject* BaseObj = Cast<ABaseObject>(SelectedObject.SandboxObj);
					if (BaseObj) {
						if (BaseObj->IsContainer()) {
							Res += TEXT("E");
						}
					}
				}

				if (SelectedObject.SandboxObj->CanTake(nullptr)) {
					Res += TEXT(" R");
				}

			}

			return Res;
		}
	}

	return TEXT("");
}



