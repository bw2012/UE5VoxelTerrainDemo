
#include "Lamp.h"
#include "Net/UnrealNetwork.h"

ALamp::ALamp() {
	PrimaryActorTick.bCanEverTick = true;

	SwitchLightState(false);
}

void ALamp::BeginPlay() {
	Super::BeginPlay();
}

void ALamp::EndPlay(const EEndPlayReason::Type EndPlayReason) {
	Super::EndPlay(EndPlayReason);
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

void ALamp::OnHandleState() {
	if (ServerState > 0) {
		if (ServerFlagActive == 1) {
			SwitchLightState(true);
		}

		if (ServerFlagActive == 0) {
			SwitchLightState(false);
		}

	} else {
		SwitchLightState(false);
	}
}

