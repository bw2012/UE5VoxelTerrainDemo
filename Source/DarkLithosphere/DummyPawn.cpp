

#include "DummyPawn.h"


ADummyPawn::ADummyPawn() {
	PrimaryActorTick.bCanEverTick = true;

	MaxZoom = 3300;
	MinZoom = 400;
	ZoomStep = 50;
}

void ADummyPawn::BeginPlay() {
	Super::BeginPlay();

	AutoPossessPlayer = EAutoReceiveInput::Player0;

	auto CameraBoom = FindComponentByClass<USpringArmComponent>();
	if (CameraBoom) {
		InitialTargetArmLength = CameraBoom->TargetArmLength;
	}

	auto CursorMeshComponent = GetCursorMesh();
	if (CursorMeshComponent) {
		CursorMeshComponent->SetVisibility(false, true);
	}
	
}

void ADummyPawn::Tick(float DeltaTime) {
	Super::Tick(DeltaTime);
}

void ADummyPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) {
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	InputComponent->BindAxis("Move Forward / Backward", this, &ADummyPawn::MoveForward);
	InputComponent->BindAxis("Move Right / Left", this, &ADummyPawn::MoveRight);

	InputComponent->BindAction("Mouse Wheel Up", IE_Released, this, &ADummyPawn::ZoomIn);
	InputComponent->BindAction("Mouse Wheel Down", IE_Released, this, &ADummyPawn::ZoomOut);
}


void ADummyPawn::MoveForward(float Value) {
	AddMovementInput(FVector(1, 0, 0), Value);
}

void ADummyPawn::MoveRight(float Value) {
	AddMovementInput(FVector(0, 1, 0), Value);
}

void ADummyPawn::ZoomIn() {
	auto CameraBoom = FindComponentByClass<USpringArmComponent>();
	if (CameraBoom) {
		if (CameraBoom->TargetArmLength > MinZoom) {
			CameraBoom->TargetArmLength -= ZoomStep;
			UE_LOG(LogTemp, Warning, TEXT("ZoomIn: %f"), CameraBoom->TargetArmLength);
		}	
	}
}

void ADummyPawn::ZoomOut() {
	auto CameraBoom = FindComponentByClass<USpringArmComponent>();
	if (CameraBoom) {
		if (CameraBoom->TargetArmLength < MaxZoom) {
			CameraBoom->TargetArmLength += ZoomStep;
			UE_LOG(LogTemp, Warning, TEXT("ZoomOut: %f"), CameraBoom->TargetArmLength);
		} 
	}
}


UStaticMeshComponent* ADummyPawn::GetCursorMesh() {
	TArray<UStaticMeshComponent*> Components;
	GetComponents<UStaticMeshComponent>(Components);

	for (auto* Mesh : Components) {
		if (Mesh->GetName() == "CursorStaticMesh") {
			return Mesh;
		}
	}

	return nullptr;
}


void ADummyPawn::SetSandboxMode(int Mode) {
	this->SandboxMode = Mode;

	auto CursorMeshComponent = GetCursorMesh();
	if (CursorMeshComponent) {
		CursorMeshComponent->SetVisibility(false, true);
	}

	if (Mode == this->SandboxMode) {
		return;
	}
}


void ADummyPawn::OnMainAction(const FHitResult& CursorHit) {

	if (CursorHit.bBlockingHit) {
//		UE_LOG(LogTemp, Warning, TEXT("test -> %s"), *CursorHit.Actor->GetName());
	}

}