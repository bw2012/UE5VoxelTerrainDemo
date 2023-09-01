

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

void AElectricBox::PostLoad() {
	Super::PostLoad();

	if (SatateOffMaterial) {
		SandboxRootMesh->SetMaterial(0, SatateOffMaterial);
	}
}

void AElectricBox::OnHandleState() {
	if (ServerState > 0) {
		if (ServerFlagActive == 1) {
			if (SatateActiveMaterial) {
				SandboxRootMesh->SetMaterial(0, SatateActiveMaterial);
			}
		}

		if (ServerFlagActive == 0) {
			if (SatateOffMaterial) {
				SandboxRootMesh->SetMaterial(0, SatateOffMaterial);
			}
		}

	} else {
		if (SatateInactiveMaterial) {
			SandboxRootMesh->SetMaterial(0, SatateInactiveMaterial);
		}
	}
}