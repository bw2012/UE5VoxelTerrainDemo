// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.
#pragma once
#include "GameFramework/PlayerController.h"
#include "SandboxPlayerController.h"
#include "SandboxEnvironment.h"
#include "EnvironmentController.h"
#include "LevelController.h"
#include "TerrainController.h"
#include "DummyPawn.h"
#include "MainPlayerControllerComponent.h"
#include "Engine/DataTable.h"
#include <memory>
#include "MainPlayerController.generated.h"


USTRUCT(BlueprintType)
struct DARKLITHOSPHERE_API FCraftRecipePart {
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	FString Name;

	UPROPERTY(EditAnywhere)
	uint32 Amount = 0;

	UPROPERTY(EditAnywhere)
	TArray<uint64> SandboxIdList;
};


USTRUCT(BlueprintType)
struct DARKLITHOSPHERE_API FCraftRecipe : public FTableRowBase {
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	UTexture2D* IconTexture;

	UPROPERTY(EditAnywhere)
	uint64 SandboxClassId;

	UPROPERTY(EditAnywhere)
	FString Name;

	UPROPERTY(EditAnywhere)
	bool bOnlyOne = true;

	UPROPERTY(EditAnywhere)
	bool bToInventory = false;

	UPROPERTY(EditAnywhere)
	TArray<FCraftRecipePart> Parts;

	UPROPERTY(EditAnywhere)
	int Qty = 1;
};


class FDummyPawnHelper; // linux


UCLASS()
class DARKLITHOSPHERE_API AMainPlayerController : public ASandboxPlayerController
{
	GENERATED_BODY()

	friend class FDummyPawnHelper;
	friend class UMainPlayerControllerComponent;


public:
	UPROPERTY(EditAnywhere, Category = "Sandbox")
	TSubclassOf<ADummyPawn> DummyPawnClass;

	UPROPERTY(EditAnywhere, Category = "Sandbox")
	UMaterialInstance* CursorMaterial;

	UPROPERTY(EditAnywhere, Category = "Sandbox")
	UMaterialInstance* CursorNokMaterial;

	UPROPERTY(BlueprintReadOnly, Category = "Sandbox")
	UMainPlayerControllerComponent* MainPlayerControllerComponent;

	UPROPERTY(EditAnywhere, Category = "Sandbox")
	class UDataTable* CraftRecipeTable;

	UPROPERTY(EditAnywhere, Category = "Sandbox")
	int32 NewCharacterId;

	UPROPERTY(EditAnywhere, Category = "Sandbox")
	bool bSpawnSandboxCharacter;

	FObjectInfo UiHoverObject;

public:

	AMainPlayerController();

	virtual void BeginPlay() override;

	virtual void OnPossess(APawn* NewPawn) override;

	virtual void OnRep_Pawn() override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:

	UFUNCTION(BlueprintCallable, Category = "Sandbox")
	void ToggleToolMode();

	UFUNCTION(BlueprintCallable, Category = "Sandbox")
	void PossessDummyPawn();

	UFUNCTION(BlueprintCallable, Category = "Sandbox")
	void SetSandboxMode(int Mode);

	UFUNCTION(BlueprintCallable, Category = "Sandbox")
	void SetSandboxModeExtId(int Id);

	UFUNCTION(BlueprintCallable, Category = "Sandbox")
	void SetSandboxExtPage(int Page);

	UFUNCTION(BlueprintCallable, Category = "Sandbox")
	int GetSandboxExtPage();

	int GetSandboxModeExtByPage(int Id);

	UFUNCTION(BlueprintCallable, Category = "Sandbox")
	void TestRemoveSandboxObject();

	void UnPossessDummyPawn(APawn* NewPawn);

	UFUNCTION(BlueprintCallable, Category = "Sandbox")
	void ChangeDummyCameraAltitude(float Val);

	UFUNCTION(BlueprintCallable, Category = "Sandbox")
	void OpenMainInventoryGui(FName ExtWidget = NAME_None);

	UFUNCTION(BlueprintCallable, Category = "Sandbox")
	void CloseMainInventoryGui();

	FCraftRecipe* GetCraftRecipeData(int32 RecipeId);

	bool ValidateCraftItems(int32 RecipeId);

	bool ValidateCraftItemPart(int32 RecipeId, int Part);

	bool SpentCraftRecipeItems(int32 RecipeId);

	virtual	void SetCurrentInventorySlot(int32 Slot) override;

	virtual bool OnContainerSlotHover(int32 SlotId, FName ContainerName) override;

	void OnDeath();

	UFUNCTION(BlueprintCallable, Category = "Sandbox")
	void SetExtMode(int Index);

	UFUNCTION(BlueprintCallable, Category = "Sandbox")
	void EnableGuiMode();

	UFUNCTION(BlueprintCallable, Category = "Sandbox")
	void DisableGuiMode();

	UFUNCTION(BlueprintCallable, Category = "Sandbox")
	void ToggleMainInventory();

	UFUNCTION(BlueprintCallable, Category = "Sandbox")
	virtual void OnMainActionPressed();

	UFUNCTION(BlueprintCallable, Category = "Sandbox")
	virtual void OnMainActionReleased();

	UFUNCTION(BlueprintCallable, Category = "Sandbox")
	virtual void OnAltActionPressed();

	UFUNCTION(BlueprintCallable, Category = "Sandbox")
	virtual void OnAltActionReleased();

	UFUNCTION(BlueprintCallable, Category = "Sandbox")
	virtual void OnWheelUp();

	UFUNCTION(BlueprintCallable, Category = "Sandbox")
	virtual void OnWheelDown();

