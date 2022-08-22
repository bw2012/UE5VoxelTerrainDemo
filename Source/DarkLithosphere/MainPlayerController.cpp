// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "MainPlayerController.h"
#include "SandboxTerrainController.h"
#include "SandboxCharacter.h"
#include "VoxelIndex.h"
//#include "Async.h"
#include "MainHUD.h"
#include "SpawnHelper.h"
#include "Blueprint/WidgetBlueprintLibrary.h"

#include "SandboxObject.h"
#include "BaseCharacter.h"
//#include "CoreCharacter.h"
#include "Objects/MiningTool.h"

#include "DrawDebugHelpers.h"


//#include "SandboxTree.h"

// ALS
#include "Character/ALSPlayerCameraManager.h"
#include "Character/ALSBaseCharacter.h"


AMainPlayerController::AMainPlayerController() {
	MainPlayerControllerComponent = CreateDefaultSubobject<UMainPlayerControllerComponent>(TEXT("MainPlayerController"));
	LastCursorPosition = FVector(0);
	SelectedPawn = nullptr;
	NewCharacterId = 0;
	bSpawnSandboxCharacter = false;
	bFirstStart = false;
	SandboxPlayerUid = TEXT("player 0");
}

void AMainPlayerController::BeginPlay() {
	Super::BeginPlay();

	bFirstStart = true;

	if (GetWorld()->IsClient()) {
		bClientPosses = true;
	}

	for (TActorIterator<ASandboxEnvironment> ActorItr(GetWorld()); ActorItr; ++ActorItr) {
		ASandboxEnvironment* Env = Cast<ASandboxEnvironment>(*ActorItr);
		if (Env) {
			UE_LOG(LogTemp, Log, TEXT("Found ASandboxEnvironment -> %s"), *Env->GetName());
			SandboxEnvironment = Env;
			break;
		}
	}

	for (TActorIterator<ATerrainController> ActorItr(GetWorld()); ActorItr; ++ActorItr) {
		ATerrainController* TerrainCtrl = Cast<ATerrainController>(*ActorItr);
		if (TerrainCtrl) {
			UE_LOG(LogTemp, Log, TEXT("Found ATerrainController -> %s"), *TerrainCtrl->GetName());
			TerrainController = TerrainCtrl;
			break;
		}
	}
}

void SetRenderCustomDepth(AActor* Actor, bool RenderCustomDepth) {
	TArray<UMeshComponent*> MeshComponentList;
	Actor->GetComponents<UMeshComponent>(MeshComponentList);

	for (UMeshComponent* MeshComponent : MeshComponentList) {
		MeshComponent->SetRenderCustomDepth(RenderCustomDepth);
	}
}

APawn* PawnUnderCursor(const FHitResult& Hit) {
	if (Hit.bBlockingHit) {
		return  Cast<APawn>(Hit.GetActor());
	}

	return nullptr;
}

void AMainPlayerController::DefineSandboxPlayerUid_Implementation(const FString& NewPlayerUid){
	SandboxPlayerUid = NewPlayerUid;
}

void AMainPlayerController::FindOrCreateCharacter_Implementation() {
	const int32 PlayerId = PlayerState->GetPlayerId();
	FString Text = TEXT("Spawn player character: ") + FString::FromInt(PlayerId) + TEXT(" ") + SandboxPlayerUid;
	GEngine->AddOnScreenDebugMessage(-1, 10, FColor::White, Text);

	ABaseCharacter* MainCharacter = nullptr;
	for (TActorIterator<ABaseCharacter> ActorItr(GetWorld()); ActorItr; ++ActorItr) {
		ABaseCharacter* BaseCharacter = Cast<ABaseCharacter>(*ActorItr);
		if (BaseCharacter) {
			UE_LOG(LogTemp, Warning, TEXT("BaseCharacter: %s"), *BaseCharacter->GetName());
			if (BaseCharacter->SandboxPlayerId == 0) {
				//MainCharacter = BaseCharacter;
			}
		}
	}

	if (bSpawnSandboxCharacter) {
		if (!MainCharacter) {
			if (LevelController) {
				FVector Location(0, 0, 500);
				FRotator Rotation(0);

				ACharacter* NewCharacter = ((ALevelController*)LevelController)->SpawnCharacterByTypeId(NewCharacterId, Location, Rotation);

				if (NewCharacter) {
					UContainerComponent* StartupInventory = GetFirstComponentByName<UContainerComponent>(TEXT("StartupInventory"));
					UContainerComponent* Inventory = GetFirstComponentByName<UContainerComponent>(NewCharacter, TEXT("Inventory"));
					if (StartupInventory && Inventory) {
						StartupInventory->CopyTo(Inventory);
					}

					MainCharacter = Cast<ABaseCharacter>(NewCharacter);
					MainCharacter->RebuildEquipment();
				}
			}
		}

		if (MainCharacter) {
			UnPossess();
			SandboxPossess(MainCharacter);
		}
	}

}

