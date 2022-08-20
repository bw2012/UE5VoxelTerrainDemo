
#include "SandboxPlayerController.h"
#include "Blueprint/AIBlueprintHelperLibrary.h"
#include "SandboxCharacter.h"
#include "SandboxObject.h"
#include "ContainerComponent.h"
#include "Blueprint/WidgetBlueprintLibrary.h"

ASandboxPlayerController::ASandboxPlayerController() {
	//bShowMouseCursor = true;
	DefaultMouseCursor = EMouseCursor::Default;
	CurrentInventorySlot = -1;
	bIsGameInputBlocked = false;
}

void ASandboxPlayerController::BeginPlay() {
	Super::BeginPlay();

	LevelController = nullptr;
	for (TActorIterator<ASandboxLevelController> ActorItr(GetWorld()); ActorItr; ++ActorItr) {
		ASandboxLevelController* LevelCtrl = Cast<ASandboxLevelController>(*ActorItr);
		if (LevelCtrl) {
			UE_LOG(LogTemp, Log, TEXT("Found ALevelController -> %s"), *LevelCtrl->GetName());
			LevelController = LevelCtrl;
			break;
		}
	}
}

void ASandboxPlayerController::PlayerTick(float DeltaTime) {
	Super::PlayerTick(DeltaTime);

	if (bMoveToMouseCursor)	{
		MoveToMouseCursor();
	}
}

void ASandboxPlayerController::SetupInputComponent() {
	// set up gameplay key bindings
	Super::SetupInputComponent();

	InputComponent->BindAction("MainAction", IE_Pressed, this, &ASandboxPlayerController::OnMainActionPressedInternal);
	InputComponent->BindAction("MainAction", IE_Released, this, &ASandboxPlayerController::OnMainActionReleasedInternal);

	InputComponent->BindAction("AltAction", IE_Pressed, this, &ASandboxPlayerController::OnAltActionPressedInternal);
	InputComponent->BindAction("AltAction", IE_Released, this, &ASandboxPlayerController::OnAltActionReleasedInternal);

	// support touch devices 
	//InputComponent->BindTouch(EInputEvent::IE_Pressed, this, &AUE4VoxelTerrainPlayerController::MoveToTouchLocation);
	//InputComponent->BindTouch(EInputEvent::IE_Repeat, this, &AUE4VoxelTerrainPlayerController::MoveToTouchLocation);

	InputComponent->BindAction("ToggleView", IE_Pressed, this, &ASandboxPlayerController::ToggleView);
}

void ASandboxPlayerController::MoveToMouseCursor() {
	FHitResult Hit;
	GetHitResultUnderCursor(ECC_WorldStatic, false, Hit);

	if (Hit.bBlockingHit) {
		// We hit something, move there
		SetNewMoveDestination(Hit.ImpactPoint);
	}
}

void ASandboxPlayerController::MoveToTouchLocation(const ETouchIndex::Type FingerIndex, const FVector Location) {
	FVector2D ScreenSpaceLocation(Location);

	FHitResult HitResult;
	GetHitResultAtScreenPosition(ScreenSpaceLocation, CurrentClickTraceChannel, true, HitResult);
	if (HitResult.bBlockingHit) {
		SetNewMoveDestination(HitResult.ImpactPoint);
	}
}

void ASandboxPlayerController::SetNewMoveDestination(const FVector DestLocation) {
	ASandboxCharacter* SandboxCharacter = Cast<ASandboxCharacter>(GetCharacter());

	if (SandboxCharacter) {
		float const Distance = FVector::Dist(DestLocation, SandboxCharacter->GetActorLocation());
		if (Distance > 120.0f) {
			UAIBlueprintHelperLibrary::SimpleMoveToLocation(this, DestLocation);
		}
	}
}

void ASandboxPlayerController::SetDestinationPressed() {
	ASandboxCharacter* SandboxCharacter = Cast<ASandboxCharacter>(GetCharacter());
	if (!SandboxCharacter) {
		return;
	}

	if (SandboxCharacter->GetSandboxPlayerView() != PlayerView::TOP_DOWN) {
		return;
	}

	if (SandboxCharacter->IsDead()) {
		return;
	}

	bMoveToMouseCursor = true;
}

void ASandboxPlayerController::SetDestinationReleased() {
	ASandboxCharacter* SandboxCharacter = Cast<ASandboxCharacter>(GetCharacter());
	if (!SandboxCharacter) {
		return;
	}

	if (SandboxCharacter->GetSandboxPlayerView() != PlayerView::TOP_DOWN) {
		return;
	}

	if (SandboxCharacter->IsDead()) {
		return;
	}

	bMoveToMouseCursor = false;
}

