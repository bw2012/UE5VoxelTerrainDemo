// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "EngineMinimal.h"
#include "GameFramework/Character.h"
#include "Runtime/UMG/Public/UMG.h"
#include "SlateBasics.h"
#include "SandboxCharacter.generated.h"


class UVitalSystemComponent;

UENUM(BlueprintType)
enum class PlayerView : uint8 {
	TOP_DOWN = 0		UMETA(DisplayName = "Top Down"),
	THIRD_PERSON = 1	UMETA(DisplayName = "Third Person"),
	FIRST_PERSON = 2	UMETA(DisplayName = "First Person")
};

UCLASS()
class UNREALSANDBOXTOOLKIT_API ASandboxCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	ASandboxCharacter();

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	
	// Called every frame
	virtual void Tick( float DeltaSeconds ) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* InputComponent) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class UCameraComponent* FollowCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class UCameraComponent* FirstPersonCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class USpringArmComponent* CameraBoom;

	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }

	UFUNCTION(BlueprintCallable, Category = "UnrealSandbox Character")
	void InitTopDownView();

	UFUNCTION(BlueprintCallable, Category = "UnrealSandbox Character")
	void InitThirdPersonView();

	UFUNCTION(BlueprintCallable, Category = "UnrealSandbox Character")
	void InitFirstPersonView();

	PlayerView GetSandboxPlayerView();

	void SetSandboxPlayerView(PlayerView SandboxView);

	UPROPERTY(EditAnywhere, Category = "UnrealSandbox Character")
	PlayerView InitialView = PlayerView::TOP_DOWN;

	UPROPERTY(EditAnywhere, Category = "UnrealSandbox Character")
	bool bEnableAutoSwitchView = true;

	bool IsDead() { return bIsDead; }

	UFUNCTION(BlueprintCallable, Category = "UnrealSandbox Character")
	void Kill();

	UFUNCTION(BlueprintCallable, Category = "UnrealSandbox Character")
	void LiveUp();

	void Jump() override;

	void StopJumping() override;

	UPROPERTY(EditAnywhere, Category = "UnrealSandbox Character")
	float MaxZoom;

	UPROPERTY(EditAnywhere, Category = "UnrealSandbox Character")
	float MaxZoomTopDown;

	UPROPERTY(EditAnywhere, Category = "UnrealSandbox Character")
	float MinZoom;

	UPROPERTY(EditAnywhere, Category = "UnrealSandbox Character")
	float ZoomStep;

	UPROPERTY(EditAnywhere, Category = "UnrealSandbox Character")
	float WalkSpeed;

	UPROPERTY(EditAnywhere, Category = "UnrealSandbox Character")
	float RunSpeed;

	UPROPERTY(EditAnywhere, Category = "UnrealSandbox Character")
	float InteractionTargetLength;

	UPROPERTY(EditAnywhere, Category = "UnrealSandbox Character")
	float VelocityHitThreshold;

	UPROPERTY(EditAnywhere, Category = "UnrealSandbox Character")
	float VelocityHitFactor;

	double VelocityHitTimestamp;

	void BoostOn();

	void BoostOff();

	/*
	UFUNCTION(BlueprintImplementableEvent, Category = "DmgSystem")
	void TakeDamage(float DamageLevel);
	*/

private:
	PlayerView CurrentPlayerView;

	bool bIsDead = false;

	FTransform InitialMeshTransform;

	UFUNCTION()
	void OnHit(class UPrimitiveComponent* HitComp, class AActor* Actor, class UPrimitiveComponent* Other, FVector Impulse, const FHitResult & HitResult);

	//UPROPERTY()
	//UVitalSystemComponent* VitalSystemComponent;

protected:
	void ZoomIn();

	void ZoomOut();

	virtual void OnDeath();

	//void Test();

	/** Called for forwards/backward input */
	void MoveForward(float Value);

	/** Called for side to side input */
	void MoveRight(float Value);

	/**
	* Called via input to turn at a given rate.
	* @param Rate	This is a normalized rate, i.e. 1.0 means 100% of desired turn rate
	*/
	void TurnAtRate(float Rate);

	/**
	* Called via input to turn look up/down at a given rate.
	* @param Rate	This is a normalized rate, i.e. 1.0 means 100% of desired turn rate
	*/
	void LookUpAtRate(float Rate);

	virtual void AddControllerYawInput(float Val) override;

	virtual void AddControllerPitchInput(float Val) override;

	virtual FVector GetThirdPersonViewCameraPos();

	virtual FRotator GetTopDownViewCameraRot();

	virtual bool CanMove();
	
};
