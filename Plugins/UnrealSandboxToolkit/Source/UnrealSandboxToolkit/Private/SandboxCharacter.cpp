// Fill out your copyright notice in the Description page of Project Settings.

#include "SandboxCharacter.h"
//#include "SandboxPlayerController.h"
//#include "VitalSystemComponent.h"


ASandboxCharacter::ASandboxCharacter() {
	VelocityHitThreshold = 1300;
	VelocityHitFactor = 0.2f;
	VelocityHitTimestamp = 0;

	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); // Attach the camera to the end of the boom and let the boom adjust to match the controller orientation
	FollowCamera->bUsePawnControlRotation = false; // Camera does not rotate relative to arm
	FollowCamera->SetRelativeLocation(FVector(0, 0, 0)); // Position the camera

	FirstPersonCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCamera->SetupAttachment(GetMesh(), TEXT("head"));
	FirstPersonCamera->SetRelativeLocation(FVector(10.0f, 32.0f, 0.f)); // Position the camera
	FirstPersonCamera->SetRelativeRotation(FRotator(0, 90, -90));
	FirstPersonCamera->bUsePawnControlRotation = true;

	GetCapsuleComponent()->OnComponentHit.AddDynamic(this, &ASandboxCharacter::OnHit);

	// initial view
	CurrentPlayerView = PlayerView::TOP_DOWN;
	InitTopDownView();

	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	MaxZoom = 500;
	MaxZoomTopDown = 1200;
	MinZoom = 100;
	ZoomStep = 50;

	WalkSpeed = 200;
	RunSpeed = 600;

	InteractionTargetLength = 200;

	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
}

void ASandboxCharacter::BeginPlay() {
	Super::BeginPlay();
	
	CurrentPlayerView = InitialView;

	if (InitialView == PlayerView::TOP_DOWN) {
		InitTopDownView();
	}

	if (InitialView == PlayerView::THIRD_PERSON) {
		InitThirdPersonView();
	}

	if (InitialView == PlayerView::FIRST_PERSON) {
		InitFirstPersonView();
	}

	/*
	TArray<UVitalSystemComponent*> Components;
	GetComponents<UVitalSystemComponent>(Components);

	for (UVitalSystemComponent* VitalSysCmp : Components) {
		VitalSystemComponent = VitalSysCmp;
		break;
	}
	*/
}

void ASandboxCharacter::Tick( float DeltaTime ) {
	Super::Tick( DeltaTime );

	if (IsDead()) {
		FVector MeshLoc = GetMesh()->GetSocketLocation(TEXT("pelvis"));
		//GetCapsuleComponent()->SetWorldLocation(MeshLoc - InitialMeshTransform.GetLocation());
	}
}

void ASandboxCharacter::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) {
	Super::SetupPlayerInputComponent(InputComponent);

	//PlayerInputComponent->BindAction("ZoomIn", IE_Released, this, &ASandboxCharacter::ZoomIn);
	//PlayerInputComponent->BindAction("ZoomOut", IE_Released, this, &ASandboxCharacter::ZoomOut);

	PlayerInputComponent->BindAction("Boost", IE_Pressed, this, &ASandboxCharacter::BoostOn);
	PlayerInputComponent->BindAction("Boost", IE_Released, this, &ASandboxCharacter::BoostOff);

	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ASandboxCharacter::Jump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &ASandboxCharacter::StopJumping);

	PlayerInputComponent->BindAxis("Move Forward / Backward", this, &ASandboxCharacter::MoveForward);
	PlayerInputComponent->BindAxis("Move Right / Left", this, &ASandboxCharacter::MoveRight);

	//InputComponent->BindAction("Test", IE_Pressed, this, &ASandboxCharacter::Test);

	// We have 2 versions of the rotation bindings to handle different kinds of devices differently
	// "turn" handles devices that provide an absolute delta, such as a mouse.
	// "turnrate" is for devices that we choose to treat as a rate of change, such as an analog joystick
	PlayerInputComponent->BindAxis("Turn Right / Left Mouse", this, &ASandboxCharacter::AddControllerYawInput);
	//PlayerInputComponent->BindAxis("TurnRate", this, &ASandboxCharacter::TurnAtRate);
	PlayerInputComponent->BindAxis("Look Up / Down Mouse", this, &ASandboxCharacter::AddControllerPitchInput);
	//PlayerInputComponent->BindAxis("LookUpRate", this, &ASandboxCharacter::LookUpAtRate);
}