void ASandboxPlayerController::OnMainActionPressedInternal() {
	if (!IsGameInputBlocked()) {
		OnMainActionPressed();
	}
}

void ASandboxPlayerController::OnMainActionReleasedInternal() {
	if (!IsGameInputBlocked()) {
		OnMainActionReleased();
	}
}

void ASandboxPlayerController::OnAltActionPressedInternal() {
	if (!IsGameInputBlocked()) {
		OnAltActionPressed();
	}
}

void ASandboxPlayerController::OnAltActionReleasedInternal() {
	if (!IsGameInputBlocked()) {
		OnAltActionReleased();
	}
}

void ASandboxPlayerController::OnMainActionPressed() {

}

void ASandboxPlayerController::OnMainActionReleased() {

}

void ASandboxPlayerController::OnAltActionPressed() {

}

void ASandboxPlayerController::OnAltActionReleased() {

}

void ASandboxPlayerController::ToggleView() {
	if (IsGameInputBlocked()) {
		return;
	}

	
}

void ASandboxPlayerController::OnPossess(APawn* NewPawn) {
	Super::OnPossess(NewPawn);

	ASandboxCharacter* SandboxCharacter = Cast<ASandboxCharacter>(NewPawn);
	if (SandboxCharacter) {
		SandboxCharacter->EnableInput(this);
		if (SandboxCharacter->GetSandboxPlayerView() == PlayerView::TOP_DOWN) {
			//bShowMouseCursor = true;
		} else {
			//bShowMouseCursor = false;
		}
	}

	//bShowMouseCursor = false;
}

void ASandboxPlayerController::BlockGameInput() {
	//UWidgetBlueprintLibrary::SetInputMode_GameAndUI(this, nullptr, false, false);
	bIsGameInputBlocked = true;
	//bShowMouseCursor = true;
}

void ASandboxPlayerController::UnblockGameInput() {
	//UWidgetBlueprintLibrary::SetInputMode_GameOnly(this);
	bIsGameInputBlocked = false;
	//bShowMouseCursor = false;
}

void ASandboxPlayerController::TraceAndSelectActionObject() {
	if (!IsGameInputBlocked()) {
		FHitResult Res = TracePlayerActionPoint();
		OnTracePlayerActionPoint(Res);
		if (Res.bBlockingHit) {
			AActor* SelectedActor = Res.GetActor();
			if (SelectedActor) {
				SelectActionObject(SelectedActor);
			}
		}
	}
}

FHitResult ASandboxPlayerController::TracePlayerActionPoint() {
	ASandboxCharacter* SandboxCharacter = Cast<ASandboxCharacter>(GetCharacter());
	if (!SandboxCharacter) {
		return FHitResult();
	}

	if (SandboxCharacter->GetSandboxPlayerView() == PlayerView::THIRD_PERSON || SandboxCharacter->GetSandboxPlayerView() == PlayerView::FIRST_PERSON) {
		float MaxUseDistance = SandboxCharacter->InteractionTargetLength;

		if (SandboxCharacter->GetSandboxPlayerView() == PlayerView::THIRD_PERSON) {
			if (SandboxCharacter->GetCameraBoom() != NULL) {
				//MaxUseDistance = Character->GetCameraBoom()->TargetArmLength + MaxUseDistance;
			}
		}

		FVector CamLoc;
		FRotator CamRot;
		GetPlayerViewPoint(CamLoc, CamRot);

		const FVector StartTrace = CamLoc;
		const FVector Direction = CamRot.Vector();
		const FVector EndTrace = StartTrace + (Direction * MaxUseDistance);

		FCollisionQueryParams TraceParams(FName(TEXT("")), true, this);
		//TraceParams.bTraceAsyncScene = true;
		//TraceParams.bReturnPhysicalMaterial = false;

		TraceParams.bTraceComplex = true;
		TraceParams.bReturnFaceIndex = true;
		TraceParams.AddIgnoredActor(SandboxCharacter);

		FHitResult Hit(ForceInit);
		GetWorld()->LineTraceSingleByChannel(Hit, StartTrace, EndTrace, ECC_Visibility, TraceParams);

		return Hit;
	}

	if (SandboxCharacter->GetSandboxPlayerView() == PlayerView::TOP_DOWN) {
		FHitResult Hit;
		GetHitResultUnderCursor(ECC_Camera, false, Hit);
		return Hit;
	}

	return FHitResult();
}

