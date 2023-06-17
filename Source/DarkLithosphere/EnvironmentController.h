

#pragma once

#include "CoreMinimal.h"
#include "SandboxEnvironment.h"
#include "EnvironmentController.generated.h"

class ATerrainController;

/**
 * 
 */
UCLASS()
class DARKLITHOSPHERE_API AEnvironmentController : public ASandboxEnvironment {
	GENERATED_BODY()
	

public:

	UPROPERTY(EditAnywhere, Category = "SandboxTest")
	TSubclassOf<AActor> TestActor;

	UPROPERTY(EditAnywhere, Category = "Sandbox")
	ATerrainController* TerrainController;

	UPROPERTY(EditAnywhere, Category = "Sandbox")
	AAmbientSound* Night1AmbientSound;

	virtual void UpdatePlayerPosition(FVector Pos, APlayerController* Controller);

	int DayNumber = 0;

protected:

	virtual void Tick(float DeltaSeconds) override;

	void BeginPlay();

	virtual float ClcHeightFactor() const;

private:
		
	bool bIsNightPrev = false;

	float PlayerZLevel = 0;
};
