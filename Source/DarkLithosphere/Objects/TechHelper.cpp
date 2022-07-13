


#include "TechHelper.h"

ATechHelper::ATechHelper() {
	PrimaryActorTick.bCanEverTick = true;

}

void ATechHelper::BeginPlay() {
	Super::BeginPlay();
	
}

void ATechHelper::Tick(float DeltaTime) {
	Super::Tick(DeltaTime);

	float TotalPower = 0;
	for (auto& Itm : ElectricityProducersMap) {
		auto ActorWeakPtr = Itm.Value;
		if (ActorWeakPtr.IsValid()) {
			IElectricityProducer* P = dynamic_cast<IElectricityProducer*>(ActorWeakPtr.Get());
			float Power = 0;
			P->ProduceElectricPower(Power);
			TotalPower += Power;
		}
	}


	for (auto& Itm : ElectricityConsumersMap) {
		auto ActorWeakPtr = Itm.Value;
		if (ActorWeakPtr.IsValid()) {
			IElectricityConsumer* C = dynamic_cast<IElectricityConsumer*>(ActorWeakPtr.Get());
			C->InElectricPower(TotalPower);
		}
	}
}

void ATechHelper::RegisterElectricityProducer(AActor* Actor) {
	UE_LOG(LogTemp, Log, TEXT("RegisterElectricityProducer -> %s"), *Actor->GetName());
	ElectricityProducersMap.Add(Actor->GetName(), TWeakObjectPtr<AActor>(Actor));
}

void ATechHelper::UnregisterElectricityProducer(AActor* Actor) {
	UE_LOG(LogTemp, Log, TEXT("UnregisterElectricityProducer -> %s"), *Actor->GetName());
	ElectricityProducersMap.Remove(Actor->GetName());
}

void ATechHelper::RegisterElectricityConsumer(AActor* Actor) {
	UE_LOG(LogTemp, Log, TEXT("RegisterElectricityConsumer -> %s"), *Actor->GetName());
	ElectricityConsumersMap.Add(Actor->GetName(), TWeakObjectPtr<AActor>(Actor));
}

void ATechHelper::UnregisterElectricityConsumer(AActor* Actor) {
	UE_LOG(LogTemp, Log, TEXT("UnregisterElectricityConsumer -> %s"), *Actor->GetName());
	ElectricityConsumersMap.Remove(Actor->GetName());
}