void AMainPlayerController::PlayerTick(float DeltaTime) {
	Super::PlayerTick(DeltaTime);

	if (bFirstStart) {
		if (GetWorld()->IsClient()) {
			DefineSandboxPlayerUid(TEXT("test-client"));
		}

		if (GetWorld()->IsServer()) {
			SandboxPlayerUid = TEXT("test-server");
		}

		FindOrCreateCharacter();
		bFirstStart = false;
	}

	// OnPossess not works on client. this is workaround
	if (GetWorld()->IsClient()) {
		if (bClientPosses) {
			SetCurrentInventorySlot(-1);



			bClientPosses = false;
		}
	}

	ADummyPawn* DummyPawn = Cast<ADummyPawn>(GetPawn());
	if (DummyPawn) {

		if (this->SandboxMode == 0) {
			FHitResult Hit;
			GetHitResultUnderCursor(ECC_Camera, false, Hit);
			ASandboxObject* Obj = Cast<ASandboxObject>(Hit.GetActor());
			if (Obj) {
				if (SelectedObj != Obj) {
					if (SelectedObj) {
						SelectedObj = nullptr;
					}

					SelectedObj = Obj;
					UE_LOG(LogTemp, Warning, TEXT("Obj: %s"), *Obj->GetName());
				}
			} else {
				if (SelectedObj) {
					SelectedObj = nullptr;
				}
			}

			/*
			APawn* Pawn = PawnUnderCursor(Hit);
			if (Pawn) {
				if (SelectedPawn != Pawn) {
					if (SelectedPawn) {
						SetRenderCustomDepth(SelectedPawn, false);
						SelectedPawn = nullptr;
					}

					SetRenderCustomDepth(Pawn, true);
					SelectedPawn = Pawn;
					UE_LOG(LogTemp, Warning, TEXT("Pawn: %s"), *Pawn->GetName());
				}
			} else {
				if (SelectedPawn) {
					SetRenderCustomDepth(SelectedPawn, false);
					SelectedPawn = nullptr;
				}
			}
			*/
		}
			

		if (this->SandboxMode == 2) {
			
		}

	}


	ABaseCharacter* PlayerCharacter = Cast<ABaseCharacter>(GetCharacter());
	if (PlayerCharacter) {
		MainPlayerControllerComponent->OnPlayerTick();

		FVector Location = PlayerCharacter->GetActorLocation();
		const float Distance = FVector::Distance(Location, PrevLocation);
		if (Distance > 50) {
			PrevLocation = Location;
			// update player position
			if (SandboxEnvironment) {
				SandboxEnvironment->UpdatePlayerPosition(Location);
				if (Location.Z < -500) { // TODO refactor
					//UE_LOG(LogTemp, Log, TEXT("SetCaveMode = true"));
					SandboxEnvironment->SetCaveMode(true);
				} else {
					//UE_LOG(LogTemp, Log, TEXT("SetCaveMode = false"));
					SandboxEnvironment->SetCaveMode(false);
				}
			}
		}
	}
}

