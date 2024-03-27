// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Runtime/UMG/Public/UMG.h"
#include "Runtime/UMG/Public/UMGStyle.h"
#include "Runtime/UMG/Public/Slate/SObjectWidget.h"
#include "Runtime/UMG/Public/IUMGModule.h"
#include "Runtime/UMG/Public/Blueprint/UserWidget.h"

#include "Blueprint/UserWidget.h"
#include "ObjectContainerCellWidget.generated.h"


class UContainerComponent;


UENUM(BlueprintType)
enum class EContainerCellBinding : uint8 {
	Player = 0				UMETA(DisplayName = "Player"),
	ExternalObject = 1		UMETA(DisplayName = "External Object"),
};


/**
 * 
 */
UCLASS()
class UNREALSANDBOXTOOLKIT_API USandboxObjectContainerCellWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:	

	UPROPERTY(EditAnywhere, Category = "SandboxInventory")
	FName ContainerName;

	UPROPERTY(EditAnywhere, Category = "SandboxInventory")
	EContainerCellBinding CellBinding;

	UFUNCTION(BlueprintCallable, Category = "SandboxInventory")
	FLinearColor SlotBorderColor(int32 SlotId);

	UFUNCTION(BlueprintCallable, Category = "SandboxInventory")
	void SelectSlot(int32 SlotId);

	UFUNCTION(BlueprintCallable, Category = "SandboxInventory")
	void HoverSlot(int32 SlotId);

	UFUNCTION(BlueprintCallable, Category = "SandboxInventory")
	UTexture2D* GetSlotTexture(int32 SlotId);

	UFUNCTION(BlueprintCallable, Category = "SandboxInventory")
	FString SlotGetAmountText(int32 SlotId);

	UFUNCTION(BlueprintCallable, Category = "SandboxInventory")
	bool SlotDrop(int32 SlotDropId, int32 SlotTargetId, AActor* SourceActor, UContainerComponent* SourceContainer, bool bOnlyOne = false);

	UFUNCTION(BlueprintCallable, Category = "SandboxInventory")
	bool SlotIsEmpty(int32 SlotId);

	UFUNCTION(BlueprintCallable, Category = "SandboxInventory")
	void HandleSlotMainAction(int32 SlotId);

	UFUNCTION(BlueprintCallable, Category = "SandboxInventory")
	AActor* GetOpenedObject();
	
	UFUNCTION(BlueprintCallable, Category = "SandboxInventory")
	UContainerComponent* GetOpenedContainer();

protected:
	UContainerComponent* GetContainer();

	bool IsExternal();

private:

	bool SlotDropInternal(int32 SlotDropId, int32 SlotTargetId, AActor* SourceActor, UContainerComponent* SourceContainer, bool bOnlyOne);
};
