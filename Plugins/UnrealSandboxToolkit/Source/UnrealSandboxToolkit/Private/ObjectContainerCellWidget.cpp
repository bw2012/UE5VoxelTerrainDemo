// Copyright blackw 2015-2020

#include "ObjectContainerCellWidget.h"
#include "ContainerComponent.h"
#include "SandboxObject.h"
#include "SandboxPlayerController.h"

FLinearColor USandboxObjectContainerCellWidget::SlotBorderColor(int32 SlotId) {
	//TODO slot colors
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
	if (SlotDropInternal(SlotDropId, SlotTargetId, SourceActor, SourceContainer, bOnlyOne)) {
		FName SourceName = *SourceContainer->GetName();
		FName TargetName = ContainerName;

		if (!IsExternal()) {
			ASandboxPlayerController* SandboxPC = Cast<ASandboxPlayerController>(GetOwningPlayer());
			if (SandboxPC) {
				SandboxPC->OnContainerDropSuccess(SlotTargetId, SourceName, TargetName);
			}
		} else {
			ACharacter* PlayerCharacter = Cast<ACharacter>(SourceActor);
			if (PlayerCharacter) {
				ASandboxPlayerController* SandboxPC = Cast<ASandboxPlayerController>(PlayerCharacter->GetController());
				if (SandboxPC) {
					SandboxPC->OnContainerDropSuccess(SlotTargetId, SourceName, TargetName);
				}
			}
		}

		return true;
	}

	return false;
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

	if (!IsExternal()) {
		ASandboxPlayerController* SandboxPC = Cast<ASandboxPlayerController>(GetOwningPlayer());
		if (SandboxPC) {
			ASandboxObject* Obj = (ASandboxObject*)StackSource.ObjectClass->GetDefaultObject();
			if (!SandboxPC->OnContainerDropCheck(SlotTargetId, ContainerName, Obj)) {
				return false;
			}
		}
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