void AMainPlayerController::ToggleToolMode() { 
	ABaseCharacter* BaseCharacter = Cast<ABaseCharacter>(GetCharacter());
	if (!BaseCharacter) {
		return;
	}

	if (BaseCharacter->IsDead()) {
		return;
	}

	ASandboxObject* Obj = GetCurrentInventoryObject();
	if (Obj) {
		AMiningTool* MiningTool = Cast<AMiningTool>(Obj);
		if (MiningTool) {
			MiningTool->ToggleToolMode();
		}
	}
};

void AMainPlayerController::SetupInputComponent() {
	// set up gameplay key bindings
	Super::SetupInputComponent();
	InputComponent->BindAction("MainInteraction", IE_Pressed, this, &AMainPlayerController::OnMainInteractionPressed);
}

void AMainPlayerController::OnMainActionPressed() {
	ADummyPawn* DummyPawn = Cast<ADummyPawn>(GetPawn());
	if (DummyPawn) {
		FHitResult Hit;
		GetHitResultUnderCursor(ECC_Camera, false, Hit);

		if (this->SandboxMode == 0) {
			ASandboxObject* Obj = Cast<ASandboxObject>(Hit.GetActor());
			if (PickedObj && PickedObj == Obj) {
				SetRenderCustomDepth(PickedObj, false);
				PickedObj = nullptr;
				goto l1;
			} 

			if (SelectedObj && SelectedObj == Obj) {
				if (PickedObj) {
					SetRenderCustomDepth(PickedObj, false);
					PickedObj = nullptr;
				}

				PickedObj = SelectedObj;
				SetRenderCustomDepth(PickedObj, true);
				goto l1;
			}

			if (!Obj) {
				if (PickedObj) {
					SetRenderCustomDepth(PickedObj, false);
					PickedObj = nullptr;
				}
				goto l1;
			}
		}

	l1:

		APawn* DetectedPawn = PawnUnderCursor(Hit);
		if (DetectedPawn) {
			if (SelectedPawn != DetectedPawn) {
				if (SelectedPawn) {
					SetRenderCustomDepth(SelectedPawn, false);
					SelectedPawn = nullptr;
				}

				SetRenderCustomDepth(DetectedPawn, true);
				SelectedPawn = DetectedPawn;
				UE_LOG(LogTemp, Warning, TEXT("Pawn: %s"), *DetectedPawn->GetName());
			} else {
				ResetAllSelections();
				UnPossessDummyPawn(DetectedPawn);
			}
		} else {
			if (SelectedPawn) {
				SetRenderCustomDepth(SelectedPawn, false);
				SelectedPawn = nullptr;
			}
		}

		DummyPawn->OnMainAction(Hit);

		if (this->SandboxMode == 1) {
			 
		}

		if (this->SandboxMode == 2) {

		}

		if (this->SandboxMode == 3) {
			
		}
			
		return;
	}

	ABaseCharacter* BaseCharacter = Cast<ABaseCharacter>(GetCharacter());
	if (BaseCharacter) {
		MainPlayerControllerComponent->PerformMainAction();
	}
}

void AMainPlayerController::OnMainActionReleased() {
	ADummyPawn* DummyPawn = Cast<ADummyPawn>(GetCharacter());
	if (DummyPawn) {
		return;
	}

	SetDestinationReleased();
}

ASandboxObject* AMainPlayerController::GetInventoryObject(int32 SlotId) {
	UContainerComponent* Inventory = GetInventory();
	if (Inventory != nullptr) {
		FContainerStack* Stack = Inventory->GetSlot(SlotId);
		if (Stack != nullptr) {
			if (Stack->Amount > 0) {
				TSubclassOf<ASandboxObject>	ObjectClass = Stack->ObjectClass;
				if (ObjectClass != nullptr) {
					ASandboxObject* Actor = Cast<ASandboxObject>(ObjectClass->ClassDefaultObject);
					return Actor;
				}
			}
		}
	}

	return nullptr;
}

ASandboxObject* AMainPlayerController::GetCurrentInventoryObject() {
	return GetInventoryObject(CurrentInventorySlot);
}