void SetRenderCustomDepth2(AActor* Actor, bool RenderCustomDepth) {
	TArray<UStaticMeshComponent*> MeshComponentList;
	Actor->GetComponents<UStaticMeshComponent>(MeshComponentList);

	for (UStaticMeshComponent* MeshComponent : MeshComponentList) {
		MeshComponent->SetRenderCustomDepth(RenderCustomDepth);
	}
}

void ASandboxPlayerController::OnSelectActionObject(AActor* Actor) {
	SetRenderCustomDepth2(Actor, true);
}

void ASandboxPlayerController::OnDeselectActionObject(AActor* Actor) {
	SetRenderCustomDepth2(Actor, false);
}

void ASandboxPlayerController::SelectActionObject(AActor* Actor) {
	ASandboxObject* Obj = Cast<ASandboxObject>(Actor);

	if (SelectedObject != Obj) {
		if (SelectedObject != nullptr && SelectedObject->IsValidLowLevel()) {
			OnDeselectActionObject(SelectedObject);
		}
	}

	if (Obj != nullptr) {
		OnSelectActionObject(Obj);
		SelectedObject = Obj;
	} else {
		if (SelectedObject != nullptr && SelectedObject->IsValidLowLevel()) {
			OnDeselectActionObject(SelectedObject);
		}
	}
}

UContainerComponent* ASandboxPlayerController::GetContainerByName(FName ContainerName) {
	APawn* PlayerPawn = GetPawn();
	if (PlayerPawn) {
		TArray<UContainerComponent*> Components;
		PlayerPawn->GetComponents<UContainerComponent>(Components);

		for (UContainerComponent* Container : Components) {
			if (Container->GetName() == ContainerName.ToString()) {
				return Container;
			}
		}
	}

	return nullptr;
}

UContainerComponent* ASandboxPlayerController::GetInventory() {
	return GetContainerByName(TEXT("Inventory"));
}

bool ASandboxPlayerController::TakeObjectToInventory() {
	UContainerComponent* Inventory = GetInventory();

	if (Inventory != nullptr) {
		FHitResult ActionPoint = TracePlayerActionPoint();
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

						return true;
					}
				}
			}
		}
	}

	return false;
}

bool ASandboxPlayerController::OpenObjectContainer(ASandboxObject* Obj) {
	if (Obj != nullptr) {
		TArray<UContainerComponent*> Components;
		Obj->GetComponents<UContainerComponent>(Components);
		for (UContainerComponent* Container : Components) {
			if (Container->GetName() == "ObjectContainer") {
				this->OpenedObject = Obj;
				this->OpenedContainer = Container;
				return true;
			}
		}
	}

	return false;
}

bool ASandboxPlayerController::HasOpenContainer() { 
	return OpenedObject != nullptr; 
}

bool ASandboxPlayerController::TraceAndOpenObjectContainer() {
	FHitResult ActionPoint = TracePlayerActionPoint();

	if (ActionPoint.bBlockingHit) {
		ASandboxObject* Obj = Cast<ASandboxObject>(ActionPoint.GetActor());
		return OpenObjectContainer(Obj);
	}

	return false;
}

void ASandboxPlayerController::CloseObjectWithContainer() {
	this->OpenedObject = nullptr;
	this->OpenedContainer = nullptr;
}

void ASandboxPlayerController::OnTracePlayerActionPoint(const FHitResult& Res) {

}

bool ASandboxPlayerController::IsGameInputBlocked() {
	ASandboxCharacter* SandboxCharacter = Cast<ASandboxCharacter>(GetCharacter());
	if (SandboxCharacter && !SandboxCharacter->InputEnabled()) {
		return true;
	}

	return bIsGameInputBlocked; 
}


void ASandboxPlayerController::SetCurrentInventorySlot(int32 Slot) { 
	ASandboxCharacter* SandboxCharacter = Cast<ASandboxCharacter>(GetCharacter());
	if (SandboxCharacter && SandboxCharacter->IsDead()) {
		return;
	}

	CurrentInventorySlot = Slot; 
}

UContainerComponent* ASandboxPlayerController::GetOpenedContainer() { 
	return this->OpenedContainer; 
}

ASandboxObject* ASandboxPlayerController::GetOpenedObject() { 
	return this->OpenedObject; 
}

void  ASandboxPlayerController::ShowMouseCursor(bool bShowCursor) { 
	this->bShowMouseCursor = bShowCursor; 
};

void ASandboxPlayerController::OnContainerMainAction(int32 SlotId, FName ContainerName) {

}

void ASandboxPlayerController::OnContainerDropSuccess(int32 SlotId, FName SourceName, FName TargetName) {

}

bool ASandboxPlayerController::OnContainerDropCheck(int32 SlotId, FName ContainerName, ASandboxObject* Obj) {
	return true;
}