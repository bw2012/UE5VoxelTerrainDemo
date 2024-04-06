// Copyright blackw 2015-2020

#include "VitalSystemComponent.h"
#include "SandboxCharacter.h"
#include "SandboxObject.h"
#include "Net/UnrealNetwork.h"

#define STAMINA_CLC_FACTOR 0.1

#define STAMINA_REDUCTION_FACTOR 2


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

	if (Health <= 0) { 
		Health = 0; 
	}

	if (Health > MaxHealth) { 
		Health = MaxHealth; 
	}
}

void UVitalSystemComponent::DamageFromFall(float Velocity) {
	auto EquipmentList = ASandboxObjectUtils::GetContainerContent(GetOwner(), TEXT("Equipment"));
	float DamageFallFactor = 1.f;
	for (auto* Item : EquipmentList) {
		ASandboxSkeletalModule* Skm = Cast<ASandboxSkeletalModule>(Item);
		if (Skm) {
			DamageFallFactor *= Skm->GetInfluenceParam(TEXT("damage_fall_factor"));
		}
	}

	const float Dmg = Velocity * 100.f / 2000.f * DamageFallFactor;
	Damage(Dmg);
}

void UVitalSystemComponent::Damage(float DamageVal) {
	if (IsOwnerAdmin()) {
		return; 
	}

	ChangeHealth(-DamageVal);
	ChangeStamina(-DamageVal);

	if (Health == 0) {
		ISandboxCoreCharacter* SandboxCharacter = Cast<ISandboxCoreCharacter>(GetOwner());
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
		auto EquipmentList = ASandboxObjectUtils::GetContainerContent(GetOwner(), TEXT("Equipment"));

		float StaminaFactor = 1.f;
		float RecoverStaminaFactor = 1.f;

		for (auto* Item : EquipmentList) {
			ASandboxSkeletalModule* Skm = Cast<ASandboxSkeletalModule>(Item);
			if (Skm) {
				StaminaFactor *= Skm->GetInfluenceParam(TEXT("stamina_factor"));
				RecoverStaminaFactor *= Skm->GetInfluenceParam(TEXT("recover_stamina_factor"));
			}
		}

		const float D = -SandboxCharacter->GetStaminaTickDelta();
		if (D < 0) {
			const float TickStaminaReduction = D * StaminaFactor * STAMINA_CLC_FACTOR * STAMINA_REDUCTION_FACTOR;
			ChangeStamina(TickStaminaReduction);
		} else {
			// recover stamina
			if (!SandboxCharacter->IsDead()) {
				const float RecoverStamina = 1.f * RecoverStaminaFactor;
				ChangeStamina(RecoverStamina);
			}
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
	GetWorld()->GetTimerManager().SetTimer(Timer, this, &UVitalSystemComponent::PerformTimer, STAMINA_CLC_FACTOR, true);
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