bool ASandboxCharacter::CanMove() {
	//ASandboxPlayerController* C = Cast<ASandboxPlayerController>(GetController());
	//if (!C || C->IsGameInputBlocked()) {
	//	return false;
	//}

	return true;
}

void ASandboxCharacter::BoostOn() {
	if (!CanMove()) {
		return;
	}

	/*
	TArray<UVitalSystemComponent*> Components;
	GetComponents<UVitalSystemComponent>(Components);
	if (Components.Num() > 0) {
		UVitalSystemComponent* Vs = Components[0];
		if (Vs->CanBoost()) {
			GetCharacterMovement()->MaxWalkSpeed = RunSpeed;
			return;
		}
	} else {
		GetCharacterMovement()->MaxWalkSpeed = RunSpeed;
	}
	*/
}

void ASandboxCharacter::BoostOff() {
	if (!CanMove()) {
		return;
	}

	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
}

void ASandboxCharacter::Jump() {
	if (!CanMove()) {
		return;
	}

	if (CurrentPlayerView == PlayerView::TOP_DOWN) {
		return;
	}

	if (IsDead()) {
		return;
	}

	Super::Jump();
}

void ASandboxCharacter::StopJumping() {
	if (IsDead()) {
		return;
	}

	Super::StopJumping();
}


void ASandboxCharacter::ZoomIn() {
	if (!CanMove()) {
		return;
	}

	if (GetCameraBoom() == NULL) {
		return;
	}

	if (CurrentPlayerView == PlayerView::FIRST_PERSON) {
		return;
	}

	if (GetCameraBoom()->TargetArmLength > MinZoom) {
		GetCameraBoom()->TargetArmLength -= ZoomStep;
	} else {
		if (bEnableAutoSwitchView) {
			InitFirstPersonView();
		}
	}

	//UE_LOG(LogVt, Log, TEXT("ZoomIn: %f"), GetCameraBoom()->TargetArmLength);
}

void ASandboxCharacter::ZoomOut() {
	if (!CanMove()) {
		return;
	}

	if (GetCameraBoom() == NULL) return;

	if (CurrentPlayerView == PlayerView::FIRST_PERSON) {
		if (bEnableAutoSwitchView) {
			InitThirdPersonView();
			return;
		}
	};

	float MZ = (CurrentPlayerView == PlayerView::TOP_DOWN) ? MaxZoomTopDown : MaxZoom;

	if (GetCameraBoom()->TargetArmLength < MZ) {
		GetCameraBoom()->TargetArmLength += ZoomStep;
	}

	//UE_LOG(LogVt, Log, TEXT("ZoomOut: %f"), GetCameraBoom()->TargetArmLength);
}

FVector ASandboxCharacter::GetThirdPersonViewCameraPos() {
	return FVector(0, 0, 64);
}

FRotator ASandboxCharacter::GetTopDownViewCameraRot() {
	return FRotator(-50.f, 0.f, 0.f); //FRotator(-60.f, 0.f, 0.f)
}

