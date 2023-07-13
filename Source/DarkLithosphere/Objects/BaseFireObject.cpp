

#include "BaseFireObject.h"
#include "Net/UnrealNetwork.h"
#include "SandboxLevelController.h"


ABaseFireObject::ABaseFireObject() {
	PrimaryActorTick.bCanEverTick = true;
	MaxLifetime = 60;
	Lifetime = 0;
	bReplicates = true;
}

bool ABaseFireObject::CanTake(const AActor* Actor) const {
	return false;
}

void ABaseFireObject::OnTerrainChange() {
	if (ASandboxLevelController::GetInstance()) {
		ASandboxLevelController::GetInstance()->RemoveSandboxObject(this);
	}
}

void ABaseFireObject::BeginPlay() {
	Super::BeginPlay();

	Timestamp = FPlatformTime::Seconds();

	Lifetime = 0;
	const auto& Param = GetProperty(TEXT("Lifetime"));
	if (Param != "") {
		Lifetime = FCString::Atof(*Param);
	}

	UPointLightComponent* LightComponent = GetFirstComponentByName<UPointLightComponent>(TEXT("FireLight"));
	if (LightComponent) {
		InitialIntensity = LightComponent->Intensity;
	}
}

void ABaseFireObject::PostLoadProperties() {
	const auto& Param = GetProperty(TEXT("Lifetime"));
	if (Param != "") {
		Lifetime = FCString::Atof(*Param);
	}

	const auto& ParamBurnt = GetProperty(TEXT("Burnt"));
	if (ParamBurnt == "Y") {
		State = -1;
		Lifetime = MaxLifetime;
		SetBurnt();
	}
}

TArray<FString> ComponentNames = { TEXT("Flame_01"), TEXT("Flame_02"), TEXT("Flame_03") };

const TArray<FString>& ABaseFireObject::GetFlameComponentsName() const {
	return ComponentNames;
}

void ABaseFireObject::SetFlameScale(FString Name, float Scale) {
	UParticleSystemComponent* Flame = GetFirstComponentByName<UParticleSystemComponent>(Name);
	if (Flame) {
		Flame->SetRelativeScale3D(FVector(Scale, Scale, Scale));
	}
}

void ABaseFireObject::SetAllFlameScale(float Scale) {
	const TArray<FString>& List = GetFlameComponentsName();

	for (const auto& Name : List) {
		SetFlameScale(Name, Scale);
	}
}

void ABaseFireObject::SetFlameVisibility(bool Visibility) {
	UParticleSystemComponent* Flame1 = GetFirstComponentByName<UParticleSystemComponent>(TEXT("Flame_01"));
	if (Flame1) {
		Flame1->SetVisibility(false);
	}

	UParticleSystemComponent* Flame2 = GetFirstComponentByName<UParticleSystemComponent>(TEXT("Flame_02"));
	if (Flame2) {
		Flame2->SetVisibility(false);
	}

	UParticleSystemComponent* Flame3 = GetFirstComponentByName<UParticleSystemComponent>(TEXT("Flame_03"));
	if (Flame3) {
		Flame3->SetVisibility(false);
	}
}

void ABaseFireObject::OnRep_State() {
	if (State < 0) {
		SetBurnt();
	}
}

void ABaseFireObject::SetBurnt() {
	SetFlameVisibility(false);

	UPointLightComponent* LightComponent = GetFirstComponentByName<UPointLightComponent>(TEXT("FireLight"));
	if (LightComponent) {
		LightComponent->SetIntensity(0);
	}

	SandboxRootMesh->SetCastShadow(true);
}

void ABaseFireObject::Tick(float DeltaTime) {
	Super::Tick(DeltaTime);

	if (GetNetMode() != NM_Client) {
		const auto& ParamBurnt = GetProperty(TEXT("Burnt"));
		if (ParamBurnt == "Y") {
			State = -1;
		}
	} else {
		//UE_LOG(LogTemp, Warning, TEXT("Lifetime = %f"), Lifetime);
	}

	if (State < 0) {
		return;
	}

	double T = FPlatformTime::Seconds();
	double Delta = T - Timestamp;
	if (Delta > 1) {
		Lifetime += Delta;
		Timestamp = T;

		if (Lifetime >= MaxLifetime) {
			Lifetime = MaxLifetime;
			SetProperty(TEXT("Burnt"), TEXT("Y"));
			RemoveProperty(TEXT("Lifetime"));
			SetBurnt();
			SandboxRootMesh->SetCastShadow(true);
		} else {
			SetProperty(TEXT("Lifetime"), FString::SanitizeFloat(Lifetime));
		}

		const float T2 = (MaxLifetime - Lifetime) / MaxLifetime;
		const float Intensity = InitialIntensity * T2;

		const float FlameScale = (T2 > 0.4) ? T2 : 0.4;
		SetAllFlameScale(FlameScale);

		UPointLightComponent* LightComponent = GetFirstComponentByName<UPointLightComponent>(TEXT("FireLight"));
		if (LightComponent) {
			LightComponent->SetIntensity(Intensity);
		}
	}
}

void ABaseFireObject::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const {
	DOREPLIFETIME(ABaseFireObject, Lifetime);
	DOREPLIFETIME(ABaseFireObject, State);
}