void AMainPlayerController::OnMainInteractionPressed() {
	ADummyPawn* DummyPawn = Cast<ADummyPawn>(GetCharacter());
	if(DummyPawn) {
		return;
	}

	ABaseCharacter* BaseCharacter = Cast<ABaseCharacter>(GetCharacter());
	if (BaseCharacter) {
		if (BaseCharacter->IsDead()) {
			return;
		}

		MainPlayerControllerComponent->MainInteraction();
	}
}

void AMainPlayerController::OnAltActionPressed() {
	ADummyPawn* DummyPawn = Cast<ADummyPawn>(GetPawn());
	if (DummyPawn) {
		return;
	}

	ABaseCharacter* BaseCharacter = Cast<ABaseCharacter>(GetCharacter());
	if (BaseCharacter) {
		if (BaseCharacter->IsDead()) {
			return;
		}

		MainPlayerControllerComponent->PerformAltAction();
	}

}

void AMainPlayerController::OnAltActionReleased() {
	//GetWorld()->GetTimerManager().ClearTimer(Timer);
}

/*
void AMainPlayerController::OnTracePlayerActionPoint(const FHitResult& Res) {
	ADummyPawn* DummyPawn = Cast<ADummyPawn>(GetPawn());
	if (DummyPawn) {
		// do nothing;
		return;
	}

	ABaseCharacter* BaseCharacter = Cast<ABaseCharacter>(GetCharacter());
	if (BaseCharacter) {
		if (BaseCharacter->IsDead()) {
			return;
		}

		MainPlayerControllerComponent->OnTracePlayerActionPoint(Res);
	}
}
*/

void AMainPlayerController::OnSelectActionObject(AActor* Actor) {
	ASandboxCharacter* PlayerCharacter = Cast<ASandboxCharacter>(GetCharacter());
	if (PlayerCharacter->GetSandboxPlayerView() == PlayerView::TOP_DOWN) {
		ASandboxPlayerController::OnSelectActionObject(Actor);
	}
}

void AMainPlayerController::OnDeselectActionObject(AActor* Actor) {
	ASandboxPlayerController::OnDeselectActionObject(Actor);
}

void AMainPlayerController::PossessDummyPawn() {
	ResetAllSelections();

	UE_LOG(LogTemp, Warning, TEXT("AMainPlayerController::PossessDummyPawn()"));
	ADummyPawn* DummyPawn = Cast<ADummyPawn>(GetPawn());
	if (!DummyPawn) {
		if (DummyPawnClass) {
			ACharacter* LastCharacter = GetCharacter();
			FVector Location = LastCharacter->GetActorLocation();
			UnPossess();
			TSubclassOf<AController> AIControllerClass = LastCharacter->AIControllerClass;
			if (AIControllerClass) {
				FVector NewLocation(0);
				FRotator NewRotation(0);
				AController* NewAIController = Cast<AController>(GetWorld()->SpawnActor(AIControllerClass, &NewLocation, &NewRotation));
				if (NewAIController) {
					NewAIController->Possess(LastCharacter);
				}
			}

			FRotator Rotation(0);
			ADummyPawn* NewDummy = Cast<ADummyPawn>(GetWorld()->SpawnActor(DummyPawnClass, &Location, &Rotation));
			if (NewDummy) {
				Possess(NewDummy);
			}
		}
	}
}

void AMainPlayerController::UnPossessDummyPawn(APawn* NewPawn) {
	UE_LOG(LogTemp, Warning, TEXT("AMainPlayerController::UnPossessDummyPawn()"));
	ADummyPawn* DummyPawn = Cast<ADummyPawn>(GetPawn());
	if (DummyPawn) {
		UnPossess();
		Possess(NewPawn);

		ASandboxCharacter* NewCharacter = Cast<ASandboxCharacter>(NewPawn);
		if (NewCharacter) {
			//NewCharacter->InitTopDownView();
			NewCharacter->InitThirdPersonView();
		}
	}
}

void AMainPlayerController::ResetAllSelections() {
	if (SelectedPawn) {
		SetRenderCustomDepth(SelectedPawn, false);
		SelectedPawn = nullptr;
	}

	if (PickedObj) {
		SetRenderCustomDepth(PickedObj, false);
		PickedObj = nullptr;
	}
}


