
#include "MainPlayerControllerComponent.h"
#include "MainPlayerController.h"
#include "BaseCharacter.h"
#include "Objects/MiningTool.h"
#include "Objects/BaseObject.h"
#include "Objects/Door.h"
#include "SpawnHelper.h"
#include "TerrainZoneComponent.h"
#include "VitalSystemComponent.h"


//bool IsCursorPositionValid(const FHitResult& Hit);
void CalculateCursorPosition(ABaseCharacter* Character, const FHitResult& Res, FVector& Location, FRotator& Rotation, ASandboxObject* Obj);

UVitalSystemComponent* GetVitalSystemComponent(ACharacter* Character) {
	TArray<UVitalSystemComponent*> Components;
	Character->GetComponents<UVitalSystemComponent>(Components);
	for (UVitalSystemComponent* VitalSystemComponent : Components) {
		return VitalSystemComponent;
	}

	return nullptr;
}

bool PerformMining(ABaseCharacter* BaseCharacter, AMiningTool* MiningTool, const FHitResult& Hit) {
	UVitalSystemComponent* VitalSystemComponent = GetVitalSystemComponent(BaseCharacter);
	if (VitalSystemComponent) {
		bool bCanUse = VitalSystemComponent->CheckStamina(2.f);
		if (!bCanUse) {
			return false;
		}
	}

	MiningTool->OnAltAction(Hit, BaseCharacter);
	if (VitalSystemComponent) {
		VitalSystemComponent->ChangeStamina(-20.f);
	}

	return true;
}


UMainPlayerControllerComponent::UMainPlayerControllerComponent() {
	PrimaryComponentTick.bCanEverTick = true;
}

void UMainPlayerControllerComponent::BeginPlay(){
	Super::BeginPlay();
}

void UMainPlayerControllerComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) {
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

void SetRenderCustomDepth(AActor* Actor, bool RenderCustomDepth) {
	TArray<UMeshComponent*> MeshComponentList;
	Actor->GetComponents<UMeshComponent>(MeshComponentList);

	for (UMeshComponent* MeshComponent : MeshComponentList) {
		MeshComponent->SetRenderCustomDepth(RenderCustomDepth);
	}
}

bool UMainPlayerControllerComponent::CanPlaceObjectToWorld(const ASandboxObject* Obj) const {
	const AMiningTool* MiningTool = Cast<AMiningTool>(Obj);
	if (MiningTool) {
		return false;
	}

	return true;
}

void ResetCursorMesh(ABaseCharacter* BaseCharacter) {
	BaseCharacter->ResetCursorMesh();
}

void UMainPlayerControllerComponent::OnPlayerTick() {
	AMainPlayerController* MainController = (AMainPlayerController*)GetOwner();
	ABaseCharacter* BaseCharacter = Cast<ABaseCharacter>(MainController->GetCharacter());
	if (!BaseCharacter) {
		return;
	}

	if (BaseCharacter->IsDead()) {
		return;
	}

	bool bRepeatingAction = false;

	if (bAltActionPressed) {
		double T = FPlatformTime::Seconds();
		double Delta = T - Timestamp;
		if (Delta >= 0.25) {
			Timestamp = T;
			bRepeatingAction = true;
			//UE_LOG(LogTemp, Warning, TEXT("Timestamp2 %f"), Timestamp);
			//UE_LOG(LogTemp, Warning, TEXT("Delta %f"), Delta);
		}
	}

	bool bShowMeshCursor = false;

	ASandboxObject* Obj = MainController->GetCurrentInventoryObject();
	if (Obj) {
		AMiningTool* MiningTool = Cast<AMiningTool>(Obj);
		if (MiningTool) {
			FHitResult Hit = MainController->TracePlayerActionPoint();
			if (Hit.bBlockingHit) {
				if (bRepeatingAction) {
					PerformMining(BaseCharacter, MiningTool, Hit);
				}

				bShowMeshCursor = MiningTool->OnTracePlayerActionPoint(Hit, BaseCharacter);

			}
		}
	}

	BaseCharacter->CursorToWorld->SetVisibility(false);

	FHitResult Res = MainController->TracePlayerActionPoint();

	/*
	if (Res.bBlockingHit) {
		if (Cast<ATerrainController>(Res.GetActor())) {
			UTerrainInstancedStaticMesh* TerrainInstMesh = Cast<UTerrainInstancedStaticMesh>(Res.GetComponent());
			if (TerrainInstMesh) {
				//UE_LOG(LogTemp, Warning, TEXT("TerrainInstMesh %d"), TerrainInstMesh->MeshTypeId);
			}
		}
	}
	*/

	bool bPlaceToWorld = CurrentActionType == ECurrentActionType::PlaceObjectToWorld || CurrentActionType == ECurrentActionType::PlaceCraftToWorld;
	bool bIsValid = false;

	//DrawDebugPoint(GetWorld(), Res.Location, 5.f, FColor(255, 255, 255, 0), false, 1);

	if (bPlaceToWorld && IsCursorPositionValid(Res)) {
		ASandboxObject* BaseObj = nullptr;

		if (CurrentActionType == ECurrentActionType::PlaceObjectToWorld) {
			BaseObj = Obj;
		}

		if (CurrentActionType == ECurrentActionType::PlaceCraftToWorld) {
			FCraftRecipe* CraftRecipeData = MainController->GetCraftRecipeData(CraftReceiptId);
			if (CraftRecipeData) {
				TSubclassOf<ASandboxObject> ObjSubclass = MainController->LevelController->GetSandboxObjectByClassId(CraftRecipeData->SandboxClassId);
				if (ObjSubclass) {
					BaseObj = Cast<ASandboxObject>(ObjSubclass->GetDefaultObject());
				}
			}
		}

		if (BaseObj) {
			auto MeshComponent = BaseObj->GetMarkerMesh();
			if (MeshComponent) {
				FVector CompLocation = MeshComponent->GetRelativeLocation();
				FRotator ComponentRotation = MeshComponent->GetRelativeRotation();

				auto Mesh = MeshComponent->GetStaticMesh();
				FVector Location(0);
				FRotator Rotation(0);

				FVector CamLoc;
				FRotator CamRot;
				MainController->GetPlayerViewPoint(CamLoc, CamRot);

				CamRot.Pitch = 0;
				CamRot.Roll = 0;

				bIsValid = BaseObj->PlaceToWorldClcPosition(GetWorld(), CamLoc, CamRot, Res, Location, Rotation);
				if (bIsValid) {
					Location += Rotation.RotateVector(CompLocation);

					FVector Scale = BaseObj->GetRootComponent()->GetRelativeScale3D();
					MainController->SetCursorMesh(Mesh, Location, Rotation, Scale);
				}
			}
		}
	}

	if (bIsValid) {
		// TODO something
	} else {
		if (!bShowMeshCursor) {
			ResetCursorMesh(BaseCharacter);
		}
	}
	
	// empty hand or not useful object in hand
	SelectActionObject();
}

void UMainPlayerControllerComponent::PerformMainAction() {

	AMainPlayerController* MainController = (AMainPlayerController*)GetOwner();
	ABaseCharacter* BaseCharacter = Cast<ABaseCharacter>(MainController->GetCharacter());
	if (BaseCharacter) {
		BaseCharacter->PerformMainAttack();
	}

	ResetState();
}

void UMainPlayerControllerComponent::EndAltAction() {
	bAltActionPressed = false;
}

void UMainPlayerControllerComponent::PerformAltAction() {
	if (CurrentActionType == ECurrentActionType::PlaceObjectToWorld) {
		PlaceCurrentObjectToWorld();
		return;
	}

	if (CurrentActionType == ECurrentActionType::PlaceCraftToWorld) {
		PlaceCraftedObjectToWorld();
		return;
	}

	AMainPlayerController* MainController = (AMainPlayerController*)GetOwner();
	ASandboxObject* Obj = MainController->GetCurrentInventoryObject();
	if (Obj) {
		AMiningTool* MiningTool = Cast<AMiningTool>(Obj);
		if (MiningTool) {
			bAltActionPressed = true;
			Timestamp = FPlatformTime::Seconds();
			//UE_LOG(LogTemp, Warning, TEXT("Timestamp1 %f"), Timestamp);

			FHitResult Hit = MainController->TracePlayerActionPoint();
			if (Hit.bBlockingHit) {
				ABaseCharacter* BaseCharacter = Cast<ABaseCharacter>(MainController->GetCharacter());
				PerformMining(BaseCharacter, MiningTool, Hit);
			}

			return;
		}
	}

	ResetState();
}

void UMainPlayerControllerComponent::SelectActionObject() {
	AMainPlayerController* MainController = (AMainPlayerController*)GetOwner();
	if (!MainController) {
		return;
	}

	if (CurrentActionType != ECurrentActionType::None) {
		return;
	}

	bool NotSelected = true;
	FHitResult Hit = MainController->TracePlayerActionPoint();
	if (Hit.bBlockingHit) {
		ALevelController* LevelController = MainController->GetLevelController();

		ATerrainController* Terrain = Cast<ATerrainController>(Hit.GetActor());
		if(Terrain) {
			UTerrainInstancedStaticMesh* TerrainMesh = Cast<UTerrainInstancedStaticMesh>(Hit.GetComponent());
			if (TerrainMesh) {
				const FTerrainInstancedMeshType* TerrainMeshType = Terrain->GetInstancedMeshType(TerrainMesh->MeshTypeId, TerrainMesh->MeshVariantId);
				if (TerrainMeshType) {
					const FTerrainObjectInfo* ObjInfo = Terrain->GetInstanceObjStaticInfo(TerrainMeshType->MeshTypeId);

					if (ObjInfo) {
						//UE_LOG(LogTemp, Warning, TEXT("ObjInfo1 %s"), *ObjInfo->GetName());
						//UE_LOG(LogTemp, Warning, TEXT("ObjInfo2 %s"), *ObjInfo->CommonName);

						FSelectedObject NewSelectedObject;
						NewSelectedObject.TerrainMesh = TerrainMesh;
						NewSelectedObject.ObjType = ESelectedObjectType::InstancedMesh;
						NewSelectedObject.Name = ObjInfo->CommonName;
						NewSelectedObject.ExtText1 = ObjInfo->ChemicalFormula;
						NewSelectedObject.bCanTake = ObjInfo->bCanTake;


						/*
						TSubclassOf<ASandboxObject> ObjSubclass = LevelController->GetSandboxObjectByClassId(TerrainMeshType->SandboxClassId);
						ASandboxObject* Obj = (ASandboxObject*)ObjSubclass->GetDefaultObject();
						if (LevelController->ObjectMap) {
							FSandboxStaticData* StaticData = LevelController->ObjectMap->StaticData.Find(Obj->GetSandboxClassId());
							if (StaticData) {
								NewSelectedObject.Name = StaticData->ObjectName;
							}
						}
						*/

						if (SelectedObject.SandboxObj) {
							SetRenderCustomDepth(SelectedObject.SandboxObj, false);
						}

						SelectedObject = NewSelectedObject;
						NotSelected = false;
					}
				}
			}
		}

		ASandboxObject* Obj = Cast<ASandboxObject>(Hit.GetActor());
		if (Obj) {
			if (SelectedObject.SandboxObj) {
				SetRenderCustomDepth(SelectedObject.SandboxObj, false);
			}

			if (Obj->CanTake(MainController->GetCharacter())) {
				SetRenderCustomDepth(Obj, true);

				FSelectedObject NewSelectedObject;
				NewSelectedObject.SandboxObj = Obj;
				NewSelectedObject.ObjType = ESelectedObjectType::SandboxObject;

				if (LevelController->ObjectMap) {
					const FObjectInfo* ObjectInfo = LevelController->GetSandboxObjectStaticData(Obj->GetSandboxClassId());
					if (ObjectInfo) {
						NewSelectedObject.Name = ObjectInfo->CommonName;
						NewSelectedObject.ExtText1 = ObjectInfo->ChemicalFormula;
					}
				}

				SelectedObject = NewSelectedObject;
				NotSelected = false;
				//UE_LOG(LogTemp, Warning, TEXT("Obj: %s"), *Obj->GetName());
				//UE_LOG(LogTemp, Warning, TEXT("Obj - %f %f %f"), Obj->GetActorRotation().Pitch, Obj->GetActorRotation().Yaw, Obj->GetActorRotation().Roll);
			}

			ABaseObject* BaseObj = Cast<ABaseObject>(Hit.GetActor());
			if (BaseObj && BaseObj->IsContainer()) {
				FSelectedObject NewSelectedObject;
				NewSelectedObject.SandboxObj = Obj;
				NewSelectedObject.ObjType = ESelectedObjectType::SandboxObject;

				if (LevelController->ObjectMap) {
					const FObjectInfo* ObjectInfo = LevelController->GetSandboxObjectStaticData(Obj->GetSandboxClassId());
					if (ObjectInfo) {
						NewSelectedObject.Name = ObjectInfo->CommonName;
						NewSelectedObject.ExtText1 = ObjectInfo->ChemicalFormula;
					}
				}

				SelectedObject = NewSelectedObject;
				NotSelected = false;
			}
		} 
	}


	if (NotSelected) {
		ResetSelectedObject();
	}
}

void UMainPlayerControllerComponent::ResetSelectedObject() {
	if (SelectedObject.SandboxObj) {
		SetRenderCustomDepth(SelectedObject.SandboxObj, false);
	}

	FSelectedObject NewSelectedObject;
	NewSelectedObject.SandboxObj = nullptr;
	NewSelectedObject.TerrainMesh = nullptr;
	NewSelectedObject.ObjType = ESelectedObjectType::None;

	SelectedObject = NewSelectedObject;
}

bool UMainPlayerControllerComponent::PlaceCurrentObjectToWorld() {
	AMainPlayerController* MainController = (AMainPlayerController*) GetOwner();
	if (!MainController) {
		return false;
	}

	ABaseCharacter* BaseCharacter = Cast<ABaseCharacter>(MainController->GetCharacter());
	if (!BaseCharacter) {
		return false;
	}

	FHitResult Hit = MainController->TracePlayerActionPoint();
	if (Hit.bBlockingHit) {
		UE_LOG(LogTemp, Warning, TEXT("PlaceCurrentObjectToWorld -> %f %f %f"), Hit.ImpactPoint.X, Hit.ImpactPoint.Y, Hit.ImpactPoint.Z);

		if (CurrentActionType == ECurrentActionType::PlaceObjectToWorld) {
			UContainerComponent* Inventory = MainController->GetInventory();
			if (Inventory) {
				ASandboxObject* Obj = MainController->GetCurrentInventoryObject();
				if (IsCursorPositionValid(Hit) && Obj) {
					FVector Location(0);
					FRotator Rotation(0);

					FVector CamLoc;
					FRotator CamRot;
					MainController->GetPlayerViewPoint(CamLoc, CamRot);

					bool isPossible = Obj->PlaceToWorldClcPosition(GetWorld(), CamLoc, CamRot, Hit, Location, Rotation, true);
					if (isPossible) {
						FTransform Transform(Rotation, Location, FVector(1));

						if (GetNetMode() != NM_Client) {
							MainController->SpawnObjectByPlayer(Obj->GetSandboxClassId(), Transform);
						}

						// TODO ïåðåäåëàòü
						if (GetNetMode() == NM_Client) {
							MainController->ServerRpcSpawnObject(Obj->GetSandboxClassId(), Transform, false);
							ServerRpcDecreaseObjectsInContainer(TEXT("Inventory"), MainController->CurrentInventorySlot);
						}

						bool NotEmpty = Inventory->DecreaseObjectsInContainer(MainController->CurrentInventorySlot, 1);
						if (!NotEmpty) {
							ResetState();
						}

						return true;
					}
				}
			}
		}
	}

	return false;
}

void UMainPlayerControllerComponent::ServerRpcDecreaseObjectsInContainer_Implementation(const FString& Name, int Slot) {
	AMainPlayerController* MainController = (AMainPlayerController*) GetOwner();
	if (!MainController) {
		return;
	}

	UContainerComponent* Inventory = MainController->GetInventory();
	if (Inventory) {
		Inventory->DecreaseObjectsInContainer(Slot, 1);
	}
}

void UMainPlayerControllerComponent::ServerRpcObjMainInteraction_Implementation(ASandboxObject* Obj) {
	AMainPlayerController* MainController = (AMainPlayerController*)GetOwner();
	if (MainController) {
		Obj->MainInteraction(MainController->GetPawn());
	}
}

void UMainPlayerControllerComponent::ServerRpcDoorInteraction_Implementation(ASandboxObject* Obj, const FVector& PlayerPos) {
	ADoor* Door = Cast<ADoor>(Obj);
	if (Door) {
		//Door->DoorInteraction(PlayerPos);
		Door->MulticastRpcDoorInteraction(PlayerPos);
	}
}

bool UMainPlayerControllerComponent::PlaceCraftedObjectToWorld() {
	AMainPlayerController* MainController = (AMainPlayerController*)GetOwner();
	if (!MainController) {
		return false;
	}

	if (!MainController->LevelController) {
		return false;
	}

	ABaseCharacter* BaseCharacter = Cast<ABaseCharacter>(MainController->GetCharacter());
	if (!BaseCharacter) {
		return false;
	}

	FHitResult Hit = MainController->TracePlayerActionPoint();
	if (Hit.bBlockingHit) {
		UE_LOG(LogTemp, Warning, TEXT("PlaceCraftedObjectToWorld -> %f %f %f"), Hit.ImpactPoint.X, Hit.ImpactPoint.Y, Hit.ImpactPoint.Z);

		if (CurrentActionType == ECurrentActionType::PlaceCraftToWorld) {
			FCraftRecipe* CraftRecipeData = MainController->GetCraftRecipeData(CraftReceiptId);
			if (CraftRecipeData) {
				TSubclassOf<ASandboxObject> ObjSubclass = MainController->LevelController->GetSandboxObjectByClassId(CraftRecipeData->SandboxClassId);
				if (ObjSubclass) {
					ABaseObject* BaseObj = Cast<ABaseObject>(ObjSubclass->GetDefaultObject());
					if (IsCursorPositionValid(Hit) && BaseObj && MainController->SpentCraftRecipeItems(CraftReceiptId)) {

						FVector Location(0);
						FRotator Rotation(0);

						FVector CamLoc;
						FRotator CamRot;
						MainController->GetPlayerViewPoint(CamLoc, CamRot);

						CamRot.Pitch = 0;
						CamRot.Roll = 0;

						bool isPossible = BaseObj->PlaceToWorldClcPosition(GetWorld(), CamLoc, CamRot, Hit, Location, Rotation, true);
						if (isPossible) {
							FTransform Transform(Rotation, Location, FVector(1));
							auto SandboxClassId = CraftRecipeData->SandboxClassId;

							if (GetNetMode() != NM_Client) {
								MainController->SpawnObjectByPlayer(SandboxClassId, Transform);
							}

							if (GetNetMode() == NM_Client) {
								MainController->ServerRpcSpawnObject(SandboxClassId, Transform, false);
							}

							if (CraftRecipeData->bOnlyOne) {
								ResetState();
							}

							return true;
						}
					}
				}
			}
		}
	}

	return false;
}

void UMainPlayerControllerComponent::ResetState() {
	bAltActionPressed = false;
	ResetSelectedObject();
	CurrentActionType = ECurrentActionType::None;
	CraftReceiptId = 0;

	AMainPlayerController* MainController = (AMainPlayerController*)GetOwner();
	ABaseCharacter* BaseCharacter = Cast<ABaseCharacter>(MainController->GetCharacter());
	if (BaseCharacter) {
		ResetCursorMesh(BaseCharacter);
	}
}

void  UMainPlayerControllerComponent::SetActionType(ECurrentActionType ActionType) {
	CurrentActionType = ActionType;
}

void  UMainPlayerControllerComponent::ToggleActionPlaceObjectToWorld() {
	if (CurrentActionType == ECurrentActionType::PlaceObjectToWorld) {
		ResetState();
	} else {
		ResetState();
		AMainPlayerController* MainController = (AMainPlayerController*)GetOwner();
		ABaseCharacter* BaseCharacter = Cast<ABaseCharacter>(MainController->GetCharacter());
		if (BaseCharacter) {
			ASandboxObject* Obj = MainController->GetCurrentInventoryObject();
			if (Obj && CanPlaceObjectToWorld(Obj)) {
				CurrentActionType = ECurrentActionType::PlaceObjectToWorld;
			}
		}
	}
}

void UMainPlayerControllerComponent::TakeSelectedObjectToInventory() {
	AMainPlayerController* MainController = (AMainPlayerController*)GetOwner();
	if (MainController->HasOpenContainer()) {
		return;
	}

	ALevelController* LevelController = MainController->GetLevelController();
	if (SelectedObject.ObjType == ESelectedObjectType::SandboxObject) {
		UContainerComponent* Inventory = MainController->GetInventory();
		if (Inventory != nullptr) {
			FHitResult ActionPoint = MainController->TracePlayerActionPoint();
			if (ActionPoint.bBlockingHit) {
				ASandboxObject* Obj = Cast<ASandboxObject>(ActionPoint.GetActor());
				if (Obj && Obj->CanTake(nullptr)) {
					uint64 ClassId = Obj->GetSandboxClassId();
					MainController->ServerRpcRemoveActor(Obj);
					MainController->SandboxAddItem(ClassId);
				}
			}
		}
	}

	if (SelectedObject.ObjType == ESelectedObjectType::InstancedMesh) {
		UContainerComponent* Inventory = MainController->GetInventory();
		if (Inventory) {
			FHitResult ActionPoint = MainController->TracePlayerActionPoint();
			if (ActionPoint.bBlockingHit) {
				ATerrainController* Terrain = Cast<ATerrainController>(ActionPoint.GetActor());
				if (Terrain) {
					UTerrainInstancedStaticMesh* TerrainMesh = Cast<UTerrainInstancedStaticMesh>(ActionPoint.GetComponent());
					if (TerrainMesh) {
						const FTerrainInstancedMeshType* TerrainMeshType = Terrain->GetInstancedMeshType(TerrainMesh->MeshTypeId, TerrainMesh->MeshVariantId);
						if (TerrainMeshType && TerrainMeshType->SandboxClassId > 0) {
							MainController->RemoveTerrainMesh(TerrainMesh, ActionPoint.Item);
							if (LevelController) {
								TSubclassOf<ASandboxObject> ObjSubclass = LevelController->GetSandboxObjectByClassId(TerrainMeshType->SandboxClassId);
								ASandboxObject* Obj = (ASandboxObject*)ObjSubclass->GetDefaultObject();
								if (Obj) {
									MainController->SandboxAddItem(Obj->GetSandboxClassId());
								}
							}
						}
					}
				}
			}
		}
	}

	ResetState();
}

void UMainPlayerControllerComponent::MainInteraction() {
	ResetState();

	AMainPlayerController* MainController = (AMainPlayerController*)GetOwner();
	if (MainController->HasOpenContainer()) {
		MainController->CloseObjectWithContainer();
		MainController->DisableGuiMode();
		MainController->CloseMainInventoryGui();
		CurrentActionType = ECurrentActionType::None;
		return;
	}

	if (CurrentActionType == ECurrentActionType::None) {
		FHitResult Hit = MainController->TracePlayerActionPoint();
		if (Hit.bBlockingHit) {
			ASandboxObject* Obj = Cast<ASandboxObject>(Hit.GetActor());
			if (Obj) {
				if (Obj->IsInteractive()) {

					if (Obj->GetSandboxTypeId() == SandboxType_Door) {
						ServerRpcDoorInteraction(Obj, MainController->GetPawn()->GetActorLocation());
						return;
					}

					if (GetNetMode() == NM_Client) {
						ServerRpcObjMainInteraction(Obj);
					} else {
						Obj->MainInteraction(MainController->GetPawn());
					}

				} else {
					ABaseObject* BaseObj = Cast<ABaseObject>(Obj);
					if (BaseObj) {
						if (BaseObj->IsContainer()) {
							if (!MainController->HasOpenContainer()) {
								if (MainController->OpenObjectContainer(BaseObj)) {
									MainController->EnableGuiMode();
									MainController->OpenMainInventoryGui(BaseObj->GetContainerWidgetName());
									return;
								}
							} 
						}
					}
				}
			}
		}
	}
}

void UMainPlayerControllerComponent::OnDeath() {
	ResetState();
}

bool UMainPlayerControllerComponent::ToggleCraftMode(int ReceiptId) {
	AMainPlayerController* MainController = (AMainPlayerController*)GetOwner();

	if (!MainController->ValidateCraftItems(ReceiptId)) {
		return false;
	}

	UContainerComponent* Inventory = MainController->GetInventory();
	if (Inventory == nullptr) {
		return false;
	}

	FCraftRecipe* CraftRecipeData = MainController->GetCraftRecipeData(ReceiptId);
	if (CraftRecipeData) {
		if (CraftRecipeData->bToInventory) {
			if (MainController->SpentCraftRecipeItems(ReceiptId)) {
				for (int Idx = 0; Idx < CraftRecipeData->Qty; Idx++) {
					MainController->SandboxAddItem(CraftRecipeData->SandboxClassId);
				}
				return false;
			}
						
		} else {
			ResetState();
			CurrentActionType = ECurrentActionType::PlaceCraftToWorld;
			CraftReceiptId = ReceiptId;
		}
	}

	return true;
}

void UMainPlayerControllerComponent::OnSelectCurrentInventorySlot(int SlotId) {
	AMainPlayerController* MainController = (AMainPlayerController*)GetOwner();
	ABaseCharacter* BaseCharacter = Cast<ABaseCharacter>(MainController->GetCharacter());
	if (BaseCharacter) {
		if (BaseCharacter->IsDead()) {
			return;
		}

		ASandboxObject* Obj = MainController->GetCurrentInventoryObject();
		if (Obj && CanPlaceObjectToWorld(Obj)) {
			CurrentActionType = ECurrentActionType::PlaceObjectToWorld;
		}

		BaseCharacter->SelectActiveInventorySlot(SlotId);
	}
}

void UMainPlayerControllerComponent::OnInventoryItemMainAction(int32 SlotId) {

}

void UMainPlayerControllerComponent::OnWheelUp() {
	AMainPlayerController* MainController = (AMainPlayerController*)GetOwner();
	ASandboxObject* Obj = MainController->GetCurrentInventoryObject();
	if (Obj) {
		AMiningTool* MiningTool = Cast<AMiningTool>(Obj);
		if (MiningTool) {
			MiningTool->SwitchUp();

			ABaseCharacter* BaseCharacter = Cast<ABaseCharacter>(MainController->GetCharacter());
			if (BaseCharacter) {
				BaseCharacter->CursorToWorld->SetVisibility(false);
			}
		}
	}
}

void UMainPlayerControllerComponent::OnWheelDown() {
	AMainPlayerController* MainController = (AMainPlayerController*)GetOwner();
	ASandboxObject* Obj = MainController->GetCurrentInventoryObject();
	if (Obj) {
		AMiningTool* MiningTool = Cast<AMiningTool>(Obj);
		if (MiningTool) {
			MiningTool->SwitchDown();

			ABaseCharacter* BaseCharacter = Cast<ABaseCharacter>(MainController->GetCharacter());
			if (BaseCharacter) {
				BaseCharacter->CursorToWorld->SetVisibility(false);
			}
		}
	}
}
