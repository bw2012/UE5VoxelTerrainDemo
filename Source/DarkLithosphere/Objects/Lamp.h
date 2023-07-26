

#pragma once

#include "CoreMinimal.h"
#include "ElectricDevice.h"
#include "Lamp.generated.h"

/**
 * 
 */
UCLASS()
class DARKLITHOSPHERE_API ALamp : public AElectricDevice {
	GENERATED_BODY()

public:

	ALamp();

protected:

	virtual void BeginPlay() override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

protected:

	virtual void OnDisable();

	virtual void OnEnable();

private:

	bool bIsWorks = false;

	void SwitchLightState(bool bIsEnable);

};
