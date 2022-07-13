// Copyright blackw 2015-2020

#pragma once

#include "Engine.h"
#include "Components/ActorComponent.h"
#include "ContainerComponent.generated.h"

class ASandboxObject;

USTRUCT()
struct FContainerStack {
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere)
	int32 Amount;

	UPROPERTY(EditAnywhere)
	TSubclassOf<ASandboxObject>	ObjectClass;

	FContainerStack() {
		Clear();
	}

	void Clear() {
		Amount = 0;
		ObjectClass = nullptr;
	}

	ASandboxObject* GetObject() {
		if (ObjectClass) {
			return (ASandboxObject*)(ObjectClass->GetDefaultObject());
		}

		return nullptr;
	}

	bool IsEmpty() {
		return Amount == 0;
	}

};


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class UNREALSANDBOXTOOLKIT_API UContainerComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UContainerComponent();

	// Called when the game starts
	virtual void BeginPlay() override;
	
	// Called every frame
	virtual void TickComponent( float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction ) override;

	UPROPERTY(Replicated, EditAnywhere, Category = "Sandbox")
	TArray<FContainerStack> Content;
    
	bool AddStack(const FContainerStack& Stack, const int SlotId);

	bool AddObject(ASandboxObject* Obj);
    
	FContainerStack* GetSlot(const int Slot);
    
	void ChangeAmount(int Slot, int Num);

    bool DecreaseObjectsInContainer(int slot, int num);
    
    ASandboxObject* GetSandboxObject(int SlotId);

	bool IsEmpty();

	bool IsSlotEmpty(int SlotId);

	void CopyTo(UContainerComponent* Target);

	ASandboxObject* GetAvailableSlotObject(const int Slot);

private:


};
