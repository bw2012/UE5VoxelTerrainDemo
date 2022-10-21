
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BaseObject.h"
#include "Furnace.generated.h"


struct FFurnaceReceipe {

	int RawMatClass = 0;

	int ProductClass = 0;

	float ProcessingTime = 0;

};

UCLASS()
class DARKLITHOSPHERE_API AFurnace : public ABaseObject
{
	GENERATED_BODY()
	
public:	
	AFurnace();

protected:
	virtual void BeginPlay() override;

public:	
	virtual void Tick(float DeltaTime) override;

	virtual bool CanTake(const AActor* Actor = nullptr) const override;

	virtual bool IsContainer();

	virtual FName GetContainerWidgetName();

	virtual void PostLoadProperties() override;

private:

	TMap<int, FFurnaceReceipe> FurnaceReceipMap;

	void SetFlameVisibility(bool Visibility);

	void SetFlameScale(float Scale);

	double Timestamp;

	float Lifetime;

	bool bIsActive;

	int CurrentReceipeId;

	float ProcessTime;

};
