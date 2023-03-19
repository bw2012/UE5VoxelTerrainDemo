// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.
#pragma once
#include "GameFramework/PlayerController.h"
#include "SandboxPlayerController.h"
#include "SandboxEnvironment.h"
#include "LevelController.h"
#include "TerrainController.h"
#include "DummyPawn.h"
#include "MainPlayerControllerComponent.h"
#include <memory>
#include "MainPlayerController.generated.h"


USTRUCT()
struct FCraftPartData {
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	uint32 SandboxClassId;

	UPROPERTY(EditAnywhere)
	uint32 Amount;
};


USTRUCT()
struct FCraftRecipeData {
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	UTexture2D* IconTexture;

	UPROPERTY(EditAnywhere)
	uint32 SandboxClassId;

	UPROPERTY(EditAnywhere)
	FString Name;

	UPROPERTY(EditAnywhere)
	bool bOnlyOne = true;

	UPROPERTY(EditAnywhere)
	bool bToInventory = false;

	UPROPERTY(EditAnywhere)
	TArray<FCraftPartData> Parts;
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
	TMap<int32, FCraftRecipeData> CraftRecipeMap;

	UPROPERTY(EditAnywhere, Category = "Sandbox")
	int32 NewCharacterId;

	UPROPERTY(EditAnywhere, Category = "Sandbox")
	bool bSpawnSandboxCharacter;

public:

	AMainPlayerController();

	virtual void BeginPlay() override;

	virtual void OnPossess(APawn* NewPawn) override;

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
	void TestRemoveSandboxObject();

	void UnPossessDummyPawn(APawn* NewPawn);

	UFUNCTION(BlueprintCallable, Category = "Sandbox")
	void ChangeDummyCameraAltitude(float Val);

	UFUNCTION(BlueprintCallable, Category = "Sandbox")
	void OpenMainInventoryGui(FName ExtWidget = NAME_None);

	UFUNCTION(BlueprintCallable, Category = "Sandbox")
	void CloseMainInventoryGui();

	FCraftRecipeData* GetCraftRecipeData(int32 RecipeId);

	virtual	void SetCurrentInventorySlot(int32 Slot) override;

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

	UFUNCTION(Server, Reliable)
	void FindOrCreateCharacter();

	UFUNCTION(Server, Reliable)
	void RegisterSandboxPlayerUid(const FString& NewSandboxPlayerUid);

	UFUNCTION(Server, Reliable)
	void RebuildEquipment();

	FHitResult TracePlayerActionPoint();

	ALevelController* GetLevelController();

	bool IsGuiMode();

	void OnContainerMainAction(int32 SlotId, FName ContainerName) override;

	void OnContainerDropSuccess(int32 SlotId, FName SourceName, FName TargetName) override;

	bool OnContainerDropCheck(int32 SlotId, FName ContainerName, const ASandboxObject* Obj) const override;

	void SetCursorMesh(UStaticMesh* Mesh, const FVector& Location, const FRotator& Rotation, const FVector& Scale);

protected:

	void OnFinishInitialLoad();

	void FindOrCreateCharacterInternal();

	virtual void PlayerTick(float DeltaTime) override;

	virtual void SetupInputComponent() override;

	//void PerformAction();

	//virtual void OnTracePlayerActionPoint(const FHitResult& Res) override;

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

private:
	bool bGuiMode = false;

	UPROPERTY()
	ASandboxEnvironment* SandboxEnvironment;

	UPROPERTY()
	ATerrainController* TerrainController;
	
	FTimerHandle Timer;

	ASandboxObject* GetInventoryObject(int32 SlotId);

	ASandboxObject* GetCurrentInventoryObject();

	FVector PrevLocation;

	FVector LastCursorPosition;

	std::shared_ptr<FDummyPawnHelper> DummyPawnHelperPtr;

	APawn* SelectedPawn;

	//APawn* PickedPawn;

	ASandboxObject* SelectedObj;

	ASandboxObject* PickedObj;

	int SandboxMode = 0;

	int SandboxModeExtId = 0;

	void ResetAllSelections();

	bool bFirstStart = false;

	bool bClientPosses = false;

	FPlayerInfo PlayerInfo;

};


