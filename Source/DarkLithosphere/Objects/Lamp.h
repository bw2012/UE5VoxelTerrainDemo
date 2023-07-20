

#pragma once

#include "CoreMinimal.h"
#include "BaseObject.h"
#include "TechHelper.h"
#include "Lamp.generated.h"

/**
 * 
 */
UCLASS()
class DARKLITHOSPHERE_API ALamp : public ABaseObject, public IElectricityConsumer {
	GENERATED_BODY()

public:

	ALamp();

protected:

	virtual void BeginPlay() override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:

	UPROPERTY()
	class USoundBase* SwitchSound;

	UPROPERTY()
	ATechHelper* TechHelper;

	virtual void Tick(float DeltaTime) override;

	virtual bool IsInteractive(const APawn* Source) override;

	virtual void MainInteraction(const APawn* Source) override;

	virtual void EnableLight();

	virtual void DisableLight();

	virtual void SwitchState(bool bIsEnable);

	virtual void PostLoadProperties() override;

	virtual void OnPlaceToWorld() override;

	virtual void InElectricPower(float InputPower) override;

private:

	UPROPERTY(ReplicatedUsing = OnRep_State)
	int ServerState = 0;

	UPROPERTY()
	int LocalState = -999;

	bool bIsWorks = false;

	UFUNCTION()
	void OnRep_State();

};
