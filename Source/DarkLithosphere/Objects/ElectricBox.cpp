

#include "ElectricBox.h"


AElectricBox::AElectricBox() {
	PrimaryActorTick.bCanEverTick = true;
}

void AElectricBox::BeginPlay() {
	Super::BeginPlay();
}

void AElectricBox::Tick(float DeltaTime) {
	Super::Tick(DeltaTime);
}

void AElectricBox::OnDisable() {
	if (SatateOffMaterial) {
		SandboxRootMesh->SetMaterial(0, SatateOffMaterial);
	}
}

void AElectricBox::OnEnable() {
	if (SatateOnMaterial) {
		SandboxRootMesh->SetMaterial(0, SatateOnMaterial);
	}
}

