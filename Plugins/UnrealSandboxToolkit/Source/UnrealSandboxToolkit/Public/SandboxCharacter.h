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



UINTERFACE(MinimalAPI, Blueprintable)
class USandboxCoreCharacter : public UInterface {
	GENERATED_BODY()
};

class ISandboxCoreCharacter {
	GENERATED_BODY()

public:

	virtual int GetSandboxTypeId() = 0;

	virtual FString GetSandboxPlayerUid() = 0;

	virtual float GetStaminaTickDelta() { return 0; }

	virtual void OnStaminaExhausted() { }

};



UCLASS()
class UNREALSANDBOXTOOLKIT_API ASandboxCharacter : public ACharacter, public ISandboxCoreCharacter {
	GENERATED_BODY()

public:

	ASandboxCharacter();

	virtual void BeginPlay() override;
	
	virtual void Tick( float DeltaSeconds ) override;

	virtual void SetupPlayerInputComponent(class UInputComponent* InputComponent) override;

	UPROPERTY(EditAnywhere, Category = "UnrealSandbox Character")
	int SandboxTypeId = 0;

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

	int GetSandboxTypeId() override;

	FString GetSandboxPlayerUid() override;

	/*
	UFUNCTION(BlueprintImplementableEvent, Category = "DmgSystem")
	void TakeDamage(float DamageLevel);
	*/

	template<class T>
	T* GetFirstComponentByName(FString ComponentName) {
		TArray<T*> Components;
		GetComponents<T>(Components);
		for (T* Component : Components) {
			if (Component->GetName() == ComponentName)
				return Component;
		}

		return nullptr;
	}

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
