

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ElectricDevice.h"
#include "ElectricGenerator.generated.h"

UCLASS()
class DARKLITHOSPHERE_API AElectricGenerator : public AElectricDevice {
	GENERATED_BODY()
	
public:	

	AElectricGenerator();

protected:

	virtual void BeginPlay() override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:	

	UPROPERTY(EditAnywhere, Category = "Sandbox")
	UAudioComponent* MainSound;

	virtual void Tick(float DeltaTime) override;

	virtual void OnTerrainChange() override;

	virtual bool PlaceToWorldClcPosition(const UWorld* World, const FVector& SourcePos, const FRotator& SourceRotation, const FHitResult& Res, FVector& Location, FRotator& Rotation, bool bFinal) const override;

protected:

	virtual void OnHandleState() override;
};
