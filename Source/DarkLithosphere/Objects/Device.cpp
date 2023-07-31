
#include "Device.h"
#include "../LevelController.h"
#include "Net/UnrealNetwork.h"

ADevice::ADevice() {
	PrimaryActorTick.bCanEverTick = true;
}

void ADevice::BeginPlay() {
	Super::BeginPlay();
	
	if (GetNetMode() == NM_Client) {
		return;
	}

}

void ADevice::EndPlay(const EEndPlayReason::Type EndPlayReason) {
	Super::EndPlay(EndPlayReason);

	if (GetNetMode() == NM_Client) {
		return;
	}
}

void ADevice::Tick(float DeltaTime) {
	Super::Tick(DeltaTime);
}

bool ADevice::IsInteractive(const APawn* Source) {
	return true;
}

void ADevice::MainInteraction(const APawn* Source) {

	ASandboxLevelController* LevelController = ASandboxLevelController::GetInstance();
	if(LevelController){
		LevelController->SpawnEffect(EffectId, GetActorTransform());
	}

	const auto& Param = GetProperty(TEXT("Enabled"));
	if (Param == "Y") {
		SetProperty(TEXT("Enabled"), TEXT("N"));
		OnDisable();
		ServerState = 0;
	} else {
		SetProperty(TEXT("Enabled"), TEXT("Y"));
		OnEnable();
		ServerState = 1;
	}
}

void ADevice::PostLoadProperties() {
	const auto& Param = GetProperty(TEXT("Enabled"));
	if (Param == "Y") {
		OnEnable();
	} else {
		OnDisable();
	}
}

void ADevice::OnPlaceToWorld() {
	SetProperty(TEXT("Enabled"), TEXT("N"));
	ServerState = 0;
	OnDisable();
}

void ADevice::OnDisable() {

}

void ADevice::OnEnable() {

}

bool ADevice::PlaceToWorldClcPosition(const UWorld* World, const FVector& SourcePos, const FRotator& SourceRotation, const FHitResult& Res, FVector& Location, FRotator& Rotation, bool bFinal) const {
	Location = Res.Location;

	const FVector R = -Res.Normal;
	Rotation = R.Rotation();

	return true;
}

bool ADevice::CanTake(const AActor* Actor) const {
	const auto& Param = GetProperty(TEXT("Enabled"));
	if (Param == "Y") {
		//return false;
	}

	return true;
}

void ADevice::OnRep_State() {
	if (ServerState != LocalState) {

		if (ServerState > 0) {
			OnEnable();
		} else {
			OnDisable();
		}

		LocalState = ServerState;
	}
}

void ADevice::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const {
	DOREPLIFETIME(ADevice, ServerState);
}