// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "MainPlayerController.h"
#include "TerrainController.h"
#include "TerrainZoneComponent.h"
#include "SandboxCharacter.h"
#include "VoxelIndex.h"
#include "MainHUD.h"
#include "SpawnHelper.h"
#include "Blueprint/WidgetBlueprintLibrary.h"

#include "Globals.h"
#include "MainGameInstance.h"

#include "SandboxObject.h"
#include "BaseCharacter.h"
#include "Objects/MiningTool.h"

#include "NotificationHelper.h"

// draw debug
#include "DrawDebugHelpers.h"

// ALS
#include "Character/ALSPlayerCameraManager.h"
#include "Character/ALSBaseCharacter.h"

#include "Math/UnrealMathUtility.h"

// json
#include "Json.h"
#include "JsonObjectConverter.h"


AMainPlayerController::AMainPlayerController() {
	MainPlayerControllerComponent = CreateDefaultSubobject<UMainPlayerControllerComponent>(TEXT("MainPlayerController"));
	LastCursorPosition = FVector(0);
	SelectedPawn = nullptr;
	NewCharacterId = 0;
	bSpawnSandboxCharacter = false;
	bFirstStart = false;
}

void AMainPlayerController::BeginPlay() {
	Super::BeginPlay();

	bFirstStart = true;
	TNotificationHelper::AddObserver(TCHAR_TO_UTF8(*GetName()), "finish_init_map_load", std::bind(&AMainPlayerController::OnFinishInitialLoad, this));
	TNotificationHelper::AddObserver(TCHAR_TO_UTF8(*GetName()), "start_background_save", std::bind(&AMainPlayerController::OnStartBackgroundSave, this));
	TNotificationHelper::AddObserver(TCHAR_TO_UTF8(*GetName()), "finish_background_save", std::bind(&AMainPlayerController::OnFinishBackgroundSave, this));

	//warning C4996 : 'UWorld::IsClient' : Use GetNetMode or IsNetMode instead for more accurate results.Please update your code to the new API before upgrading to the next release, otherwise your project will no longer compile.
	if (GetNetMode() == NM_Client) {
		bClientPosses = true;
	}

	for (TActorIterator<AEnvironmentController> ActorItr(GetWorld()); ActorItr; ++ActorItr) {
		AEnvironmentController* Env = Cast<AEnvironmentController>(*ActorItr);
		if (Env) {
			UE_LOG(LogTemp, Log, TEXT("Found ASandboxEnvironment -> %s"), *Env->GetName());
			Environment = Env;
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

	//TODO refactor
	if (IsLocalController()) {
		UE_LOG(LogTemp, Log, TEXT("Load player json"));
		FString FileName = TEXT("player.json");
		FString FullPath = FPaths::ProjectSavedDir() + TEXT("/") + FileName;

		FString JsonRaw;
		if (!FFileHelper::LoadFileToString(JsonRaw, *FullPath, FFileHelper::EHashOptions::None)) {
			UE_LOG(LogTemp, Warning, TEXT("Error loading json file"));

			// new player info
			PlayerInfo.PlayerUid = TEXT("player") + FString::FromInt(FMath::FRandRange(0.f, 255.f));

			FString JsonStr;
			FJsonObjectConverter::UStructToJsonObjectString(PlayerInfo, JsonStr);
			FFileHelper::SaveStringToFile(*JsonStr, *FullPath);
		} else {
			if (!FJsonObjectConverter::JsonObjectStringToUStruct(JsonRaw, &PlayerInfo, 0, 0)) {
				UE_LOG(LogTemp, Error, TEXT("Error parsing json file"));
			}

			if (GetWorld()->WorldType == EWorldType::PIE || GetWorld()->WorldType == EWorldType::Editor) {
				if (GetNetMode() == NM_Client) {
					PlayerInfo.PlayerUid = TEXT("player-client0");
				}
			}

			UE_LOG(LogTemp, Warning, TEXT("Local PlayerUid: %s"), *PlayerInfo.PlayerUid);
		}
	}
}

void AMainPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason) {
	TNotificationHelper::RemoveObserver(TCHAR_TO_UTF8(*GetName()));
}

APawn* PawnUnderCursor(const FHitResult& Hit) {
	if (Hit.bBlockingHit) {
		return  Cast<APawn>(Hit.GetActor());
	}

	return nullptr;
}

void AMainPlayerController::ServerRpcRebuildEquipment_Implementation() {
	ABaseCharacter* BaseCharacter = Cast<ABaseCharacter>(GetCharacter());
	if (BaseCharacter) {
		BaseCharacter->RebuildEquipment();
	}
}

void AMainPlayerController::ClientRpcRegisterFinished_Implementation() {
	UE_LOG(LogTemp, Warning, TEXT("AMainPlayerController::ClientRpcRegisterFinished"));
	TNotificationHelper::SendNotification("finish_register_player");
}

void AMainPlayerController::ServerRpcRegisterSandboxPlayer_Implementation(const FString& NewPlayerUid, const FString& ClientSoftwareVersion) {
	if (ClientSoftwareVersion != GetVersionString()) {
		// TODO возможно это больше не нужно

		UE_LOG(LogTemp, Warning, TEXT("player %s has wrong software version %s"), *NewPlayerUid, *ClientSoftwareVersion);

		AGameSession* Session = GetWorld()->GetAuthGameMode()->GameSession;
		Session->KickPlayer(this, FText::FromString(TEXT("Outdated client")));
		return;
	}

	PlayerInfo.PlayerUid = NewPlayerUid;
	ClientRpcRegisterFinished();
}

void AMainPlayerController::ClientWasKicked_Implementation(const FText& KickReason) {
	FString Msg = KickReason.ToString();
	UE_LOG(LogTemp, Warning, TEXT("KickReason: %s"), *Msg);

	UMainGameInstance* GI = Cast<UMainGameInstance>(UGameplayStatics::GetGameInstance(GetWorld()));
	if (GI) {
		GI->SetMessageString(TEXT("Kicked by server:"), Msg);
	}
}

void AMainPlayerController::FindOrCreateCharacterInternal() {
	FString Text = TEXT("Spawn player: ") + PlayerInfo.PlayerUid;
	//GEngine->AddOnScreenDebugMessage(-1, 10, FColor::White, Text);

	ABaseCharacter* MainCharacter = nullptr;
	for (TActorIterator<ABaseCharacter> ActorItr(GetWorld()); ActorItr; ++ActorItr) {
		ABaseCharacter* BaseCharacter = Cast<ABaseCharacter>(*ActorItr);
		if (BaseCharacter) {
			UE_LOG(LogTemp, Warning, TEXT("BaseCharacter: %s => %s"), *BaseCharacter->SandboxPlayerUid, *BaseCharacter->GetName());
			if (BaseCharacter->SandboxPlayerUid == PlayerInfo.PlayerUid) {
				MainCharacter = BaseCharacter;
			}
		}
	}

	if (!MainCharacter) {
		if (GetLevelController()) {
			auto Map = GetLevelController()->GetConservedCharacterMap();
			if (Map.Contains(PlayerInfo.PlayerUid)) {
				MainCharacter = Cast<ABaseCharacter>(GetLevelController()->SpawnCharacter(Map[PlayerInfo.PlayerUid]));
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
					if (NewCharacter) {
						MainCharacter->RebuildEquipment();
					}
				}
			}
		}

		if (MainCharacter) {
			UnPossess();
			SandboxPossess(MainCharacter);
		}
	}
}

