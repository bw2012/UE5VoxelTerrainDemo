
#include "ElectricGenerator.h"


AElectricGenerator::AElectricGenerator() {
	PrimaryActorTick.bCanEverTick = true;
	MainSound = CreateDefaultSubobject<UAudioComponent>(TEXT("MainSound"));

	//TODO
	auto SoundObj = ConstructorHelpers::FObjectFinder<USoundBase>(TEXT("/Game/Sandbox/Object/Industrial/A_ToggleSwitch"));
	if (SoundObj.Object != nullptr) {
		SwitchSound = SoundObj.Object;
	}
}

void AElectricGenerator::BeginPlay() {
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
		TechHelper->RegisterElectricityProducer(this);
	}

	const auto& Param = GetProperty(TEXT("Enabled"));
	if (Param == "Y") {
		Enable();
	} else {
		Disable();
	}
}

void AElectricGenerator::EndPlay(const EEndPlayReason::Type EndPlayReason) {
	if (TechHelper) {
		TechHelper->UnregisterElectricityProducer(this);
	}
}

void AElectricGenerator::Tick(float DeltaTime) {
	Super::Tick(DeltaTime);

}

bool AElectricGenerator::IsInteractive(const APawn* Source) {
	return true; 
}

void AElectricGenerator::MainInteraction(const APawn* Source) {
	if (SwitchSound) {
		UGameplayStatics::PlaySoundAtLocation(this, SwitchSound, GetActorLocation());
	}

	const auto& Param = GetProperty(TEXT("Enabled"));
	if (Param == "Y") {
		Disable();
	} else {
		Enable();
	}
}

void AElectricGenerator::Enable() {
	SetProperty(TEXT("Enabled"), TEXT("Y"));
	MainSound->Play();
}

void AElectricGenerator::Disable() {
	SetProperty(TEXT("Enabled"), TEXT("N"));
	MainSound->Stop();
}

void AElectricGenerator::PostLoadProperties() {
	const auto& Param = GetProperty(TEXT("Enabled"));
	if (Param == "Y") {
		Enable();
	} else {
		Disable();
	}
}

void AElectricGenerator::ProduceElectricPower(float& OutputPower) {
	const auto& Param = GetProperty(TEXT("Enabled"));
	if (Param == "Y") {
		OutputPower = 1;
	}
}