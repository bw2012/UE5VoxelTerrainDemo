
#include "ElectricDevice.h"
#include "../LevelController.h"
#include "Net/UnrealNetwork.h"

AElectricDevice::AElectricDevice() {
	PrimaryActorTick.bCanEverTick = true;
	LinkType = FElectricDeviceLinkType::None;
	ElectricLinkDistance = 500.f;
}

void AElectricDevice::BeginPlay() {
	Super::BeginPlay();
	
	if (GetNetMode() == NM_Client) {
		return;
	}

	for (TActorIterator<ATechHelper> ActorItr(GetWorld()); ActorItr; ++ActorItr) {
		ATechHelper* Helper = Cast<ATechHelper>(*ActorItr);
		if (Helper) {
			TechHelper = Helper;
			TechHelper->RegisterElectricDevice(this);
			break;
		}
	}
}

void AElectricDevice::EndPlay(const EEndPlayReason::Type EndPlayReason) {
	Super::EndPlay(EndPlayReason);

	if (GetNetMode() == NM_Client) {
		return;
	}

	if (TechHelper) {
		TechHelper->UnregisterElectricDevice(this);
	}
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
		SetFlagActive(0);
	} else {
		SetProperty(TEXT("Enabled"), TEXT("Y"));
		SetFlagActive(1);
	}
}

void AElectricDevice::PostLoadProperties() {
	const auto& Param = GetProperty(TEXT("Enabled"));
	if (Param == "Y") {
		SetFlagActive(1);
	} else {
		SetFlagActive(0);
	}
}

void AElectricDevice::OnPlaceToWorld() {
	SetProperty(TEXT("Enabled"), TEXT("N"));
	ServerState = 0;
	SetFlagActive(0);
}

void AElectricDevice::SetFlagActive(int FlagActive) {
	ServerFlagActive = FlagActive;

	if (TechHelper) {
		TechHelper->SetActiveElectricDevice(GetName(), ServerFlagActive);
	}

	if (GetNetMode() != NM_Client) {
		OnHandleState();
	}
}

void AElectricDevice::OnHandleState() {

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
		//return false;
	}

	return true;
}

int AElectricDevice::GetElectricDeviceType() {
	return (int)LinkType;
}

void AElectricDevice::SetElectricDeviceServerState(int NewState) {
	if (GetNetMode() == NM_Client) {
		return;
	}

	ServerState = NewState; 
	OnHandleState();
}

void AElectricDevice::OnRep_State() {
	//UE_LOG(LogTemp, Log, TEXT("OnRep_State -> %d %d"), ServerState, LocalState);
	if (ServerState != LocalState) {
		OnHandleState();
		LocalState = ServerState;
	}
}

void AElectricDevice::OnRep_FlagActive() {
	//UE_LOG(LogTemp, Log, TEXT("OnRep_FlagActive -> %d"), ServerFlagActive);
	OnHandleState();
}

int AElectricDevice::GetElectricDeviceServerState() {
	return ServerState;
}

void AElectricDevice::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const {
	DOREPLIFETIME(AElectricDevice, ServerState);
	DOREPLIFETIME(AElectricDevice, ServerFlagActive);
}