

#pragma once

#include "CoreMinimal.h"
#include "SandboxEnvironment.h"
#include "EnvironmentController.generated.h"

/**
 * 
 */
UCLASS()
class DARKLITHOSPHERE_API AEnvironmentController : public ASandboxEnvironment
{
	GENERATED_BODY()
	

public:

	UPROPERTY(EditAnywhere, Category = "Sandbox")
	AAmbientSound* Night1AmbientSound;

	virtual void Tick(float DeltaSeconds) override;

private:
		
	bool bIsNightPrev = false;;
};
