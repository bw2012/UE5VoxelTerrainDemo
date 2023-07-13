

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SandboxObject.h"
#include "MainPlayerControllerComponent.generated.h"


class UTerrainInstancedStaticMesh;

UENUM(BlueprintType)
enum class ECurrentActionType : uint8 {
	None = 0				UMETA(DisplayName = "None"),
	PlaceObjectToWorld = 1	UMETA(DisplayName = "Place object to world"),
	PlaceCraftToWorld =	2	UMETA(DisplayName = "Place craft to world"),
};

UENUM(BlueprintType)
enum class ESelectedObjectType : uint8 {
	None = 0				UMETA(DisplayName = "None"),
	SandboxObject = 1		UMETA(DisplayName = "SandboxObject"),
	InstancedMesh = 2		UMETA(DisplayName = "InstancedMesh"),
};


USTRUCT()
struct FSelectedObject {
	GENERATED_BODY()

	UPROPERTY()
	ASandboxObject* SandboxObj = nullptr;

	UPROPERTY()
	UTerrainInstancedStaticMesh* TerrainMesh = nullptr;

	UPROPERTY()
	ESelectedObjectType ObjType;

	UPROPERTY()
	FString Name;
};



UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class DARKLITHOSPHERE_API UMainPlayerControllerComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UMainPlayerControllerComponent();

	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	

	UFUNCTION(BlueprintCallable, Category = "Sandbox")
	bool PlaceCurrentObjectToWorld();

	bool PlaceCraftedObjectToWorld();

	void PerformMainAction();

	void OnPlayerTick();

	void PerformAltAction();

	void EndAltAction();

	//void OnTracePlayerActionPoint(const FHitResult& Res);

	void ResetState();

	void SetActionType(ECurrentActionType ActionType);

	UFUNCTION(BlueprintCallable, Category = "Sandbox")
	void ToggleActionPlaceObjectToWorld();

	void TakeSelectedObjectToInventory();

	void SelectActionObject();

	void MainInteraction();

	void OnDeath();

	bool ToggleCraftMode(int ReceiptId);

	void OnSelectCurrentInventorySlot(int SlotId);

	FSelectedObject SelectedObject;

	void OnInventoryItemMainAction(int32 SlotId);

	void OnWheelUp();

	void OnWheelDown();

private:

	ECurrentActionType CurrentActionType = ECurrentActionType::None;

	bool CanPlaceObjectToWorld(const ASandboxObject* Obj) const;

	int32 CraftReceiptId = 0;

	void ResetSelectedObject();

	bool bAltActionPressed = false;

	volatile double Timestamp = 0.f;
		
protected:

	UFUNCTION(Server, Reliable)
	void ServerRpcDecreaseObjectsInContainer(const FString& Name, int Slot);

	UFUNCTION(Server, Reliable)
	void ServerRpcObjMainInteraction(ASandboxObject* Obj);

	UFUNCTION(Server, Reliable)
	void ServerRpcDoorInteraction(ASandboxObject* Door, const FVector& PlayerPos);

};