void AMainPlayerController::ServerRpcFindOrCreateCharacter_Implementation() {
	FindOrCreateCharacterInternal();
}

void AMainPlayerController::PlayerTick(float DeltaTime) {
	Super::PlayerTick(DeltaTime);

	if (bFirstStart) {
		bFirstStart = false;

		FString PlayerUid = PlayerInfo.PlayerUid;
		ServerRpcRegisterSandboxPlayer(PlayerUid, GetVersionString());

		ADummyPawn* DummyPawn = Cast<ADummyPawn>(GetPawn());
		if (DummyPawn) {
			if (GetLevelController()) {
				auto Map = GetLevelController()->GetConservedCharacterMap();
				if (Map.Contains(PlayerUid)) {
					DummyPawn->SetActorLocation(Map[PlayerUid].Location);
				}
			}
		}

		//RpcFindOrCreateCharacter();

		bool bDebugOff = TerrainController && !TerrainController->IsDebugModeOn(); 
		if (bDebugOff) {
			BlockGameInput();
		}

		AMainHUD* MainHud = Cast<AMainHUD>(GetHUD());
		if (MainHud) {
			if (bDebugOff) {
				MainHud->OpenWidget(TEXT("loading_terrain"));
			}
		}
	}

	// OnPossess not works on client. workaround
	if (GetNetMode() == NM_Client) {
		ABaseCharacter* BaseCharacter = Cast<ABaseCharacter>(GetCharacter());
		if (BaseCharacter) {
			if (bClientPosses) {
				SetCurrentInventorySlot(-1);
				ShowInGameHud();
				bClientPosses = false;
			}
		}
	}

	/*
	if (HasAuthority()) {
		UE_LOG(LogTemp, Warning, TEXT("server local %s"), *UEnum::GetValueAsString(GetLocalRole()));
		UE_LOG(LogTemp, Warning, TEXT("server remote %s"), *UEnum::GetValueAsString(GetRemoteRole()));
	} else {
		UE_LOG(LogTemp, Warning, TEXT("client local %s"), *UEnum::GetValueAsString(GetLocalRole()));
		UE_LOG(LogTemp, Warning, TEXT("client remote %s"), *UEnum::GetValueAsString(GetRemoteRole()));
	}
	*/

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
			if (Environment) {
				Environment->UpdatePlayerPosition(Location, this);
			}

			if (TerrainController) {
				int R = TerrainController->CheckPlayerPositionZone(Location);

				if (R < -1) {
					UE_LOG(LogTemp, Warning, TEXT("Incorrect player position detected: %d"), R);
					SandboxTp(0, 0, 2);
				}
			}

		}
	}
}