void AMainPlayerController::SetSandboxMode(int Mode) {
	this->SandboxMode = Mode;

	LastCursorPosition = FVector(0);

	ADummyPawn* DummyPawn = Cast<ADummyPawn>(GetPawn());
	if (DummyPawn) {
		DummyPawn->SetSandboxMode(Mode);
		ResetAllSelections();
	}
}

void AMainPlayerController::SetSandboxModeExtId(int Id) {
	this->SandboxModeExtId = Id;

	ADummyPawn* DummyPawn = Cast<ADummyPawn>(GetPawn());
	if (DummyPawn) {

	}
}

void AMainPlayerController::TestRemoveSandboxObject() {
	ADummyPawn* DummyPawn = Cast<ADummyPawn>(GetPawn());
	if (DummyPawn) {
		if (this->PickedObj) {
			this->PickedObj->Destroy();
		}
	}

	ABaseCharacter* BaseCharacter = Cast<ABaseCharacter>(GetCharacter());
	if (BaseCharacter) {
		if (BaseCharacter->IsDead()) {
			return;
		}

		MainPlayerControllerComponent->TakeSelectedObjectToInventory();
	}
}

void AMainPlayerController::SetCurrentInventorySlot(int32 Slot) {
	ABaseCharacter* BaseCharacter = Cast<ABaseCharacter>(GetCharacter());
	if (BaseCharacter) {
		if (BaseCharacter->IsDead()) {
			return;
		}
	}

	if (CurrentInventorySlot == Slot) {
		Slot = -1;
	}

	Super::SetCurrentInventorySlot(Slot);
	MainPlayerControllerComponent->ResetState();
	MainPlayerControllerComponent->OnSelectCurrentInventorySlot(Slot);
}

void AMainPlayerController::ChangeDummyCameraAltitude(float Val) {
	ADummyPawn* DummyPawn = Cast<ADummyPawn>(GetPawn());
	if (DummyPawn) {
		FVector Pos = DummyPawn->GetActorLocation();
		Pos.Z += Val;
		DummyPawn->SetActorLocation(Pos);


		UE_LOG(LogTemp, Warning, TEXT("test1 -> %f"), Pos.Z);

		TArray<AActor*> ObjList;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), ASandboxObject::StaticClass(), ObjList);

		for (AActor* O : ObjList) {
			ASandboxObject* Obj = (ASandboxObject*)O;
			float ObjZ = Obj->GetActorLocation().Z;

			//if (Obj->GetSandboxTypeId() == SandboxCeiling) {
			//	ObjZ += 300;
			//}

			//UE_LOG(LogTemp, Warning, TEXT("test2 -> %f - %s"), ObjPos.Z, *Obj->GetName());

			if (ObjZ > Pos.Z) {
				//UE_LOG(LogTemp, Warning, TEXT("test -> %s"), *Obj->GetName());
				//UE_LOG(LogTemp, Warning, TEXT("test2 -> %f - %s"), ObjZ, *Obj->GetName());

				TArray<UStaticMeshComponent*> Components;
				Obj->GetComponents<UStaticMeshComponent>(Components);
				for (UStaticMeshComponent* StaticMeshComponent : Components) {
					StaticMeshComponent->SetVisibility(false, false); // hide
					StaticMeshComponent->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
				}
			} else {
				TArray<UStaticMeshComponent*> Components;
				Obj->GetComponents<UStaticMeshComponent>(Components);
				for (UStaticMeshComponent* StaticMeshComponent : Components) {
					StaticMeshComponent->SetVisibility(true, false); // show
					StaticMeshComponent->SetCollisionResponseToChannel(ECC_Camera, ECR_Block);

				}
			}
		}
	}
}

void AMainPlayerController::OnDeath() {
	ABaseCharacter* BaseCharacter = Cast<ABaseCharacter>(GetCharacter());
	if (BaseCharacter) {
		MainPlayerControllerComponent->OnDeath();
	}
}

