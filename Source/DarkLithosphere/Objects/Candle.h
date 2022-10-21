

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BaseObject.h"
#include "Candle.generated.h"

UCLASS()
class DARKLITHOSPHERE_API ACandle : public ABaseObject
{
	GENERATED_BODY()
	
public:	
	ACandle();

protected:
	virtual void BeginPlay() override;

public:

	UPROPERTY(EditAnywhere, Category = "Sandbox")
	float MaxLifetime;

	virtual void Tick(float DeltaTime) override;

	virtual bool PlaceToWorldClcPosition(const UWorld* World, const FVector& SourcePos, const FRotator& SourceRotation, const FHitResult& Res, FVector& Location, FRotator& Rotation, bool bFinal) const override;

	virtual int GetMaxStackSize() override;

	virtual bool CanTake(const AActor* Actor = nullptr) const override;

	virtual void PostLoadProperties() override;

	virtual void OnTerrainChange() override;

private:

	double Timestamp;

	float InitialIntensity;

	float Lifetime;
};
