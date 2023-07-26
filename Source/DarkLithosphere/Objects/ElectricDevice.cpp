
#include "ElectricDevice.h"
#include "../LevelController.h"
#include "Net/UnrealNetwork.h"

AElectricDevice::AElectricDevice() {
	PrimaryActorTick.bCanEverTick = true;

}

void AElectricDevice::BeginPlay() {
	Super::BeginPlay();
	
	if (GetNetMode() == NM_Client) {
		return;
	}

	/*
	for (TActorIterator<ATechHelper> ActorItr(GetWorld()); ActorItr; ++ActorItr) {
		ATechHelper* Helper = Cast<ATechHelper>(*ActorItr);
		if (Helper) {
			UE_LOG(LogTemp, Log, TEXT("Found ATechHelper -> %s"), *Helper->GetName());
			TechHelper = Helper;
			break;
		}
	}

	if (TechHelper) {
		TechHelper->RegisterElectricityConsumer(this);
	}
	*/

}

void AElectricDevice::Tick(float DeltaTime) {
	Super::Tick(DeltaTime);
}

bool AElectricDevice::IsInteractive(const APawn* Source) {
	return true;
}

void AElectricDevice::MainInteraction(const APawn* Source) {

	ASandboxLevelController* LevelController = ASandboxLevelController::GetInstance();
	if(LevelController){
		LevelController->SpawnEffect(EffectId, GetActorTransform());
	}

	const auto& Param = GetProperty(TEXT("Enabled"));
	if (Param == "Y") {
		SetProperty(TEXT("Enabled"), TEXT("N"));
		OnDisable();
		ServerState = 0;
	}
	else {
		SetProperty(TEXT("Enabled"), TEXT("Y"));
		OnEnable();
		ServerState = 1;
	}
}

void AElectricDevice::PostLoadProperties() {
	const auto& Param = GetProperty(TEXT("Enabled"));
	if (Param == "Y") {
		OnEnable();
	} else {
		OnDisable();
	}
}

void AElectricDevice::OnPlaceToWorld() {
	SetProperty(TEXT("Enabled"), TEXT("N"));
	ServerState = 0;
	OnDisable();
}

void AElectricDevice::OnDisable() {

}

void AElectricDevice::OnEnable() {

}

bool AElectricDevice::PlaceToWorldClcPosition(const UWorld* World, const FVector& SourcePos, const FRotator& SourceRotation, const FHitResult& Res, FVector& Location, FRotator& Rotation, bool bFinal) const {
	Location = Res.Location;

	const FVector R = -Res.Normal;
	Rotation = R.Rotation();

	return true;
}

bool AElectricDevice::CanTake(const AActor* Actor) const {
	const auto& Param = GetProperty(TEXT("Enabled"));
	if (Param == "Y") {
		return false;
	}

	return true;
}

void AElectricDevice::OnRep_State() {
	if (ServerState != LocalState) {

		if (ServerState > 0) {
			OnEnable();
		} else {
			OnDisable();
		}

		LocalState = ServerState;
	}
}

void AElectricDevice::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const {
	DOREPLIFETIME(AElectricDevice, ServerState);
}