
#include "MainPlayerControllerComponent.h"
#include "MainPlayerController.h"
#include "BaseCharacter.h"
#include "Objects/MiningTool.h"
#include "Objects/BaseObject.h"
#include "SpawnHelper.h"
#include "TerrainZoneComponent.h"



//bool IsCursorPositionValid(const FHitResult& Hit);
void CalculateCursorPosition(ABaseCharacter* Character, const FHitResult& Res, FVector& Location, FRotator& Rotation, ASandboxObject* Obj);
//AActor* SpawnSandboxObject(UWorld* World, const FVector& Location, const FRotator& Rotation, ASandboxObject* Object, bool bEnablePhys = false);


UMainPlayerControllerComponent::UMainPlayerControllerComponent() {
	PrimaryComponentTick.bCanEverTick = true;
}

void UMainPlayerControllerComponent::BeginPlay(){
	Super::BeginPlay();
}

void UMainPlayerControllerComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) {
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

//void SetRenderCustomDepth(AActor* Actor, bool RenderCustomDepth);

bool UMainPlayerControllerComponent::CanPlaceObjectToWorld(const ASandboxObject* Obj) const {
	const AMiningTool* MiningTool = Cast<AMiningTool>(Obj);
	if (MiningTool) {
		return false;
	}

	return true;
}

void ResetCursorMesh(ABaseCharacter* BaseCharacter) {
	BaseCharacter->CursorMesh->SetVisibility(false, true);
	BaseCharacter->CursorMesh->SetStaticMesh(nullptr);
	BaseCharacter->CursorMesh->SetRelativeScale3D(FVector(1));
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

	ASandboxObject* Obj = MainController->GetCurrentInventoryObject();
	if (Obj) {
		AMiningTool* MiningTool = Cast<AMiningTool>(Obj);
		if (MiningTool) {
			//if (bToolActive) {
				FHitResult Hit = MainController->TracePlayerActionPoint();
				if (Hit.bBlockingHit) {
					if (MiningTool->OnTracePlayerActionPoint(Hit, BaseCharacter)) {
						if (SelectedObject.ObjType == ESelectedObjectType::SandboxObject) {
							//SetRenderCustomDepth(SelectedObj, false);
							ResetSelectedObject();
						}
						return;
					}
				}
			//}
		}
	}

	FHitResult Res = MainController->TracePlayerActionPoint();
	bool bPlaceToWorld = CurrentActionType == ECurrentActionType::PlaceObjectToWorld || CurrentActionType == ECurrentActionType::PlaceCraftToWorld;
	bool bIsValid = false;

	if (bPlaceToWorld && IsCursorPositionValid(Res)) {
		ASandboxObject* BaseObj = nullptr;

		if (CurrentActionType == ECurrentActionType::PlaceObjectToWorld) {
			BaseObj = Obj;
		}

		if (CurrentActionType == ECurrentActionType::PlaceCraftToWorld) {
			FCraftRecipeData* CraftRecipeData = MainController->GetCraftRecipeData(CraftReceiptId);
			if (CraftRecipeData) {
				TSubclassOf<ASandboxObject> ObjSubclass = MainController->LevelController->GetSandboxObjectByClassId(CraftRecipeData->SandboxClassId);
				if (ObjSubclass) {
					BaseObj = Cast<ASandboxObject>(ObjSubclass->GetDefaultObject());
				}
			}
		}

		if (BaseObj) {
			auto Mesh = BaseObj->SandboxRootMesh->GetStaticMesh();
			FVector Location(0);
			FRotator Rotation(0);

			FVector CamLoc;
			FRotator CamRot;
			MainController->GetPlayerViewPoint(CamLoc, CamRot);

			bIsValid = BaseObj->PlaceToWorldClcPosition(CamLoc, CamRot, Res, Location, Rotation);
			if (bIsValid) {
				FVector Scale = BaseObj->GetRootComponent()->GetRelativeScale3D();
				BaseCharacter->CursorMesh->SetStaticMesh(Mesh);
				BaseCharacter->CursorMesh->SetVisibility(true, true);
				BaseCharacter->CursorMesh->SetRelativeScale3D(Scale);
				BaseCharacter->CursorMesh->SetWorldLocationAndRotationNoPhysics(Location, Rotation);

				if (MainController->CursorMaterial) {
					for (int I = 0; I < Mesh->GetStaticMaterials().Num(); ++I) {
						UMaterialInstance* Material = (UMaterialInstance*)Mesh->GetMaterial(I);
						BaseCharacter->CursorMesh->SetMaterial(I, MainController->CursorMaterial);
					}
				}
			}
		}
	}

	if (bIsValid) {
		// TODO something
	} else {
		ResetCursorMesh(BaseCharacter);
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
			//if (!bToolActive) {
			//	bToolActive = true;
			//	return;
			//}

			FHitResult Hit = MainController->TracePlayerActionPoint();
			if (Hit.bBlockingHit) {
				ABaseCharacter* BaseCharacter = Cast<ABaseCharacter>(MainController->GetCharacter());
				MiningTool->OnAltAction(Hit, BaseCharacter);
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
				if (TerrainMeshType && TerrainMeshType->SandboxClassId > 0) {
					FSelectedObject NewSelectedObject;
					NewSelectedObject.TerrainMesh = TerrainMesh;
					NewSelectedObject.ObjType = ESelectedObjectType::InstancedMesh;
					TSubclassOf<ASandboxObject> ObjSubclass = LevelController->GetSandboxObjectByClassId(TerrainMeshType->SandboxClassId);
					ASandboxObject* Obj = (ASandboxObject*)ObjSubclass->GetDefaultObject();

					if (LevelController->ObjectMap) {
						FSandboxStaticData* StaticData = LevelController->ObjectMap->StaticData.Find(Obj->GetSandboxClassId());
						if (StaticData) {
							NewSelectedObject.Name = StaticData->ObjectName;
						}
					}

					SelectedObject = NewSelectedObject;
					NotSelected = false;
				}
			}
		}

		ASandboxObject* Obj = Cast<ASandboxObject>(Hit.GetActor());
		if (Obj) {
			//if (Obj->CanTake(MainController->GetCharacter())) {
				//SetRenderCustomDepth(Obj, true);

				FSelectedObject NewSelectedObject;
				NewSelectedObject.SandboxObj = Obj;
				NewSelectedObject.ObjType = ESelectedObjectType::SandboxObject;

				if (LevelController->ObjectMap) {
					FSandboxStaticData* StaticData = LevelController->ObjectMap->StaticData.Find(Obj->GetSandboxClassId());
					if (StaticData) {
						NewSelectedObject.Name = StaticData->ObjectName;
					}
				}

				SelectedObject = NewSelectedObject;
				NotSelected = false;
				//UE_LOG(LogTemp, Warning, TEXT("Obj: %s"), *Obj->GetName());
				//UE_LOG(LogTemp, Warning, TEXT("Obj - %f %f %f"), Obj->GetActorRotation().Pitch, Obj->GetActorRotation().Yaw, Obj->GetActorRotation().Roll);
			//}
		} 
	}


	if (NotSelected) {
		//if (SelectedObj) {
		//	SetRenderCustomDepth(SelectedObj, false);
		//}

		ResetSelectedObject();
	}
}

