// Copyright blackw 2015-2020

#include "ObjectContainerCellWidget.h"
#include "ContainerComponent.h"
#include "SandboxObject.h"
#include "SandboxPlayerController.h"

FLinearColor USandboxObjectContainerCellWidget::SlotBorderColor(int32 SlotId) {
	if (ContainerId == 0) {
		ASandboxPlayerController* PlayerController = Cast<ASandboxPlayerController>(GetOwningPlayer());
		if (PlayerController) {
			if (PlayerController->CurrentInventorySlot == SlotId) {
				return FLinearColor(0.1, 0.4, 1, 1);
			}
		}
	}

	/*
	UContainerComponent* Container = GetContainer();
	if (Container != NULL) {
		FContainerStack* Stack = Container->GetSlot(SlotId);
		if (Stack != NULL) {
			if (Stack->Object != nullptr) {
				if (Stack->Amount > 0) {
					return FLinearColor(0.97, 1, 0.4, 1);
				}
			}
		}
	}
	*/

	return FLinearColor(0, 0, 0, 0.5);
}


FString USandboxObjectContainerCellWidget::SlotGetAmountText(int32 SlotId) {
	UContainerComponent* Container = GetContainer();
	if (Container != NULL) {
		FContainerStack* Stack = Container->GetSlot(SlotId);
		if (Stack != NULL) {
			if (Stack->ObjectClass != nullptr) {
				ASandboxObject* DefaultObject = Cast<ASandboxObject>(Stack->ObjectClass->GetDefaultObject());
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


UContainerComponent* USandboxObjectContainerCellWidget::GetContainer() {
	if (ContainerId == 0) {
		APawn* Pawn = GetOwningPlayer()->GetPawn();
		if (Pawn) {
			TArray<UContainerComponent*> Components;
			Pawn->GetComponents<UContainerComponent>(Components);

			for (UContainerComponent* Container : Components) {
				if (Container->GetName().Equals(TEXT("Inventory"))) {
					return Container;
				}
			}
		}
	}

	if (ContainerId == 100) { // opened object
		ASandboxPlayerController* SandboxPC = Cast<ASandboxPlayerController>(GetOwningPlayer());
		if (SandboxPC) {
			return SandboxPC->GetOpenedContainer();
		}
	}

	return nullptr;
}

UTexture2D* USandboxObjectContainerCellWidget::GetSlotTexture(int32 SlotId) {
	
	UContainerComponent* Container = GetContainer();
	if (Container != nullptr) {
		FContainerStack* Stack = Container->GetSlot(SlotId);
		if (Stack != nullptr) {
			if (Stack->Amount > 0) {
				if (Stack->ObjectClass != nullptr) {
					ASandboxObject* DefaultObject = Cast<ASandboxObject>(Stack->ObjectClass->GetDefaultObject());
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

bool IsSameObject(FContainerStack* StackSourcePtr, FContainerStack* StackTargetPtr) {
	if (StackSourcePtr && StackTargetPtr) {
		const auto* SourceObj = StackSourcePtr->GetObject();
		const auto* TargetObj = StackTargetPtr->GetObject();
		if (SourceObj && TargetObj) {
			return SourceObj->GetSandboxClassId() == TargetObj->GetSandboxClassId();
		}

	}

	return false;
}

bool USandboxObjectContainerCellWidget::SlotDrop(int32 SlotDropId, int32 SlotTargetId, AActor* SourceActor, UContainerComponent* SourceContainer, bool bOnlyOne) {
	UE_LOG(LogTemp, Log, TEXT("UI cell drop: drop id -> %d ---> target id -> %d"), SlotDropId, SlotTargetId);

	if (SourceContainer == nullptr) {
		return false;
	}

	if (SourceActor == nullptr) {
		return false;
	}

	UContainerComponent* TargetContainer = GetContainer();

	if (SlotDropId == SlotTargetId && TargetContainer == SourceContainer) {
		return false;
	}

	FContainerStack* StackSourcePtr = SourceContainer->GetSlot(SlotDropId);
	FContainerStack* StackTargetPtr = TargetContainer->GetSlot(SlotTargetId);

	FContainerStack StackSource;
	FContainerStack StackTarget;

	if (StackTargetPtr) {
		StackTarget = *StackTargetPtr;
	}

	if (StackSourcePtr) {
		StackSource = *StackSourcePtr;
	}

	if (IsSameObject(StackSourcePtr, StackTargetPtr)) {
		ASandboxObject* Obj = (ASandboxObject*)StackTarget.ObjectClass->GetDefaultObject();
		uint32 AddAmount = (bOnlyOne) ? 1 : StackSourcePtr->Amount;
		uint32 NewAmount = StackTargetPtr->Amount + AddAmount;
		if (NewAmount <= Obj->MaxStackSize) {
			StackTargetPtr->Amount = NewAmount;
			StackSourcePtr->Clear();
			return true;
		} else {
			int D = Obj->MaxStackSize - StackTargetPtr->Amount;
			StackTargetPtr->Amount = Obj->MaxStackSize;
			StackSourcePtr->Amount -= D;
			return true;
		}

		StackTargetPtr->Amount += StackSourcePtr->Amount;
		StackSourcePtr->Clear();
		return true;
	}

	if (bOnlyOne) {
		if (!SourceContainer->IsSlotEmpty(SlotDropId)) {
			if (TargetContainer->IsSlotEmpty(SlotTargetId)) {
				TSubclassOf<ASandboxObject>	ObjectClass = StackSourcePtr->ObjectClass;
				SourceContainer->ChangeAmount(SlotDropId, -1);
				FContainerStack NewStack;
				NewStack.ObjectClass = ObjectClass;
				NewStack.Amount = 1;
				TargetContainer->AddStack(NewStack, SlotTargetId);
				return true;
			}
		}
	} else {
		SourceContainer->AddStack(StackTarget, SlotDropId);
		TargetContainer->AddStack(StackSource, SlotTargetId);
		return true;
	}
	
	return false;
}

bool USandboxObjectContainerCellWidget::SlotIsEmpty(int32 SlotId) {
	return false;
}


AActor* USandboxObjectContainerCellWidget::GetOpenedObject() {
	if (ContainerId == 100) { 
		ASandboxPlayerController* SandboxPC = Cast<ASandboxPlayerController>(GetOwningPlayer());
		if (SandboxPC != nullptr) {
			return SandboxPC->GetOpenedObject();
		}
	}

	if (ContainerId == 0) {
		return GetOwningPlayer()->GetPawn();
	}

	return nullptr;
}

UContainerComponent* USandboxObjectContainerCellWidget::GetOpenedContainer() {
	return GetContainer();
}