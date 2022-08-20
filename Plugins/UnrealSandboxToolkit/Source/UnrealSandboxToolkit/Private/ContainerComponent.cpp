// Copyright blackw 2015-2020


#include "ContainerComponent.h"
#include "SandboxObject.h"
//#include "SandboxCharacter.h"
#include "Net/UnrealNetwork.h"


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

bool UContainerComponent::AddStack(const FContainerStack& Stack, const int SlotId) {
	if (SlotId >= Content.Num()) {
		Content.SetNum(SlotId + 1);
	}

	FContainerStack* StackPtr = &Content[SlotId];
	StackPtr->Amount = Stack.Amount;
	StackPtr->ObjectClass = Stack.ObjectClass;

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
			if (Stack->ObjectClass != nullptr && MaxStackSize > 1) {
				if (Stack->ObjectClass->GetName().Equals(Obj->GetClass()->GetName())) {
					Stack->Amount++;
					bIsAdded = true;
					break;
				}
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
			Stack->ObjectClass = Obj->GetClass();
		} else {
			FContainerStack NewStack;
			NewStack.Amount = 1;
			NewStack.ObjectClass = Obj->GetClass();
			Content.Add(NewStack);
		}
	}
	
	return true;
}

FContainerStack* UContainerComponent::GetSlot(const int Slot) {
	if (!Content.IsValidIndex(Slot)) {
		return nullptr;
	}

	return &Content[Slot];
}

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
}

TArray<ASandboxObject*> UContainerComponent::GetAllObjects() {
	TArray<ASandboxObject*> Result;
	for (int Idx = 0; Idx < Content.Num(); Idx++) {
		FContainerStack* Stack = &Content[Idx];
		if (Stack) {
			ASandboxObject* Obj = Stack->GetObject();
			if (Obj) {
				Result.Add(Obj);
			}
		}


	}

	return Result;
}
