

#include "Candle.h"
#include "SandboxLevelController.h"


ACandle::ACandle() {
	PrimaryActorTick.bCanEverTick = true;
	MaxLifetime = 60;
	Lifetime = 0;
	bReplicates = true;
}


void ACandle::BeginPlay() {
	Super::BeginPlay();

	Timestamp = FPlatformTime::Seconds();

	Lifetime = 0;
	const auto& Param = GetProperty(TEXT("Lifetime"));
	if (Param != "") {
		Lifetime = FCString::Atof(*Param);
	}

	UPointLightComponent* LightComponent = GetFirstComponentByName<UPointLightComponent>(TEXT("CandleLight"));
	if (LightComponent) {
		InitialIntensity = LightComponent->Intensity;
	}
}

void ACandle::PostLoadProperties() {
	const auto& Param = GetProperty(TEXT("Lifetime"));
	if (Param != "") {
		Lifetime = FCString::Atof(*Param);
	}

	const auto& ParamBurnt = GetProperty(TEXT("Burnt"));
	if (ParamBurnt == "Y") {
		UParticleSystemComponent* Flame = GetFirstComponentByName<UParticleSystemComponent>(TEXT("Flame"));
		Flame->SetVisibility(false);

		UPointLightComponent* LightComponent = GetFirstComponentByName<UPointLightComponent>(TEXT("CandleLight"));
		if (LightComponent) {
			LightComponent->SetIntensity(0);
		}
	}
}

void ACandle::Tick(float DeltaTime) {
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
			UParticleSystemComponent* Flame = GetFirstComponentByName<UParticleSystemComponent>(TEXT("Flame"));
			Flame->SetVisibility(false);
		} else {
			SetProperty(TEXT("Lifetime"), FString::SanitizeFloat(Lifetime));
		}

		//UE_LOG(LogTemp, Warning, TEXT("Lifetime = %f"), Lifetime);
		float Intensity = InitialIntensity * (MaxLifetime - Lifetime) / MaxLifetime;

		UPointLightComponent* LightComponent = GetFirstComponentByName<UPointLightComponent>(TEXT("CandleLight"));
		if (LightComponent) {
			LightComponent->SetIntensity(Intensity);
		}
	}
}


bool ACandle::PlaceToWorldClcPosition(const UWorld* World, const FVector& SourcePos, const FRotator& SourceRotation, const FHitResult& Res, FVector& Location, FRotator& Rotation, bool bFinal) const {
	Super::PlaceToWorldClcPosition(World, SourcePos, SourceRotation, Res, Location, Rotation, bFinal);

	auto* Component = Res.GetComponent();
	if (Component) {
		FString ComponentName = Component->GetClass()->GetName();
		if (ComponentName == "TerrainInstancedStaticMesh") {
			return false;
		}
	}

	if (Res.Normal.Z < 0.45) {
		return false;
	}

	return true;
}

int ACandle::GetMaxStackSize() {
	return Super::GetMaxStackSize();
}

bool ACandle::CanTake(const AActor* Actor) const {
	const auto& ParamBurnt = GetProperty(TEXT("Burnt"));
	if (ParamBurnt == "Y") {
		return false;
	}

	if (Lifetime > MaxLifetime / 3) {
		return false;
	}

	return true;
}

void ACandle::OnTerrainChange() {
	ASandboxLevelController* Ctrl = ASandboxLevelController::GetInstance();
	if (Ctrl) {
		Ctrl->RemoveSandboxObject(this);
	}
}

void ACandle::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const {
	DOREPLIFETIME(ACandle, Lifetime);
}