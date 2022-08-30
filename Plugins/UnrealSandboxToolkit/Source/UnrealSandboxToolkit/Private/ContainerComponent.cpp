// Copyright blackw 2015-2020


#include "ContainerComponent.h"
#include "SandboxObject.h"
#include "Net/UnrealNetwork.h"
#include "SandboxLevelController.h"
#include "SandboxPlayerController.h"
#include <algorithm>

const ASandboxObject* FContainerStack::GetObject() const {
	return ASandboxLevelController::GetDefaultSandboxObject(SandboxClassId);
}

UContainerComponent::UContainerComponent() {
	//bWantsBeginPlay = true;
	PrimaryComponentTick.bCanEverTick = false;
}

void UContainerComponent::BeginPlay() {
	Super::BeginPlay();
}

void UContainerComponent::TickComponent( float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction ) {
	Super::TickComponent( DeltaTime, TickType, ThisTickFunction );
}

/*
bool UContainerComponent::IsOwnerAdmin() {
	ASandboxCharacter* SandboxCharacter = Cast<ASandboxCharacter>(GetOwner());
	if (SandboxCharacter) {
		return SandboxCharacter->bIsAdmin;
	}

	return true;
}
*/

bool UContainerComponent::IsEmpty() {
	if (Content.Num() == 0) {
		return true;
	}

	for (int Idx = 0; Idx < Content.Num(); Idx++) {
		FContainerStack Stack = Content[Idx];
		if (Stack.Amount > 0 ) {
			return false;
		}
	}

	return true;
}

bool UContainerComponent::SetStackDirectly(const FContainerStack& Stack, const int SlotId) {
	if (SlotId >= Content.Num()) {
		Content.SetNum(SlotId + 1);
	}

	FContainerStack* StackPtr = &Content[SlotId];
	if (Stack.Amount > 0) {
		StackPtr->Amount = Stack.Amount;
		StackPtr->SandboxClassId = Stack.SandboxClassId;
	} else {
		StackPtr->Clear();
	}

	bUpdated = true;
	return true;
}


bool UContainerComponent::AddObject(ASandboxObject* Obj) {
	if (Obj == nullptr) {
		return false;
	}

	uint32 MaxStackSize = Obj->GetMaxStackSize();

	int FirstEmptySlot = -1;
	bool bIsAdded = false;
	for (int Idx = 0; Idx < Content.Num(); Idx++) {
		FContainerStack* Stack = &Content[Idx];

		//TODO check inventory max volume and mass
		if (Stack->Amount != 0) {
			if (Stack->SandboxClassId > 0 && MaxStackSize > 1) {
				Stack->Amount++;
				bIsAdded = true;
				break;
			}
		} else {
			if (FirstEmptySlot < 0) {
				FirstEmptySlot = Idx;
				if (MaxStackSize == 1) {
					break;
				}
			}
		}
	}

	if (!bIsAdded) {
		if (FirstEmptySlot >= 0) {
			FContainerStack* Stack = &Content[FirstEmptySlot];
			Stack->Amount = 1;
			Stack->SandboxClassId = Obj->GetSandboxClassId();
		} else {
			FContainerStack NewStack;
			NewStack.Amount = 1;
			NewStack.SandboxClassId = Obj->GetSandboxClassId();
			Content.Add(NewStack);
		}
	}
	
	bUpdated = true;
	return true;
}

FContainerStack* UContainerComponent::GetSlot(const int Slot) {
	if (!Content.IsValidIndex(Slot)) {
		return nullptr;
	}

	return &Content[Slot];
}

/*
ASandboxObject* UContainerComponent::GetAvailableSlotObject(const int Slot) {
	if (!Content.IsValidIndex(Slot)) {
		return nullptr;
	}

	FContainerStack* Stack = &Content[Slot];
	if (Stack->Amount > 0) {
		TSubclassOf<ASandboxObject>	ObjectClass = Stack->ObjectClass;
		if (ObjectClass) {
			return (ASandboxObject*)(ObjectClass->GetDefaultObject());
		}
	}

	return nullptr;
}
*/

