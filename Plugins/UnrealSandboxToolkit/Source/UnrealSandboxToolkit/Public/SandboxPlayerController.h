
#pragma once

#include "Engine.h"
#include "GameFramework/PlayerController.h"
#include "SandboxLevelController.h"
#include "SandboxPlayerController.generated.h"

class ASandboxObject;
class UUserWidget;
class UContainerComponent;

USTRUCT()
struct FPlayerInfo {
	GENERATED_BODY()

	UPROPERTY()
	FString PlayerUid;
};

UCLASS()
class UNREALSANDBOXTOOLKIT_API ASandboxPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, Category = "Sandbox")
	int32 CurrentInventorySlot;

	UPROPERTY(EditAnywhere, Category = "Sandbox")
	bool bIsAdmin;

	ASandboxPlayerController();

	virtual void OnPossess(APawn* NewPawn) override;

	UFUNCTION(BlueprintCallable, Category = "Sandbox")
	void BlockGameInput();

	UFUNCTION(BlueprintCallable, Category = "Sandbox")
	void UnblockGameInput();

	UFUNCTION(BlueprintCallable, Category = "Sandbox")
	virtual	void SetCurrentInventorySlot(int32 Slot);

	UFUNCTION(BlueprintCallable, Category = "Sandbox")
	bool TakeObjectToInventory();

	UFUNCTION(BlueprintCallable, Category = "Sandbox")
	bool OpenObjectContainer(ASandboxObject* Obj);

	UFUNCTION(BlueprintCallable, Category = "Sandbox")
	bool TraceAndOpenObjectContainer();

	UFUNCTION(BlueprintCallable, Category = "Sandbox")
	bool HasOpenContainer();

	UFUNCTION(BlueprintCallable, Category = "Sandbox")
	UContainerComponent* GetOpenedContainer();

	UFUNCTION(BlueprintCallable, Category = "Sandbox")
	ASandboxObject* GetOpenedObject();

	UFUNCTION(BlueprintCallable, Category = "Sandbox")
	void CloseObjectWithContainer();

	UFUNCTION(BlueprintCallable, Category = "Sandbox")
	FHitResult TracePlayerActionPoint();

	UFUNCTION(BlueprintCallable, Category = "Sandbox")
	void SelectActionObject(AActor* Actor);

	UFUNCTION(BlueprintCallable, Category = "Sandbox")
	bool IsGameInputBlocked();

	UFUNCTION(BlueprintCallable, Category = "Sandbox")
	void TraceAndSelectActionObject();

	UFUNCTION(Server, Reliable)
	void TransferContainerStack(uint64 ObjectNetUid, const FString& ContainerName, const FContainerStack& Stack, const int SlotId);

	UFUNCTION(Server, Reliable)
	void TransferInventoryStack(const FString& ContainerName, const FContainerStack& Stack, const int SlotId);

protected:
	/** True if the controlled character should navigate to the mouse cursor. */
	uint32 bMoveToMouseCursor : 1;

	// Begin PlayerController interface
	virtual void PlayerTick(float DeltaTime) override;
	virtual void SetupInputComponent() override;
	// End PlayerController interface

	/** Navigate player to the current mouse cursor location. */
	void MoveToMouseCursor();

	/** Navigate player to the current touch location. */
	void MoveToTouchLocation(const ETouchIndex::Type FingerIndex, const FVector Location);
	
	/** Navigate player to the given world location. */
	void SetNewMoveDestination(const FVector DestLocation);

	/** Input handlers for SetDestination action. */
	void SetDestinationPressed();

	void SetDestinationReleased();

	// ==========================================================================================================

	virtual void OnMainActionPressed();

	virtual void OnMainActionReleased();

	virtual void OnAltActionPressed();

	virtual void OnAltActionReleased();

	virtual void ToggleView();

	UContainerComponent* GetContainerByName(FName ContainerName);

	UContainerComponent* GetInventory();

	virtual void OnTracePlayerActionPoint(const FHitResult& Res);

	virtual void OnSelectActionObject(AActor* Actor);

	virtual void OnDeselectActionObject(AActor* Actor);

	UPROPERTY()
	ASandboxLevelController* LevelController;

public:

	void ShowMouseCursor(bool bShowCursor);

	virtual void OnContainerMainAction(int32 SlotId, FName ContainerName);

	virtual void OnContainerDropSuccess(int32 SlotId, FName SourceName, FName TargetName);

	virtual bool OnContainerDropCheck(int32 SlotId, FName ContainerName, const ASandboxObject* Obj) const;

	virtual bool OnContainerSlotHover(int32 SlotId, FName ContainerName);

	ASandboxLevelController* GetLevelController();

private:

	void OnMainActionPressedInternal();
	
	void OnMainActionReleasedInternal();

	void OnAltActionPressedInternal();

	void OnAltActionReleasedInternal();


	FHitResult CurrentPos;

	bool bIsGameInputBlocked;

	UPROPERTY()
	ASandboxObject* SelectedObject;

	UPROPERTY()
	ASandboxObject* OpenedObject;

	UPROPERTY()
	UContainerComponent* OpenedContainer;
};


