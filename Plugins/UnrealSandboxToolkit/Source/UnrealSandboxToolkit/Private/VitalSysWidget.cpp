// Copyright blackw 2015-2020

#include "VitalSysWidget.h"
#include "VitalSystemComponent.h"
#include "SandboxCharacter.h"


float USandboxVitalSysWidget::GetHealth() {
	return 0;
}

float USandboxVitalSysWidget::GetMaxHealth() {
	return 0;
}

UVitalSystemComponent* USandboxVitalSysWidget::GetVitalSystemComponent() {
	if (GetOwningPlayer()->GetPawn()) {
		TArray<UVitalSystemComponent*> Components;
		GetOwningPlayer()->GetPawn()->GetComponents<UVitalSystemComponent>(Components);
		for (UVitalSystemComponent* VitalSystemComponent : Components) {
			return VitalSystemComponent;
		}
	}

	return nullptr;
}


float USandboxVitalSysWidget::GetHealthInPercent() {

	UVitalSystemComponent* VitalSystemComponent = GetVitalSystemComponent();
	if (VitalSystemComponent) {
		const float Health = VitalSystemComponent->GetHealth();
		const float MaxHealth = VitalSystemComponent->GetMaxHealth();
		static const float AbsoluteMaxHealth = 1000.f;
		const float Res = Health / AbsoluteMaxHealth;
		return Res;
	}
	

	return 1.f;
}

float USandboxVitalSysWidget::GetStaminaInPercent() {
	
	UVitalSystemComponent* VitalSystemComponent = GetVitalSystemComponent();
	if (VitalSystemComponent) {
		const float Stamina = VitalSystemComponent->GetStamina();
		static const float AbsoluteMaxStamina = 1000.f;
		const float Res = Stamina / AbsoluteMaxStamina;
		return Res;
	}
	
	return 0.f;
}