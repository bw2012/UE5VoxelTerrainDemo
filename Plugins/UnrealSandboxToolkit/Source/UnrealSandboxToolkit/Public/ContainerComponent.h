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
	uint64 SandboxClassId;

	FContainerStack() {
		Clear();
	}

	FContainerStack(int32 Amount_, uint64 SandboxClassId_) : Amount(Amount_), SandboxClassId(SandboxClassId_) { }

	void Clear() {
		Amount = 0;
		SandboxClassId = 0;
	}

	bool IsEmpty() {
		return Amount == 0;
	}

	const ASandboxObject* GetObject() const;
};


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class UNREALSANDBOXTOOLKIT_API UContainerComponent : public UActorComponent
{
	GENERATED_BODY()


public:
	UPROPERTY(ReplicatedUsing = OnRep_Content, EditAnywhere, Category = "Sandbox")
	TArray<FContainerStack> Content;

public:	
	UContainerComponent();

	virtual void BeginPlay() override;
	
	virtual void TickComponent( float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction ) override;

	UFUNCTION()
	void OnRep_Content();
    
	bool SetStackDirectly(const FContainerStack& Stack, const int SlotId);

	bool AddObject(ASandboxObject* Obj);
    
	FContainerStack* GetSlot(const int Slot);
    
	void ChangeAmount(int Slot, int Num);

    bool DecreaseObjectsInContainer(int slot, int num);
    
    ASandboxObject* GetSandboxObject(int SlotId);

	bool IsEmpty();

	bool IsSlotEmpty(int SlotId);

	void CopyTo(UContainerComponent* Target);

	TArray<uint64> GetAllObjects();

	bool SlotTransfer(int32 SlotDropId, int32 SlotTargetId, AActor* SourceActor, UContainerComponent* SourceContainer, bool bOnlyOne = false);

	bool IsUpdated();

	void ResetUpdatedFlag();

private:

	bool bUpdated = false;

};