bool AMainPlayerController::IsGuiMode() {
	return bGuiMode;
}

void AMainPlayerController::EnableGuiMode() {
	if (!bGuiMode) {
		BlockGameInput();
		bShowMouseCursor = true;
		UWidgetBlueprintLibrary::SetInputMode_GameAndUIEx(this);
		bGuiMode = true;
	}
}

void AMainPlayerController::DisableGuiMode() {
	if (bGuiMode) {
		UnblockGameInput();
		bShowMouseCursor = false;
		UWidgetBlueprintLibrary::SetInputMode_GameOnly(this);
		bGuiMode = false;
	}
}

void AMainPlayerController::ToggleMainInventory() {
	if (!bGuiMode) {
		EnableGuiMode();
		OpenMainInventoryGui();
	} else {
		DisableGuiMode();
		CloseMainInventoryGui();
	}
}

void AMainPlayerController::OpenMainInventoryGui(FName ExtWidget) {
	ABaseCharacter* BaseCharacter = Cast<ABaseCharacter>(GetCharacter());
	if (BaseCharacter) {
		if (BaseCharacter->IsDead()) {
			return;
		}

		FString Name = (ExtWidget == NAME_None) ? TEXT("craft") : ExtWidget.ToString();
		AMainHUD* MainHud = Cast<AMainHUD>(GetHUD());
		if (MainHud) {
			MainHud->OpenWidget(TEXT("inventory"));
			MainHud->OpenWidget(Name);
		}
	}
}

void AMainPlayerController::CloseMainInventoryGui() {
	ABaseCharacter* BaseCharacter = Cast<ABaseCharacter>(GetCharacter());
	if (BaseCharacter) {
		AMainHUD* MainHud = Cast<AMainHUD>(GetHUD());
		if (MainHud) {
			MainHud->CloseAllWidgets();
		}
	}
}

void AMainPlayerController::SetExtMode(int Index) {
	UE_LOG(LogTemp, Warning, TEXT("SetExtMode -> %d"), Index);

	ABaseCharacter* BaseCharacter = Cast<ABaseCharacter>(GetCharacter());
	if (BaseCharacter) {
		SetCurrentInventorySlot(-1);
		DisableGuiMode();
		CloseMainInventoryGui();
		MainPlayerControllerComponent->ToggleCraftMode(Index);
	}
}

FCraftRecipeData* AMainPlayerController::GetCraftRecipeData(int32 RecipeId) {
	FCraftRecipeData* D = CraftRecipeMap.Find(RecipeId);
	return D;
}

// server only
void AMainPlayerController::SandboxPossess(ACharacter* PlayerCharacter) {
	ABaseCharacter* BaseCharacter = Cast<ABaseCharacter>(PlayerCharacter);
	if (BaseCharacter) {
		Possess(PlayerCharacter);
		SetCurrentInventorySlot(-1);
		ShowInGameHud();
	}
}

void AMainPlayerController::OnPossess(APawn* NewPawn) {
	Super::OnPossess(NewPawn);

	if (!IsRunningDedicatedServer()) {
		SetupCamera();
	}

	SetCurrentInventorySlot(-1);

	/*
	ABaseCharacter* BaseCharacter = Cast<ABaseCharacter>(NewPawn);
	if (BaseCharacter) {
		AMainHUD* MainHud = Cast<AMainHUD>(GetHUD());
		if (MainHud) {
			MainHud->ShowInGameInventory();
		}
	}
	*/
}

void AMainPlayerController::SetupCamera() {
	AALSBaseCharacter* ALSBaseCharacter = Cast<AALSBaseCharacter>(GetCharacter());
	AALSPlayerCameraManager* CameraManager = Cast<AALSPlayerCameraManager>(PlayerCameraManager);
	if (CameraManager && ALSBaseCharacter) {
		CameraManager->OnPossess(ALSBaseCharacter);
	}
}

