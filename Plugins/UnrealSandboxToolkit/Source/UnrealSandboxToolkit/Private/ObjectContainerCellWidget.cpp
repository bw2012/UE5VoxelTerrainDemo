// Copyright blackw 2015-2020

#include "ObjectContainerCellWidget.h"
#include "ContainerComponent.h"
#include "SandboxObject.h"
#include "SandboxPlayerController.h"

FLinearColor USandboxObjectContainerCellWidget::SlotBorderColor(int32 SlotId) {
	if (ContainerName == TEXT("Inventory")) { 	//TODO fix
		ASandboxPlayerController* PlayerController = Cast<ASandboxPlayerController>(GetOwningPlayer());
		if (PlayerController) {
			if (PlayerController->CurrentInventorySlot == SlotId) {
				return FLinearColor(0.1, 0.4, 1, 1);
		}
	}
}
	return FLinearColor(0, 0, 0, 0.5);
}

FString USandboxObjectContainerCellWidget::SlotGetAmountText(int32 SlotId) {
	UContainerComponent* Container = GetContainer();
	if (Container != NULL) {
		const FContainerStack* Stack = Container->GetSlot(SlotId);
		if (Stack != NULL) {
			if (Stack->GetObject() != nullptr) {
				const ASandboxObject* DefaultObject = Cast<ASandboxObject>(Stack->GetObject());
				if (DefaultObject != nullptr) {
					if (!DefaultObject->bStackable) {
						return TEXT("");
					}
				}

				if (Stack->Amount > 0) {
					return FString::Printf(TEXT("%d"), Stack->Amount);
				}
			}
		}
	}

	return TEXT("");
}

bool USandboxObjectContainerCellWidget::IsExternal() {
	return CellBinding == EContainerCellBinding::ExternalObject;
}

UContainerComponent* USandboxObjectContainerCellWidget::GetContainer() {
	if (IsExternal()) {
		ASandboxPlayerController* SandboxPC = Cast<ASandboxPlayerController>(GetOwningPlayer());
		if (SandboxPC) {
			return SandboxPC->GetOpenedContainer();
		}
	} else {
		APawn* Pawn = GetOwningPlayer()->GetPawn();
		if (Pawn) {
			TArray<UContainerComponent*> Components;
			Pawn->GetComponents<UContainerComponent>(Components);

			for (UContainerComponent* Container : Components) {
				if (Container->GetName().Equals(ContainerName.ToString())) {
					return Container;
				}
			}
		}
	}

	return nullptr;
}

UTexture2D* USandboxObjectContainerCellWidget::GetSlotTexture(int32 SlotId) {
	
	UContainerComponent* Container = GetContainer();
	if (Container != nullptr) {
		const FContainerStack* Stack = Container->GetSlot(SlotId);
		if (Stack != nullptr) {
			if (Stack->Amount > 0) {
				if (Stack->GetObject() != nullptr) {
					const ASandboxObject* DefaultObject = Cast<ASandboxObject>(Stack->GetObject());
					if (DefaultObject != nullptr) {
						return DefaultObject->IconTexture;
					}
				}
			}
		}
	}
	
	return nullptr;
}

void USandboxObjectContainerCellWidget::SelectSlot(int32 SlotId) {
	UE_LOG(LogTemp, Log, TEXT("SelectSlot: %d"), SlotId);
}

bool USandboxObjectContainerCellWidget::SlotDrop(int32 SlotDropId, int32 SlotTargetId, AActor* SourceActor, UContainerComponent* SourceContainer, bool bOnlyOne) {
	bool bResult = SlotDropInternal(SlotDropId, SlotTargetId, SourceActor, SourceContainer, bOnlyOne);
	if (bResult) {
		ASandboxPlayerController* LocalController = Cast<ASandboxPlayerController>(UGameplayStatics::GetPlayerController(GetWorld(), 0));
		if (LocalController && LocalController->GetNetMode() != NM_Client) {
			const FString TargetContainerName = GetContainer()->GetName();
			const FString SourceContainerName = SourceContainer->GetName();
			LocalController->OnContainerDropSuccess(SlotTargetId, *SourceContainerName, *TargetContainerName);
		}
	}

	return bResult;
}

bool USandboxObjectContainerCellWidget::SlotDropInternal(int32 SlotDropId, int32 SlotTargetId, AActor* SourceActor, UContainerComponent* SourceContainer, bool bOnlyOne) {
	UE_LOG(LogTemp, Log, TEXT("UI cell drop: drop id -> %d ---> target id -> %d"), SlotDropId, SlotTargetId);

	if (SourceContainer == nullptr) {
		return false;
	}

	if (SourceActor == nullptr) {
		return false;
	}

	UContainerComponent* TargetContainer = GetContainer();
	if (!TargetContainer) {
		return false;
	}

	if (SlotDropId == SlotTargetId && TargetContainer == SourceContainer) {
		return false;
	}

	return TargetContainer->SlotTransfer(SlotDropId, SlotTargetId, SourceActor, SourceContainer, bOnlyOne);
}

bool USandboxObjectContainerCellWidget::SlotIsEmpty(int32 SlotId) {
	return false;
}

void USandboxObjectContainerCellWidget::HandleSlotMainAction(int32 SlotId) {
	ASandboxPlayerController* SandboxPC = Cast<ASandboxPlayerController>(GetOwningPlayer());
	if (SandboxPC) {
		SandboxPC->OnContainerMainAction(SlotId, ContainerName);
	}
}

AActor * USandboxObjectContainerCellWidget::GetOpenedObject() {
	if (IsExternal()) {
		ASandboxPlayerController* SandboxPC = Cast<ASandboxPlayerController>(GetOwningPlayer());
		if (SandboxPC != nullptr) {
			return SandboxPC->GetOpenedObject();
		}
	} else {
		return GetOwningPlayer()->GetPawn();
	}

	return nullptr;
}

UContainerComponent * USandboxObjectContainerCellWidget::GetOpenedContainer() {
	return GetContainer();
}
