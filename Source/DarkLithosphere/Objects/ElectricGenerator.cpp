
#include "ElectricGenerator.h"


AElectricGenerator::AElectricGenerator() {
	PrimaryActorTick.bCanEverTick = true;
	MainSound = CreateDefaultSubobject<UAudioComponent>(TEXT("MainSound"));
	MainSound->AttachToComponent(SandboxRootMesh, FAttachmentTransformRules::KeepRelativeTransform);
	MainSound->Stop();
}

void AElectricGenerator::BeginPlay() {
	Super::BeginPlay();
}

void AElectricGenerator::EndPlay(const EEndPlayReason::Type EndPlayReason) {
	Super::EndPlay(EndPlayReason);
}

void AElectricGenerator::Tick(float DeltaTime) {
	Super::Tick(DeltaTime);
}

void AElectricGenerator::OnDisable() {
	MainSound->Stop();
}

void AElectricGenerator::OnEnable() {
	MainSound->Play();
}

void AElectricGenerator::OnTerrainChange() {
	Super::OnTerrainChange();
	SetProperty(TEXT("Enabled"), TEXT("N"));
}

bool AElectricGenerator::PlaceToWorldClcPosition(const UWorld* World, const FVector& SourcePos, const FRotator& SourceRotation, const FHitResult& Res, FVector& Location, FRotator& Rotation, bool bFinal) const {
	Location = Res.Location;
	Rotation.Pitch = 0;
	Rotation.Roll = 0;
	Rotation.Yaw = SourceRotation.Yaw;
	return true;
}