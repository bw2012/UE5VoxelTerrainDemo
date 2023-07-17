// Copyright blackw 2015-2020

#include "VitalSystemComponent.h"
#include "SandboxCharacter.h"
#include "Net/UnrealNetwork.h"

UVitalSystemComponent::UVitalSystemComponent() {
//	bWantsBeginPlay = true;
	PrimaryComponentTick.bCanEverTick = false;

	Health = 100;
	MaxHealth = 100;

	Stamina = 60;
	MaxStamina = 60;
}

bool UVitalSystemComponent::IsOwnerAdmin() {
	return false;
}

float UVitalSystemComponent::GetHealth() {
	return Health;
}

float UVitalSystemComponent::GetMaxHealth() {
	return MaxHealth;
}

void UVitalSystemComponent::ChangeHealth(float Val){
	Health += Val;
	if (Health <= 0) { Health = 0; }
	if (Health > MaxHealth) { Health = MaxHealth; }
}

void UVitalSystemComponent::Damage(float DamageVal) {
	if (IsOwnerAdmin()) { return; }
	ChangeHealth(-DamageVal);
	ChangeStamina(-DamageVal);
	if (Health == 0) {
		ASandboxCharacter* SandboxCharacter = Cast<ASandboxCharacter>(GetOwner());
		if (SandboxCharacter) {
			SandboxCharacter->Kill();
		}
	}
}

float UVitalSystemComponent::GetStamina() {
	return Stamina;
}

float UVitalSystemComponent::GetMaxStamina() {
	return MaxStamina;
}

void UVitalSystemComponent::ChangeStamina(float Val) {
	if (IsOwnerAdmin() && Val < 0) { 
		return; 
	}

	Stamina += Val;

	if (Stamina <= 0) { 
		Stamina = 0; 
	}

	if (Stamina > MaxStamina) { 
		Stamina = MaxStamina;
	}
}

void UVitalSystemComponent::PerformTimer() {
	ISandboxCoreCharacter* SandboxCharacter = Cast<ISandboxCoreCharacter>(GetOwner());
	if (SandboxCharacter) {
		const float D = -SandboxCharacter->GetStaminaTickDelta();
		if (D < 0) {
			const float TickStaminaReduction = D * 0.1;
			ChangeStamina(TickStaminaReduction);
		} else {
			ChangeStamina(1.f);
		}

		if (Stamina == 0) {
			SandboxCharacter->OnStaminaExhausted();
		}
	}
}

bool UVitalSystemComponent::CheckStamina(float Val) {
	return Stamina > Val;
}

void UVitalSystemComponent::BeginPlay() {
	Super::BeginPlay();
	GetWorld()->GetTimerManager().SetTimer(Timer, this, &UVitalSystemComponent::PerformTimer, 0.1, true);
}

void UVitalSystemComponent::EndPlay(const EEndPlayReason::Type EndPlayReason) {

}

void UVitalSystemComponent::TickComponent( float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction ) {
	Super::TickComponent( DeltaTime, TickType, ThisTickFunction );
}

void UVitalSystemComponent::GetLifetimeReplicatedProps(TArray< FLifetimeProperty > & OutLifetimeProps) const {
	DOREPLIFETIME(UVitalSystemComponent, Health);
	DOREPLIFETIME(UVitalSystemComponent, MaxHealth);
	DOREPLIFETIME(UVitalSystemComponent, Stamina);
	DOREPLIFETIME(UVitalSystemComponent, MaxStamina);
}