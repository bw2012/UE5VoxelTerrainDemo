
#include "SelectedObjectWidget.h"
#include "../TerrainController.h"
#include "../MainPlayerController.h"
#include "../MainPlayerControllerComponent.h"
#include "TerrainZoneComponent.h"
#include "../LevelController.h"
#include "../Objects/BaseObject.h"


FString USelectedObjectWidget::GetSandboxSelectedObjectText() {
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

// TODO переделать тут все с использованием массива

FString USelectedObjectWidget::GetSandboxKeyText() {
	AMainPlayerController* MainPlayerController = Cast<AMainPlayerController>(GetOwningPlayer());
	if (MainPlayerController->HasOpenContainer()) {
		return TEXT("");
	}

	if (MainPlayerController && MainPlayerController->MainPlayerControllerComponent) {
		FSelectedObject SelectedObject = MainPlayerController->MainPlayerControllerComponent->SelectedObject;

		if (SelectedObject.ObjType == ESelectedObjectType::InstancedMesh && SelectedObject.bCanTake) {
			return TEXT("R");
		}

		if (SelectedObject.ObjType == ESelectedObjectType::SandboxObject) {
			if (SelectedObject.SandboxObj) {
				if (SelectedObject.SandboxObj->IsInteractive()) {
					return TEXT("E");
				} 

				ABaseObject* BaseObj = Cast<ABaseObject>(SelectedObject.SandboxObj);
				if (BaseObj && BaseObj->IsContainer()) {
					return TEXT("E");
				}

				if (SelectedObject.SandboxObj->CanTake(nullptr)) {
					return TEXT("R");
				}

			}
		}
	}

	return TEXT("");
}


FString USelectedObjectWidget::GetSandboxKeyText2() {
	AMainPlayerController* MainPlayerController = Cast<AMainPlayerController>(GetOwningPlayer());
	if (MainPlayerController->HasOpenContainer()) {
		return TEXT("");
	}

	if (MainPlayerController && MainPlayerController->MainPlayerControllerComponent) {
		FSelectedObject SelectedObject = MainPlayerController->MainPlayerControllerComponent->SelectedObject;

		if (SelectedObject.ObjType == ESelectedObjectType::SandboxObject) {
			if (SelectedObject.SandboxObj) {
				bool bShow = false;

				if (SelectedObject.SandboxObj->IsInteractive()) {
					bShow = true;
				}

				ABaseObject* BaseObj = Cast<ABaseObject>(SelectedObject.SandboxObj);
				if (BaseObj && BaseObj->IsContainer()) {
					bShow = true;
				}

				if (bShow && SelectedObject.SandboxObj->CanTake(nullptr)) {
					return TEXT("R");
				}

			}
		}
	}

	return TEXT("");
}



