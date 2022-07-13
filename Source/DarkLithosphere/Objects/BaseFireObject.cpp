
#include "BaseFireObject.h"

ABaseFireObject::ABaseFireObject() {
	PrimaryActorTick.bCanEverTick = true;
	MaxLifetime = 60;
	Lifetime = 0;
}


bool ABaseFireObject::CanTake(AActor* actor) {
	return false;
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
		SetFlameVisibility(false);

		UPointLightComponent* LightComponent = GetFirstComponentByName<UPointLightComponent>(TEXT("FireLight"));
		if (LightComponent) {
			LightComponent->SetIntensity(0);
		}

		SandboxRootMesh->SetCastShadow(true);
	}
}

void ABaseFireObject::SetFlameScale(float Scale) {
	UParticleSystemComponent* Flame1 = GetFirstComponentByName<UParticleSystemComponent>(TEXT("Flame_01"));
	if (Flame1) {
		Flame1->SetRelativeScale3D(FVector(Scale, Scale, Scale));
	}

	UParticleSystemComponent* Flame2 = GetFirstComponentByName<UParticleSystemComponent>(TEXT("Flame_02"));
	if (Flame2) {
		Flame2->SetRelativeScale3D(FVector(Scale, Scale, Scale));
	}

	UParticleSystemComponent* Flame3 = GetFirstComponentByName<UParticleSystemComponent>(TEXT("Flame_03"));
	if (Flame3) {
		Flame3->SetRelativeScale3D(FVector(Scale, Scale, Scale));
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

void ABaseFireObject::Tick(float DeltaTime) {
	Super::Tick(DeltaTime);

	const auto& ParamBurnt = GetProperty(TEXT("Burnt"));
	if (ParamBurnt == "Y") {
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
			SetFlameVisibility(false);
			SandboxRootMesh->SetCastShadow(true);
		} else {
			SetProperty(TEXT("Lifetime"), FString::SanitizeFloat(Lifetime));
		}


		const float T2 = (MaxLifetime - Lifetime) / MaxLifetime;
		const float Intensity = InitialIntensity * T2;

		//UE_LOG(LogTemp, Warning, TEXT("T = %f"), T);

		const float FlameScale = (T2 > 0.4) ? T2 : 0.4;
		SetFlameScale(FlameScale);

		UPointLightComponent* LightComponent = GetFirstComponentByName<UPointLightComponent>(TEXT("FireLight"));
		if (LightComponent) {
			LightComponent->SetIntensity(Intensity);
		}
	}
}