void AMainPlayerController::OnStartBackgroundSave() {
	AMainHUD* MainHud = Cast<AMainHUD>(GetHUD());
	if (MainHud) {
		MainHud->OpenWidget(TEXT("background_save"));
	}
}

void AMainPlayerController::OnFinishBackgroundSave() {
	AMainHUD* MainHud = Cast<AMainHUD>(GetHUD());
	if (MainHud) {
		MainHud->CloseWidget(TEXT("background_save"));
	}
}

void AMainPlayerController::OnFinishInitialLoad() {
	AsyncTask(ENamedThreads::AnyBackgroundThreadNormalTask, [&] {
		FPlatformProcess::Sleep(0.5f);

		AsyncTask(ENamedThreads::GameThread, [&] {
			ServerRpcFindOrCreateCharacter();
			UnblockGameInput();


			if (GetNetMode() != NM_DedicatedServer) {
				bShowMouseCursor = false;
				UWidgetBlueprintLibrary::SetInputMode_GameOnly(this);

				AMainHUD* MainHud = Cast<AMainHUD>(GetHUD());
				if (MainHud) {
					MainHud->CloseWidget(TEXT("loading_terrain"));
					MainHud->OpenWidget(TEXT("debug_info"));
					MainHud->OpenWidget(TEXT("crosshair"));
				}
			}
		});
	});
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

ASandboxObject* AMainPlayerController::GetInventoryObject(int32 SlotId) {
	UContainerComponent* Inventory = GetInventory();
	if (Inventory != nullptr) {
		FContainerStack* Stack = Inventory->GetSlot(SlotId);
		if (Stack != nullptr) {
			if (Stack->Amount > 0) {
				return GetLevelController()->GetSandboxObject((Stack->SandboxClassId));
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

void AMainPlayerController::OnMainActionPressed() {
	ADummyPawn* DummyPawn = Cast<ADummyPawn>(GetPawn());
	if (DummyPawn) {
		return;
	}

	ABaseCharacter* BaseCharacter = Cast<ABaseCharacter>(GetCharacter());
	if (BaseCharacter) {
		if (BaseCharacter->IsDead()) {
			return;
		}

		MainPlayerControllerComponent->PerformMainAction();
	}
}

void AMainPlayerController::OnMainActionReleased() {

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

		if (GetNetMode() == NM_Client) {
			ServerRpcRebuildEquipment();
		}
	}
}

void AMainPlayerController::OnAltActionReleased() {
	ADummyPawn* DummyPawn = Cast<ADummyPawn>(GetPawn());
	if (DummyPawn) {
		return;
	}

	ABaseCharacter* BaseCharacter = Cast<ABaseCharacter>(GetCharacter());
	if (BaseCharacter) {
		MainPlayerControllerComponent->EndAltAction();
	}
}

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
		//SetRenderCustomDepth(SelectedPawn, false);
		SelectedPawn = nullptr;
	}

	if (PickedObj) {
		//SetRenderCustomDepth(PickedObj, false);
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

	if (Slot > 0 && IsGameInputBlocked()) {
		return;
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
		if (!IsGameInputBlocked()) {
			EnableGuiMode();
			OpenMainInventoryGui();
		}
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
		if (MainPlayerControllerComponent->ToggleCraftMode(Index)) {
			DisableGuiMode();
			CloseMainInventoryGui();
		}
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
		BaseCharacter->SandboxPlayerUid = PlayerInfo.PlayerUid;
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

void AMainPlayerController::OnRep_Pawn() {
	Super::OnRep_Pawn();
	SetupCamera();
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
			// do something 
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

bool AMainPlayerController::OnContainerDropCheck(int32 SlotId, FName ContainerName, const ASandboxObject* Obj) const {
	if (ContainerName == TEXT("Equipment")) {
		if (Obj->GetSandboxTypeId() == SandboxType_Equipment) {
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

void AMainPlayerController::OnWheelUp() {
	ABaseCharacter* BaseCharacter = Cast<ABaseCharacter>(GetCharacter());
	if (BaseCharacter) {
		if (BaseCharacter->IsDead()) {
			return;
		}

		MainPlayerControllerComponent->OnWheelUp();
	}
}

void AMainPlayerController::OnWheelDown() {
	ABaseCharacter* BaseCharacter = Cast<ABaseCharacter>(GetCharacter());
	if (BaseCharacter) {
		if (BaseCharacter->IsDead()) {
			return;
		}

		MainPlayerControllerComponent->OnWheelDown();
	}
}

void AMainPlayerController::SetCursorMesh(UStaticMesh* Mesh, const FVector& Location, const FRotator& Rotation, const FVector& Scale) {
	ABaseCharacter* BaseCharacter = Cast<ABaseCharacter>(GetCharacter());
	if (BaseCharacter) {
		if (CursorMaterial) {
			BaseCharacter->SetCursorMesh(Mesh, CursorMaterial, Location, Rotation, Scale);
		}
	}
}

void AMainPlayerController::ServerRpcDigTerrain_Implementation(int32 Type, FVector DigOrigin, FVector SpawnOrigin, float Size, int32 X, int32 Y, int32 Z, int32 FaceIndex) {
	if (TerrainController) {
		TVoxelIndex ZoneIndex(X, Y, Z);
		UVoxelMeshComponent* ZoneMesh = TerrainController->GetVoxelMeshComponent(ZoneIndex);

		if (!ZoneMesh) {
			return;
		}

		uint16 MatId = ZoneMesh->GetMaterialIdFromCollisionFaceIndex(FaceIndex);
		//UE_LOG(LogTemp, Warning, TEXT("MatId -> %d"), MatId);

		if (Type == 0) {
			TerrainController->DigTerrainRoundHole(DigOrigin, Size);	
		}

		if (Type == 1) {
			TerrainController->DigTerrainCubeHole(DigOrigin, Size);
		}

		FTransform Transform(SpawnOrigin);
		if (MatId > 0) {
			FSandboxTerrainMaterial MatInfo;
			TerrainController->GetTerrainMaterialInfoById(MatId, MatInfo);
			if (TerrainController->GetTerrainMaterialInfoById(MatId, MatInfo)) {
				if (MatInfo.Type == FSandboxTerrainMaterialType::Rock) {
					((ALevelController*)LevelController)->SpawnEffect(1, Transform);
				}

				if (MatInfo.Type == FSandboxTerrainMaterialType::Soil) {
					((ALevelController*)LevelController)->SpawnEffect(4, Transform);
				}
			}

		} else {
			((ALevelController*)LevelController)->SpawnEffect(1, Transform);
		}

	}
}

void AMainPlayerController::ServerRpcDestroyTerrainMesh_Implementation(int32 X, int32 Y, int32 Z, uint32 TypeId, uint32 VariantId, int32 Item, int EffectId, FVector EffectOrigin) {
	if (TerrainController) {
		TerrainController->RemoveInstanceAtMesh(TVoxelIndex(X, Y, Z), TypeId, VariantId, Item);

		if (EffectId > 0) {
			((ALevelController*)LevelController)->SpawnEffect(EffectId, FTransform(EffectOrigin));
		}
	}
}

void AMainPlayerController::RemoveTerrainMesh(UTerrainInstancedStaticMesh* TerrainMesh, int32 ItemIndex) {
	if (TerrainController) {
		TVoxelIndex Index = TerrainController->GetZoneIndex(TerrainMesh->GetComponentLocation());
		ServerRpcDestroyTerrainMesh(Index.X, Index.Y, Index.Z, TerrainMesh->MeshTypeId, TerrainMesh->MeshVariantId, ItemIndex, 0, FVector());
	}
}

void AMainPlayerController::ServerRpcDestroyActor_Implementation(int32 X, int32 Y, int32 Z, const FString& Name, FVector Origin) {
	if (TerrainController) {
		TVoxelIndex ZoneIndex(X, Y, Z);
		TerrainController->DestroySandboxObjectByName(TVoxelIndex(X, Y, Z), Name);

		((ALevelController*)LevelController)->SpawnEffect(2, FTransform(Origin));
	}
}

void AMainPlayerController::ServerRpcRemoveActor_Implementation(ASandboxObject* Obj) {
	if (LevelController) {
		((ALevelController*)LevelController)->RemoveSandboxObject(Obj);
	} else {
		Obj->Destroy();
	}
}

void AMainPlayerController::SandboxAddItem(int ItemId) {
	UE_LOG(LogTemp, Warning, TEXT("Add item id = %d"), ItemId);
	ServerRpcAddItem(ItemId);
}

void AMainPlayerController::ServerRpcAddItem_Implementation(int ItemId) {
	UContainerComponent* Inventory = GetInventory();
	if (Inventory && LevelController) {
		ASandboxObject* Obj = LevelController->GetSandboxObject(ItemId);
		if (Obj) {
			Inventory->AddObject(Obj);
		}
	}
}

void AMainPlayerController::SandboxTp(int X, int Y, int Z) {
	UE_LOG(LogTemp, Warning, TEXT("Teleport player %d to zone: %d %d %d"), GetUniqueID(), X, Y, Z);

	if (TerrainController) {
		APawn* MainPawn = GetPawn();
		if (MainPawn) {
			FVector Pos = TerrainController->GetZonePos(TVoxelIndex(X, Y, Z));
			MainPawn->SetActorLocation(Pos);
		}
	}

	if (GetNetMode() == NM_Client) {
		ServerRpcTp(X, Y, Z);
	}
}

void AMainPlayerController::ServerRpcTp_Implementation(int X, int Y, int Z) {
	SandboxTp(X, Y, Z);
}

void AMainPlayerController::SandboxExec(const FString& Cmd, const FString& Param) {
	UE_LOG(LogTemp, Warning, TEXT("SandboxExec: %s %s"), *Cmd, *Param);

	if (Cmd == "resync") {
		if (TerrainController) {
			UE_LOG(LogTemp, Warning, TEXT("Resync terrain"));
			TerrainController->ForceTerrainNetResync();
		} else {
			UE_LOG(LogTemp, Warning, TEXT("No terrain controller"));
		}
	}

	if (Cmd == "redraw") {
		if (TerrainController) {
			UE_LOG(LogTemp, Warning, TEXT("Redraw terrain zones"));
			TerrainController->UE51MaterialIssueWorkaround();
		} else {
			UE_LOG(LogTemp, Warning, TEXT("No terrain controller"));
		}
	}

	if (Cmd == "test_body") {
		ABaseCharacter* BaseCharacter = Cast<ABaseCharacter>(GetCharacter());
		if (BaseCharacter) {
			BaseCharacter->GetMesh()->SetMorphTarget("Breasts Implants", 1.f);
			BaseCharacter->GetMesh()->SetMorphTarget("Breasts Implants Right", 1.f);
			BaseCharacter->GetMesh()->SetMorphTarget("Breasts Implants Left", 1.f);
			BaseCharacter->GetMesh()->SetMorphTarget("Waist Width", -1.f);
			BaseCharacter->GetMesh()->SetMorphTarget("Fitness", 1.f);
			BaseCharacter->GetMesh()->SetMorphTarget("Glutes Size", 1.f);
			BaseCharacter->GetMesh()->SetMorphTarget("Glutes Width", 1.f);
			BaseCharacter->GetMesh()->SetMorphTarget("Body Size", 1.f);
		}
	}

	if (Cmd == "test_body2") {
		ABaseCharacter* BaseCharacter = Cast<ABaseCharacter>(GetCharacter());
		if (BaseCharacter) {
			BaseCharacter->GetMesh()->SetMorphTarget("Breasts Implants", 1.f);
			BaseCharacter->GetMesh()->SetMorphTarget("Breasts Implants Right", 0.f);
			BaseCharacter->GetMesh()->SetMorphTarget("Breasts Implants Left", 0.f);
			BaseCharacter->GetMesh()->SetMorphTarget("Waist Width", -1.f);
			BaseCharacter->GetMesh()->SetMorphTarget("Fitness", 1.f);
			BaseCharacter->GetMesh()->SetMorphTarget("Glutes Size", 0.5f);
			BaseCharacter->GetMesh()->SetMorphTarget("Glutes Width", 1.f);
			BaseCharacter->GetMesh()->SetMorphTarget("Body Size", 0.75f);
		}
	}

	if (Cmd == "event1") {

	}

}

void AMainPlayerController::ServerRpcSpawnObject_Implementation(uint64 SandboxClassId, const FTransform& Transform, bool bEnablePhysics) {
	if (LevelController) {
		ASandboxObject* Obj = LevelController->SpawnSandboxObject(SandboxClassId, Transform);
		if (Obj) {
			Obj->OnPlaceToWorld(); // invoke only if spawn by player. not save/load or other cases

			if (bEnablePhysics) {
				Obj->SandboxRootMesh->SetSimulatePhysics(true);
			}
		}
	}
}

void AMainPlayerController::SpawnObjectByPlayer(uint64 SandboxClassId, FTransform Transform) {
	if (LevelController) {
		ASandboxObject* Obj = LevelController->SpawnSandboxObject(SandboxClassId, Transform);
		if (Obj) {
			Obj->OnPlaceToWorld(); // invoke only if spawn by player. not save/load or other cases
		}
	}
}

