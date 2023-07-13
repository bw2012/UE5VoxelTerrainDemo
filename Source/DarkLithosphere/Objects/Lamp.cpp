
#include "Lamp.h"
#include "Net/UnrealNetwork.h"

ALamp::ALamp() {
	PrimaryActorTick.bCanEverTick = true;

	//TODO
	auto SoundObj = ConstructorHelpers::FObjectFinder<USoundBase>(TEXT("/Game/Sandbox/Object/Industrial/A_ToggleSwitch"));
	if (SoundObj.Object != nullptr) {
		SwitchSound = SoundObj.Object;
	}
}

void ALamp::BeginPlay() {
	Super::BeginPlay();

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

	const auto& Param = GetProperty(TEXT("Enabled"));
	if (Param == "Y") {
		//EnableLight();
	} else {
		DisableLight();
	}
}

void ALamp::EndPlay(const EEndPlayReason::Type EndPlayReason) {
	if (TechHelper) {
		TechHelper->UnregisterElectricityConsumer(this);
	}
}

void ALamp::PostLoadProperties() {
	const auto& Param = GetProperty(TEXT("Enabled"));
	if (Param == "Y") {
		//EnableLight();
	} else {
		DisableLight();
	}
}

void ALamp::Tick(float DeltaTime) {
	Super::Tick(DeltaTime);
}

bool ALamp::IsInteractive(const APawn* Source) {
	return true;
}

void ALamp::MainInteraction(const APawn* Source) {
	if (SwitchSound) {
		UGameplayStatics::PlaySoundAtLocation(this, SwitchSound, GetActorLocation());
	}

	const auto& Param = GetProperty(TEXT("Enabled"));
	if (Param == "Y") {
		SetProperty(TEXT("Enabled"), TEXT("N"));
		DisableLight();
		ServerState = 0;
	} else {
		SetProperty(TEXT("Enabled"), TEXT("Y"));
		EnableLight();
		ServerState = 1;
	}
}

void ALamp::EnableLight() {
	SwitchState(true);
}

void ALamp::DisableLight() {
	SwitchState(false);
}

void ALamp::SwitchState(bool bIsEnable) {
	TArray<UPointLightComponent*> Components;
	GetComponents<UPointLightComponent>(Components);
	for (UPointLightComponent* LightComponent : Components) {
		bool T = LightComponent->IsVisible();
		if (T != bIsEnable) {
			LightComponent->SetVisibility(bIsEnable);
		}
	}
}

void ALamp::OnPlaceToWorld() {
	SetProperty(TEXT("Enabled"), TEXT("Y"));
	//EnableLight();
}

void ALamp::InElectricPower(float InputPower) {
	if (InputPower > 0) {
		const auto& Param = GetProperty(TEXT("Enabled"));
		if (Param == "Y") {
			EnableLight();
			bIsWorks = true;
		}
	} else {
		if (bIsWorks) {
			DisableLight();
			bIsWorks = false;
		}
	}
}

void ALamp::OnRep_State() {
	if (ServerState != LocalState) {

		if (ServerState > 0) {
			EnableLight();
		} else {
			DisableLight();
		}

		LocalState = ServerState;
	}
}

void ALamp::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const {
	DOREPLIFETIME(ALamp, ServerState);
}