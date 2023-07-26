
#include "Lamp.h"
#include "Net/UnrealNetwork.h"

ALamp::ALamp() {
	PrimaryActorTick.bCanEverTick = true;
}

void ALamp::BeginPlay() {
	Super::BeginPlay();

	/*
	if (GetNetMode() == NM_Client) {
		return;
	}

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
	*/
}

void ALamp::EndPlay(const EEndPlayReason::Type EndPlayReason) {
	Super::EndPlay(EndPlayReason);
	//if (TechHelper) {
	//	TechHelper->UnregisterElectricityConsumer(this);
	//}
}

void ALamp::SwitchLightState(bool bIsEnable) {
	TArray<UPointLightComponent*> Components;
	GetComponents<UPointLightComponent>(Components);
	for (UPointLightComponent* LightComponent : Components) {
		bool T = LightComponent->IsVisible();
		if (T != bIsEnable) {
			LightComponent->SetVisibility(bIsEnable);
		}
	}
}

void ALamp::OnDisable() {
	SwitchLightState(false);
}

void ALamp::OnEnable() {
	SwitchLightState(true);
}
