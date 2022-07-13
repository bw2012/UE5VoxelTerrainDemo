

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TechHelper.generated.h"

class IElectricityProducer {

public:

	virtual void ProduceElectricPower(float& OutputPower) {

	};

};

class IElectricityConsumer {

public:

	virtual void InElectricPower(float InputPower) {

	};

};


UCLASS()
class DARKLITHOSPHERE_API ATechHelper : public AActor {
	GENERATED_BODY()
	
public:	
	ATechHelper();

protected:
	virtual void BeginPlay() override;

public:	
	virtual void Tick(float DeltaTime) override;

	void RegisterElectricityProducer(AActor* Actor);

	void UnregisterElectricityProducer(AActor* Actor);

	void RegisterElectricityConsumer(AActor* Actor);

	void UnregisterElectricityConsumer(AActor* Actor);

private:

	TMap<FString, TWeakObjectPtr<AActor>> ElectricityProducersMap;

	TMap<FString, TWeakObjectPtr<AActor>> ElectricityConsumersMap;
};
