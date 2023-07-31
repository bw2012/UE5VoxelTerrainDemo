

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TechHelper.generated.h"


#define ED_TYPE_SOURCE		1
#define ED_TYPE_TRANSMITER	2
#define ED_TYPE_ENDPOINT	3
#define ED_TYPE_TARGET		4


class AElectricDevice;


struct TElectricDeviceProxyItem {

	AElectricDevice* DeviceActor = nullptr;

	int Id = 0;
};



UCLASS()
class DARKLITHOSPHERE_API ATechHelper : public AActor {
	GENERATED_BODY()
	
public:	
	ATechHelper();

protected:

	virtual void BeginPlay() override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:	
	virtual void Tick(float DeltaTime) override;

	virtual void FinishDestroy() override;

	void RegisterElectricDevice(AElectricDevice* DeviceActor);

	void UnregisterElectricDevice(AElectricDevice* DeviceActor);

	void SetActiveElectricDevice(FString Name, int FlagActive);

	void RebuildEnergyNet();

	void DrawDebugEnergyNet();

private:

	TMap<FString, TElectricDeviceProxyItem> ElectricDeviceMap;
};