	UFUNCTION(BlueprintCallable, Category = "Sandbox")
	void Respawn();

	UFUNCTION(Exec, Category = "Sandbox")
	void SandboxAddItem(int ItemId);

	UFUNCTION(Exec, Category = "Sandbox")
	void SandboxTp(int X, int Y, int Z);

	UFUNCTION(Exec, Category = "Sandbox")
	void SandboxExec(const FString& Cmd, const FString& Param);

	UFUNCTION(Exec, Category = "Sandbox")
	void SandboxRebuildEnergyNet();

	UFUNCTION(Exec, Category = "Sandbox")
	void SandboxForceSave();

	UFUNCTION(Server, Reliable)
	void ServerRebuildEnergyNet();

	UFUNCTION(Server, Reliable)
	void ServerRpcForceSave();

	UFUNCTION(Server, Reliable)
	void ServerRpcAddItem(int ItemId);

	UFUNCTION(Server, Reliable)
	void ServerRpcAddItemOrSpawnObject(int ItemId, FTransform Transform);

	UFUNCTION(Server, Reliable)
	void ServerRpcTp(int X, int Y, int Z);

	UFUNCTION(Server, Reliable)
	void ServerRpcFindOrCreateCharacter();

	UFUNCTION(Server, Reliable)
	void ServerRpcRegisterSandboxPlayer(const FString& NewSandboxPlayerUid, const FString& ClientSoftwareVersion);

	UFUNCTION(Server, Reliable)
	void ServerRpcRebuildEquipment();

	UFUNCTION(Server, Reliable)
	void ServerRpcDestroyTerrainMesh(int32 X, int32 Y, int32 Z, uint32 TypeId, uint32 VariantId, int32 Item, int EffectId, FVector EffectOrigin);

	UFUNCTION(Server, Reliable)
	void ServerRpcDigTerrain(int32 Type, FVector DigOrigin, FVector Origin, float Size, int32 X, int32 Y, int32 Z, int32 FaceIndex);

	UFUNCTION(Server, Reliable)
	void ServerRpcDestroyActor(int32 X, int32 Y, int32 Z, const FString& Name, FVector Origin, uint32 EffectId = 0);

	UFUNCTION(Server, Reliable)
	void ServerRpcDestroyActorByNetUid(uint64 NetUid, FVector EffectOrigin = FVector(), uint32 EffectId = 0);

	UFUNCTION(Server, Reliable)
	void ServerRpcRemoveActor(ASandboxObject* Obj);

	UFUNCTION(Server, Reliable)
	void ServerRpcSpawnObject(uint64 SandboxClassId, const FTransform& Transform, bool bEnablePhysics);

	UFUNCTION(Reliable, Client)
	void ClientRpcRegisterFinished();

	void ClientWasKicked_Implementation(const FText& KickReason);

	FHitResult TracePlayerActionPoint();

	ALevelController* GetLevelController();

	bool IsGuiMode();

	void OnContainerMainAction(int32 SlotId, FName ContainerName) override;

	void OnContainerDropSuccess(int32 SlotId, FName SourceName, FName TargetName) override;

	bool OnContainerDropCheck(int32 SlotId, FName ContainerName, const ASandboxObject* Obj) const override;

	void SetCursorMesh(UStaticMesh* Mesh, const FVector& Location, const FRotator& Rotation, const FVector& Scale);

	void RemoveTerrainMesh(UTerrainInstancedStaticMesh* TerrainMesh, int32 ItemIndex);

protected:

	void OnFinishInitialLoad();

	void OnStartBackgroundSave();

	void OnFinishBackgroundSave();

	void FindOrCreateCharacterInternal();

	virtual void PlayerTick(float DeltaTime) override;

	virtual void SetupInputComponent() override;

	virtual void OnSelectActionObject(AActor* Actor) override;

	virtual void OnDeselectActionObject(AActor* Actor) override;

	void OnMainInteractionPressed();

	void SandboxPossess(ACharacter* Character);

	void SetupCamera();

	void ShowInGameHud();

	template<class T>
	T* GetFirstComponentByName(FString ComponentName) {
		TArray<T*> Components;
		GetComponents<T>(Components);
		for (T* Component : Components) {
			if (Component->GetName() == ComponentName)
				return Component;
		}

		return nullptr;
	}

	template<class T>
	T* GetFirstComponentByName(AActor* Actor, FString ComponentName) {
		TArray<T*> Components;
		Actor->GetComponents<T>(Components);
		for (T* Component : Components) {
			if (Component->GetName() == ComponentName)
				return Component;
		}

		return nullptr;
	}

	void SpawnObjectByPlayer(uint64 SandboxClassId, FTransform Transform);

private:
	bool bGuiMode = false;

	UPROPERTY()
	AEnvironmentController* Environment;

	UPROPERTY()
	ATerrainController* TerrainController;
	
	FTimerHandle Timer;

	ASandboxObject* GetInventoryObject(int32 SlotId);

	ASandboxObject* GetCurrentInventoryObject();

	FVector PrevLocation;

	FVector LastCursorPosition;

	std::shared_ptr<FDummyPawnHelper> DummyPawnHelperPtr;

	APawn* SelectedPawn;

	ASandboxObject* SelectedObj;

	ASandboxObject* PickedObj;

	int SandboxMode = 0;

	int SandboxModeExtId = 0;

	int SandboxExtPage = 0;

	int SandboxMaxPage = 2;

	void ResetAllSelections();

	bool bFirstStart = false;

	bool bClientPosses = false;

	FPlayerInfo PlayerInfo;

};


