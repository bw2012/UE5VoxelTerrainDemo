
#include "CoreCharacter.h"

ACoreCharacter::ACoreCharacter(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
	PrimaryActorTick.bCanEverTick = true;
	InitialOverlayState = EALSOverlayState::Default;
	//DefaultFootRotator = FRotator(0, 0, 0); // 0, 20, 10
}

void ACoreCharacter::BeginPlay() {
	Super::BeginPlay();
	//SetOverlayState(InitialOverlayState, true);

	LeftFootRotator = DefaultFootRotator; // 55, 5, 10 roll, pitch, yaw Pitch(InF), Yaw(InF), Roll(InF)
	RightFootRotator = FRotator(-DefaultFootRotator.Pitch, -DefaultFootRotator.Yaw, DefaultFootRotator.Roll);
	//RightFootRotator = DefaultFootRotator;
}

void ACoreCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent){
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAxis("Move Forward / Backward", this, &ACoreCharacter::PlayerForwardMovementInput);
	PlayerInputComponent->BindAxis("Move Right / Left", this, &ACoreCharacter::PlayerRightMovementInput);
	PlayerInputComponent->BindAxis("Look Up / Down Mouse", this, &ACoreCharacter::PlayerCameraUpInput);
	PlayerInputComponent->BindAxis("Turn Right / Left Mouse", this, &ACoreCharacter::PlayerCameraRightInput);
	PlayerInputComponent->BindAction("JumpAction", IE_Pressed, this, &ACoreCharacter::JumpPressedAction);
	PlayerInputComponent->BindAction("JumpAction", IE_Released, this, &ACoreCharacter::JumpReleasedAction);
	//PlayerInputComponent->BindAction("StanceAction", IE_Pressed, this, &AALSBaseCharacter::StancePressedAction);
	//PlayerInputComponent->BindAction("WalkAction", IE_Pressed, this, &AALSBaseCharacter::WalkPressedAction);
	PlayerInputComponent->BindAction("RagdollAction", IE_Pressed, this, &ACoreCharacter::RagdollPressedAction);
	//PlayerInputComponent->BindAction("SelectRotationMode_1", IE_Pressed, this, &AALSBaseCharacter::VelocityDirectionPressedAction);
	//PlayerInputComponent->BindAction("SelectRotationMode_2", IE_Pressed, this,&AALSBaseCharacter::LookingDirectionPressedAction);
	PlayerInputComponent->BindAction("SprintAction", IE_Pressed, this, &ACoreCharacter::SprintPressedAction);
	PlayerInputComponent->BindAction("SprintAction", IE_Released, this, &ACoreCharacter::SprintReleasedAction);
	//PlayerInputComponent->BindAction("AimAction", IE_Pressed, this, &AALSBaseCharacter::AimPressedAction);
	//PlayerInputComponent->BindAction("AimAction", IE_Released, this, &AALSBaseCharacter::AimReleasedAction);
	PlayerInputComponent->BindAction("CameraAction", IE_Pressed, this, &ACoreCharacter::CameraPressedAction);
	//PlayerInputComponent->BindAction("CameraAction", IE_Released, this, &AALSBaseCharacter::CameraReleasedAction);
}

bool ACoreCharacter::CanMove() {
	return true;
}

FRotator ACoreCharacter::GetLeftFootRotator() {
	return LeftFootRotator;
}

FVector ACoreCharacter::GetBoneScale(FName BoneName) {
	return FVector(1, 1.1, 1);
}

FRotator ACoreCharacter::GetRightFootRotator() {
	return RightFootRotator;
}

void ACoreCharacter::PlayerForwardMovementInput(float Value) {
	if (CanMove()) {
		ForwardMovementAction_Implementation(Value);
	}
}

void ACoreCharacter::PlayerRightMovementInput(float Value) {
	if (CanMove()) {
		RightMovementAction_Implementation(Value);
	}
}

void ACoreCharacter::PlayerCameraUpInput(float Value) {
	if (CanMove()) {
		CameraUpAction_Implementation(Value);
	}
}

void ACoreCharacter::PlayerCameraRightInput(float Value) {
	if (CanMove()) {
		CameraRightAction_Implementation(Value);
	}
}

void ACoreCharacter::SprintPressedAction() {
	if (CanMove()) {
		SetDesiredGait(EALSGait::Sprinting);
	}
}

void ACoreCharacter::SprintReleasedAction() {
	SetDesiredGait(EALSGait::Running);
}

void ACoreCharacter::JumpPressedAction() {
	if (CanMove()) {
		JumpAction_Implementation(true);
	}
}

void ACoreCharacter::JumpReleasedAction() {
	JumpAction_Implementation(false);
}

void ACoreCharacter::RagdollPressedAction() {
	RagdollAction_Implementation();
}

void ACoreCharacter::CameraPressedAction() {
	if (CanMove()) {
		// Switch camera mode
		if (ViewMode == EALSViewMode::FirstPerson) {
			SetViewMode(EALSViewMode::ThirdPerson);
		} else if (ViewMode == EALSViewMode::ThirdPerson) {
			SetViewMode(EALSViewMode::FirstPerson);
		}
	}
}