void UMainPlayerControllerComponent::ResetSelectedObject() {
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

					bool isPossible = Obj->PlaceToWorldClcPosition(CamLoc, CamRot, Hit, Location, Rotation, true);
					if (isPossible) {
						FTransform Transform(Rotation, Location, FVector(1));
						ASandboxObject* NewObj = MainController->LevelController->SpawnSandboxObject(Obj->GetSandboxClassId(), Transform);
						if (NewObj) {
							NewObj->OnPlaceToWorld();

							if (MainController->TerrainController) {
								MainController->TerrainController->RegisterSandboxObject(NewObj);
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
	}

	return false;
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
			FCraftRecipeData* CraftRecipeData = MainController->GetCraftRecipeData(CraftReceiptId);
			if (CraftRecipeData) {
				TSubclassOf<ASandboxObject> ObjSubclass = MainController->LevelController->GetSandboxObjectByClassId(CraftRecipeData->SandboxClassId);
				if (ObjSubclass) {
					ABaseObject* BaseObj = Cast<ABaseObject>(ObjSubclass->GetDefaultObject());
					if (IsCursorPositionValid(Hit) && BaseObj) {
						FVector Location(0);
						FRotator Rotation(0);

						FVector CamLoc;
						FRotator CamRot;
						MainController->GetPlayerViewPoint(CamLoc, CamRot);

						bool isPossible = BaseObj->PlaceToWorldClcPosition(CamLoc, CamRot, Hit, Location, Rotation, true);
						if (isPossible) {
							
							FTransform Transform(Rotation, Location, FVector(1));
							
							ASandboxObject* Obj = MainController->LevelController->SpawnSandboxObject(CraftRecipeData->SandboxClassId, Transform);
							if (Obj) {
								if (MainController->TerrainController) {
									MainController->TerrainController->RegisterSandboxObject(Obj);
								}
							}

							ResetState();
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
	//if (SelectedObj) {
		//SetRenderCustomDepth(SelectedObj, false);
	//}

	ResetSelectedObject();
	CurrentActionType = ECurrentActionType::None;
	CraftReceiptId = 0;

	AMainPlayerController* MainController = (AMainPlayerController*)GetOwner();
	ABaseCharacter* BaseCharacter = Cast<ABaseCharacter>(MainController->GetCharacter());
	if (BaseCharacter) {
		ResetCursorMesh(BaseCharacter);
	}

	//bToolActive = false;
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
				if (Obj) {
					if (Obj->CanTake(nullptr)) {
						if (Inventory->AddObject(Obj)) {
							if (LevelController) {
								LevelController->RemoveSandboxObject(Obj);
							} else {
								Obj->Destroy();
							}
						}
					}
				}
			}
		}

		MainController->TakeObjectToInventory();
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
							TerrainMesh->RemoveInstance(ActionPoint.Item);
							if (LevelController) {
								TSubclassOf<ASandboxObject> ObjSubclass = LevelController->GetSandboxObjectByClassId(TerrainMeshType->SandboxClassId);
								ASandboxObject* Obj = (ASandboxObject*)ObjSubclass->GetDefaultObject();
								if (Obj) {
									Inventory->AddObject(Obj);
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
					Obj->MainInteraction();
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

void UMainPlayerControllerComponent::ToggleCraftMode(int ReceiptId) {
	ResetState();
	CurrentActionType = ECurrentActionType::PlaceCraftToWorld;
	CraftReceiptId = ReceiptId;
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