bool UContainerComponent::DecreaseObjectsInContainer(int Slot, int Num) {
	FContainerStack* Stack = GetSlot(Slot);

	if (Stack == NULL) { 
		return false;
	}

	if (Stack->Amount > 0) {
		Stack->Amount -= Num;
		if (Stack->Amount == 0) { 
			Stack->Clear();
		}
	}

	bUpdated = true;
	return Stack->Amount > 0;
}

void UContainerComponent::ChangeAmount(int Slot, int Num) {
	FContainerStack* Stack = GetSlot(Slot);

	//TODO check stack size
	if (Stack) {
		if (Stack->Amount > 0) {
			Stack->Amount += Num;
			if (Stack->Amount == 0) {
				Stack->Clear();
			}

			bUpdated = true;
		}
	}
}

bool UContainerComponent::IsSlotEmpty(int SlotId) {
	FContainerStack* Stack = GetSlot(SlotId);
	if (Stack) {
		return Stack->IsEmpty();
	}

	return true;
}

ASandboxObject* UContainerComponent::GetSandboxObject(int SlotId) {
	return NULL;
}

void UContainerComponent::GetLifetimeReplicatedProps(TArray< FLifetimeProperty > & OutLifetimeProps) const {
	DOREPLIFETIME(UContainerComponent, Content);
}

void UContainerComponent::CopyTo(UContainerComponent* Target) {
	Target->Content = this->Content;
	bUpdated = true;
}

TArray<uint64> UContainerComponent::GetAllObjects() {
	TArray<uint64> Result;
	for (int Idx = 0; Idx < Content.Num(); Idx++) {
		FContainerStack* Stack = &Content[Idx];
		if (Stack) {
			const ASandboxObject* Obj = Stack->GetObject();
			if (Obj) {
				Result.Add(Obj->GetSandboxClassId());
			}
		}
	}

	return Result;
}

bool IsSameObject(const FContainerStack* StackSourcePtr, const FContainerStack* StackTargetPtr) {
	if (StackSourcePtr && StackTargetPtr) {
		const auto* SourceObj = StackSourcePtr->GetObject();
		const auto* TargetObj = StackTargetPtr->GetObject();
		if (SourceObj && TargetObj) {
			return SourceObj->GetSandboxClassId() == TargetObj->GetSandboxClassId();
		}

	}

	return false;
}

void NetObjectTransfer(ASandboxPlayerController* LocalController, const ASandboxObject* RemoteObj, const UContainerComponent* RemoteContainer, const FContainerStack Stack, int32 SlotId) {
	const FString TargetContainerName = RemoteContainer->GetName();
	LocalController->TransferContainerStack(RemoteObj->GetSandboxNetUid(), TargetContainerName, Stack, SlotId);
}

void NetControllerTransfer(ASandboxPlayerController* LocalController, const UContainerComponent* RemoteContainer, const FContainerStack Stack, int32 SlotId) {
	const FString TargetContainerName = RemoteContainer->GetName();
	LocalController->TransferInventoryStack(TargetContainerName, Stack, SlotId);
}