void ASandboxCharacter::InitTopDownView() {
	if (IsDead()) {
		return;
	}

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true; // Rotate character to moving direction
	GetCharacterMovement()->RotationRate = FRotator(0.f, 640.f, 0.f);
	GetCharacterMovement()->bConstrainToPlane = true;
	GetCharacterMovement()->bSnapToPlaneAtStart = true;

	CameraBoom->SetUsingAbsoluteRotation(true); // Don't want arm to rotate when character does

	CameraBoom->SetRelativeRotation(GetTopDownViewCameraRot());
	CameraBoom->TargetArmLength = MaxZoomTopDown;
	CameraBoom->bDoCollisionTest = false; // Don't want to pull camera in when it collides with level
	CameraBoom->bUsePawnControlRotation = false; // Rotate the arm based on the controller
	CameraBoom->ProbeSize = 0;
	CameraBoom->SetRelativeLocation(FVector(0, 0, 0));

	FirstPersonCamera->Deactivate();
	FollowCamera->Activate();

	bUseControllerRotationYaw = false;

	CurrentPlayerView = PlayerView::TOP_DOWN;

	/*
	ASandboxPlayerController* C = Cast<ASandboxPlayerController>(GetController());
	if (C != NULL) {
		C->ShowMouseCursor(true);
	}
	*/
}

void ASandboxCharacter::InitThirdPersonView() {
	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true; // Character moves in the direction of input...
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 540.0f, 0.0f); // ...at this rotation rate
	//GetCharacterMovement()->JumpZVelocity = 600.f;
	//GetCharacterMovement()->AirControl = 0.2f;

	CameraBoom->TargetArmLength = 300.0f; // The camera follows at this distance behind the character
	CameraBoom->bUsePawnControlRotation = true; // Rotate the arm based on the controller
	CameraBoom->bDoCollisionTest = true;
	CameraBoom->ProbeSize = 12;
	CameraBoom->SetRelativeLocation(GetThirdPersonViewCameraPos());

	FirstPersonCamera->Deactivate();
	FollowCamera->Activate();

	bUseControllerRotationYaw = false;

	CurrentPlayerView = PlayerView::THIRD_PERSON;

	/*
	ASandboxPlayerController* Controller = Cast<ASandboxPlayerController>(GetController());
	if (Controller) {
		Controller->ShowMouseCursor(false);
	}
	*/
}

void ASandboxCharacter::InitFirstPersonView() {
	if (IsDead()) {
		return;
	}

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = false; // Character moves in the direction of input...
	//GetCharacterMovement()->JumpZVelocity = 600.f;
	//GetCharacterMovement()->AirControl = 0.2f;

	FirstPersonCamera->Activate();
	FollowCamera->Deactivate();

	bUseControllerRotationYaw = true;

	CurrentPlayerView = PlayerView::FIRST_PERSON;

	/*
	ASandboxPlayerController* Controller = Cast<ASandboxPlayerController>(GetController());
	if (Controller) {
		Controller->ShowMouseCursor(false);
	}
	*/
}

void ASandboxCharacter::AddControllerYawInput(float Val) {
	if (!CanMove()) {
		return;
	}

	//if (Controller->IsGameInputBlocked() && CurrentPlayerView != PlayerView::THIRD_PERSON) {
		//return;
	//}

	if (CurrentPlayerView == PlayerView::TOP_DOWN) {
		return;
	}

	Super::AddControllerYawInput(Val);

}

void ASandboxCharacter::AddControllerPitchInput(float Val) {
	if (!CanMove()) {
		return;
	}

	/*
	ASandboxPlayerController* Controller = Cast<ASandboxPlayerController>(GetController());
	if (Controller->IsGameInputBlocked() && CurrentPlayerView != PlayerView::THIRD_PERSON) {
		//return;
	}
	*/

	if (CurrentPlayerView == PlayerView::TOP_DOWN){
		return;
	}

	Super::AddControllerPitchInput(Val);
}

void ASandboxCharacter::TurnAtRate(float Rate) {
	if (!CanMove()) {
		return;
	}

	if (CurrentPlayerView == PlayerView::TOP_DOWN) {
		return;
	}

	// calculate delta for this frame from the rate information
	//AddControllerYawInput(Rate * BaseTurnRate * GetWorld()->GetDeltaSeconds());
}

void ASandboxCharacter::LookUpAtRate(float Rate) {
	if (!CanMove()) {
		return;
	}

	if (CurrentPlayerView == PlayerView::TOP_DOWN) {
		return;
	}

	// calculate delta for this frame from the rate information
	//AddControllerPitchInput(Rate * BaseLookUpRate * GetWorld()->GetDeltaSeconds());
}


