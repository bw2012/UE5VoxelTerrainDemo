

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SandboxObject.h"
#include "BaseObject.h"
#include "TechHelper.h"
#include "ElectricGenerator.generated.h"

UCLASS()
class DARKLITHOSPHERE_API AElectricGenerator : public ABaseObject, public IElectricityProducer {
	GENERATED_BODY()
	
public:	

	AElectricGenerator();

protected:

	virtual void BeginPlay() override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:	

	UPROPERTY(EditAnywhere, Category = "Sandbox")
	UAudioComponent* MainSound;

	UPROPERTY()
	class USoundBase* SwitchSound;

	virtual void Tick(float DeltaTime) override;

	virtual bool IsInteractive(const APawn* Source);

	virtual void MainInteraction(const APawn* Source);

	virtual void Enable();

	virtual void Disable();

	virtual void PostLoadProperties() override;

	UPROPERTY()
	ATechHelper* TechHelper;

	virtual void ProduceElectricPower(float& OutputPower) override;
};
