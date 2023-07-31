
#pragma once

#include "CoreMinimal.h"
#include "ElectricDevice.h"
#include "GameFramework/Actor.h"
#include "ElectricBox.generated.h"

UCLASS()
class DARKLITHOSPHERE_API AElectricBox : public AElectricDevice {
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, Category = "Sandbox")
	UMaterialInterface* SatateActiveMaterial;

	UPROPERTY(EditAnywhere, Category = "Sandbox")
	UMaterialInterface* SatateInactiveMaterial;

	UPROPERTY(EditAnywhere, Category = "Sandbox")
	UMaterialInterface* SatateOffMaterial;

public:	

	AElectricBox();

protected:

	virtual void BeginPlay() override;

public:	

	virtual void Tick(float DeltaTime) override;

protected:

	virtual void OnHandleState() override;

};