FHitResult AMainPlayerController::TracePlayerActionPoint() {
	const float MaxUseDistance = 1000;
	float Distance = 0;

	FVector CamLoc;
	FRotator CamRot;
	GetPlayerViewPoint(CamLoc, CamRot);

	const FVector StartTrace = CamLoc;
	const FVector Direction = CamRot.Vector();

	ACharacter* PlayerCharacter = GetCharacter();
	if (PlayerCharacter) {
		FVector Pos = PlayerCharacter->GetActorLocation();
		Distance = FVector::Dist(Pos, CamLoc);
		Distance += 300;
	} else {
		Distance = MaxUseDistance;
	}

	const FVector EndTrace = StartTrace + (Direction * Distance);

	FCollisionQueryParams TraceParams(FName(TEXT("")), true, this);
	//TraceParams.bTraceAsyncScene = true;
	//TraceParams.bReturnPhysicalMaterial = false;

	TraceParams.bTraceComplex = true;
	TraceParams.bReturnFaceIndex = true;
	TraceParams.AddIgnoredActor(GetCharacter());

	FHitResult Hit(ForceInit);
	GetWorld()->LineTraceSingleByChannel(Hit, StartTrace, EndTrace, ECC_Visibility, TraceParams);

	return Hit;
}

ALevelController* AMainPlayerController::GetLevelController() {
	return (ALevelController*)LevelController;
}

void AMainPlayerController::OnContainerMainAction(int32 SlotId, FName ContainerName) {
	ABaseCharacter* BaseCharacter = Cast<ABaseCharacter>(GetCharacter());
	if (BaseCharacter) {
		ASandboxObject* Obj = GetInventoryObject(SlotId);
		if (Obj) {

			/*
			const int TypeId = Obj->GetSandboxTypeId();
			if (TypeId == 500) {
				ASandboxSkeletalModule* Clothing = Cast<ASandboxSkeletalModule>(Obj);
				if (Clothing) {
					if (Clothing->SkeletalMesh) {
						USkeletalMeshComponent* SkeletalMeshComponent = GetFirstComponentByName<USkeletalMeshComponent>(BaseCharacter, TEXT("BootsMesh"));
						if (SkeletalMeshComponent) {
							UE_LOG(LogTemp, Warning, TEXT("Equip"));

							USkeletalMeshComponent* CharacterMeshComponent = GetFirstComponentByName<USkeletalMeshComponent>(BaseCharacter, TEXT("CharacterMesh0"));
							SkeletalMeshComponent->SetSkeletalMesh(Clothing->SkeletalMesh);
							SkeletalMeshComponent->SetMasterPoseComponent(CharacterMeshComponent, true);

							if (Clothing->bModifyFootPose) {
								Clothing->GetFootPose(BaseCharacter->LeftFootRotator, BaseCharacter->RightFootRotator);
							}

							for (auto& Entry : Clothing->MorphMap) {
								FString Name = Entry.Key;
								float Value = Entry.Value;

								UE_LOG(LogTemp, Warning, TEXT("%s %f"), *Name, Value);
								SkeletalMeshComponent->SetMorphTarget(*Name, Value);
							}
						}
					}
				}
			}
			*/
		}
	}
}

void AMainPlayerController::OnContainerDropSuccess(int32 SlotId, FName SourceName, FName TargetName) {
	if (SourceName == TEXT("Equipment") || TargetName == TEXT("Equipment")) {
		ABaseCharacter* BaseCharacter = Cast<ABaseCharacter>(GetCharacter());
		if (BaseCharacter) {
			BaseCharacter->RebuildEquipment();
		}
	}
}

bool AMainPlayerController::OnContainerDropCheck(int32 SlotId, FName ContainerName, ASandboxObject* Obj) {
	if (ContainerName == TEXT("Equipment")) {
		if (Obj->GetSandboxTypeId() == 500) {
			return true;
		} 

		// TODO

		return false;
	}

	return true;
}


void AMainPlayerController::ShowInGameHud() {
	if (!IsRunningDedicatedServer()) {
		AMainHUD* MainHud = Cast<AMainHUD>(GetHUD());
		if (MainHud) {
			MainHud->ShowInGameInventory();
		}
	}
}