void ASandboxCharacter::MoveForward(float Value) {
	if (!CanMove()) {
		return;
	}

	if (IsDead()) { 
		return; 
	};

	if (CurrentPlayerView == PlayerView::THIRD_PERSON) {
		if (Value != 0.0f)	{
			// find out which way is forward
			const FRotator Rotation = Controller->GetControlRotation();
			const FRotator YawRotation(0, Rotation.Yaw, 0);

			// get forward vector
			const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
			AddMovementInput(Direction, Value);
		}
	}

	if (CurrentPlayerView == PlayerView::FIRST_PERSON) {
		if (Value != 0.0f)	{
			// add movement in that direction
			AddMovementInput(GetActorForwardVector(), Value);
		}
	}
}

void ASandboxCharacter::MoveRight(float Value) {
	if (!CanMove()) {
		return;
	}

	if (IsDead()) { 
		return;
	};

	if (CurrentPlayerView == PlayerView::THIRD_PERSON) {
		if (Value != 0.0f) {
			// find out which way is right
			const FRotator Rotation = Controller->GetControlRotation();
			const FRotator YawRotation(0, Rotation.Yaw, 0);

			// get right vector 
			const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
			// add movement in that direction
			AddMovementInput(Direction, Value);
		}
	}

	if (CurrentPlayerView == PlayerView::FIRST_PERSON) {
		if (Value != 0.0f) {
			// add movement in that direction
			AddMovementInput(GetActorRightVector(), Value);
		}
	}
}

PlayerView ASandboxCharacter::GetSandboxPlayerView() {
	return CurrentPlayerView;
}

void ASandboxCharacter::SetSandboxPlayerView(PlayerView SandboxView) {
	CurrentPlayerView = SandboxView;
}

/*
void ASandboxCharacter::Test() {
	if(!IsDead()) {
		Kill();
	} else {
		LiveUp();
	}
}
*/

void ASandboxCharacter::OnDeath() {
	GetMesh()->SetSimulatePhysics(true);
	GetMesh()->SetAllBodiesBelowPhysicsBlendWeight(TEXT("pelvis"), 1);
}

void ASandboxCharacter::Kill() {
	if (!IsDead()) {
		if (CurrentPlayerView == PlayerView::FIRST_PERSON) {
			InitThirdPersonView();
		}

		InitialMeshTransform = GetMesh()->GetRelativeTransform();
		bIsDead = true;

		OnDeath();
	}
}

void ASandboxCharacter::LiveUp() {
	if (IsDead()) {
		GetMesh()->SetSimulatePhysics(false);
		bIsDead = false;
		GetMesh()->SetupAttachment(GetCapsuleComponent(), NAME_None);
		GetMesh()->SetRelativeTransform(InitialMeshTransform);
	}
}

int ASandboxCharacter::GetSandboxTypeId() {
	return SandboxTypeId;
}

FString ASandboxCharacter::GetSandboxPlayerUid() {
	return "";
}

void ASandboxCharacter::OnHit(class UPrimitiveComponent* HitComp, class AActor* Actor, class UPrimitiveComponent* Other, FVector Impulse, const FHitResult & HitResult) {
	/*
	float HitVelocity = GetCapsuleComponent()->GetComponentVelocity().Size();
	if (VitalSystemComponent != nullptr) {
		if (HitVelocity > VelocityHitThreshold) {
			const double Timestamp = FPlatformTime::Seconds();
			const double D = Timestamp - VelocityHitTimestamp;
			if (D > 0.5) {
				UE_LOG(LogTemp, Log, TEXT("HitVelocity -> %f"), HitVelocity);
				VelocityHitTimestamp = Timestamp;
				const float Damage = (HitVelocity - VelocityHitThreshold) * VelocityHitFactor;
				VitalSystemComponent->Damage(Damage);
			}

		}
	}
	*/
}