bool UContainerComponent::SlotTransfer(int32 SlotSourceId, int32 SlotTargetId, AActor* SourceActor, UContainerComponent* SourceContainer, bool bOnlyOne) {
		UContainerComponent* TargetContainer = this;
		const FContainerStack* StackSourcePtr = SourceContainer->GetSlot(SlotSourceId);
		const FContainerStack* StackTargetPtr = TargetContainer->GetSlot(SlotTargetId);

		bool bInternalTransfer = (TargetContainer == SourceContainer);

		FContainerStack NewSourceStack;
		FContainerStack NewTargetStack;

		if (StackTargetPtr) {
			NewTargetStack = *StackTargetPtr;
		}

		if (StackSourcePtr) {
			NewSourceStack = *StackSourcePtr;
		}

		bool bResult = false;

		APawn* LocalPawn = (APawn*)TargetContainer->GetOwner();
		if (LocalPawn) {
			ASandboxPlayerController* LocalController = Cast<ASandboxPlayerController>(LocalPawn->GetController());
			if (LocalController) {
				const FString ContainerName = GetName();
				const ASandboxObject* Obj = StackSourcePtr->GetObject();
				bool isValid = LocalController->OnContainerDropCheck(SlotTargetId, *ContainerName, Obj);
				if (!isValid) {
					return false;
				}
			}
		}

		if (IsSameObject(StackSourcePtr, StackTargetPtr)) {
			const ASandboxObject* Obj = (ASandboxObject*)StackTargetPtr->GetObject();
			if (StackTargetPtr->Amount < (int)Obj->MaxStackSize) {
				uint32 ChangeAmount = (bOnlyOne) ? 1 : StackSourcePtr->Amount;
				uint32 NewAmount = StackTargetPtr->Amount + ChangeAmount;

				if (NewAmount <= Obj->MaxStackSize) {
					NewTargetStack.Amount = NewAmount;
					NewSourceStack.Amount -= ChangeAmount;
				} else {
					int D = NewAmount - Obj->MaxStackSize;
					NewTargetStack.Amount = Obj->MaxStackSize;
					NewSourceStack.Amount = D;
				}

				bResult = true;
			}
		} else {
			if (bOnlyOne) {
				if (!SourceContainer->IsSlotEmpty(SlotSourceId)) {
					if (TargetContainer->IsSlotEmpty(SlotTargetId)) {
						NewTargetStack = NewSourceStack;
						NewTargetStack.Amount = 1;
						NewSourceStack.Amount--;
						bResult = true;
					}
				}
			} else {
				std::swap(NewTargetStack, NewSourceStack);
				bResult = true;
			}
		}

		if (bResult) {
			if (GetNetMode() == NM_Client) {
				ASandboxPlayerController* LocalController = Cast<ASandboxPlayerController>(UGameplayStatics::GetPlayerController(GetWorld(), 0));
				if (LocalController) {
					if (bInternalTransfer) {
						//TargetContainer->SetStackDirectly(NewTargetStack, SlotTargetId);
						//SourceContainer->SetStackDirectly(NewSourceStack, SlotSourceId);
					} 

					ASandboxObject* TargetObj = Cast<ASandboxObject>(TargetContainer->GetOwner());
					if (TargetObj) {
						NetObjectTransfer(LocalController, TargetObj, TargetContainer, NewTargetStack, SlotTargetId);
					}

					ASandboxObject* SourceObj = Cast<ASandboxObject>(SourceContainer->GetOwner());
					if (SourceObj) {
						NetObjectTransfer(LocalController, SourceObj, SourceContainer, NewSourceStack, SlotSourceId);
					}

					APawn* TargetPawn = Cast<APawn>(TargetContainer->GetOwner());
					if (TargetPawn) {
						NetControllerTransfer(LocalController, TargetContainer, NewTargetStack, SlotTargetId);
					}

					APawn* SourcePawn = Cast<APawn>(SourceContainer->GetOwner());
					if (SourcePawn) {
						NetControllerTransfer(LocalController, SourceContainer, NewSourceStack, SlotSourceId);
					}
				}
			} else {
				// server only
				TargetContainer->SetStackDirectly(NewTargetStack, SlotTargetId);
				SourceContainer->SetStackDirectly(NewSourceStack, SlotSourceId);
			}
		}

		bUpdated = bResult;
		return bResult;

}

void UContainerComponent::OnRep_Content() {
	UE_LOG(LogTemp, Warning, TEXT("OnRep_Content %s"), *GetName());
	bUpdated = true;
}

bool UContainerComponent::IsUpdated() {
	return bUpdated;
}

void UContainerComponent::ResetUpdatedFlag() {
	bUpdated = false;
}
