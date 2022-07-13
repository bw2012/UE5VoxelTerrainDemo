

#pragma once

#include "CoreMinimal.h"
#include "Character/ALSCharacter.h"
#include "CoreCharacter.generated.h"

/**
 * 
 */
UCLASS()
class DARKLITHOSPHERE_API ACoreCharacter : public AALSCharacter
{
	GENERATED_BODY()
	
public:

	ACoreCharacter(const FObjectInitializer& ObjectInitializer);

	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

protected:

	virtual void BeginPlay() override;

public:

	void PlayerForwardMovementInput(float Value);

	void PlayerRightMovementInput(float Value);

	void PlayerCameraUpInput(float Value);

	void PlayerCameraRightInput(float Value);

	void SprintPressedAction();

	void SprintReleasedAction();

	void JumpPressedAction();

	void JumpReleasedAction();

	void RagdollPressedAction();

	void CameraPressedAction();

public:

	UPROPERTY(EditAnywhere, Category = "Sandbox")
	EALSOverlayState InitialOverlayState;

	/*
	void JumpAction(bool bValue);

	void SprintAction(bool bValue);

	void AimAction(bool bValue);

	void CameraTapAction();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "ALS|Input")
		void CameraHeldAction();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "ALS|Input")
		void StanceAction();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "ALS|Input")
		void WalkAction();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "ALS|Input")
		void RagdollAction();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "ALS|Input")
		void VelocityDirectionAction();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "ALS|Input")
		void LookingDirectionAction();

		*/

};
