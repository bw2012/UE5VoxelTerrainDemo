
#pragma once

#include "CoreMinimal.h"
#include "BaseObject.h"
#include "TechHelper.h"
#include "GameFramework/Actor.h"
#include "Device.generated.h"



UCLASS()
class DARKLITHOSPHERE_API ADevice : public ABaseObject {
	GENERATED_BODY()
	
public:	

	ADevice();

public:	

	UPROPERTY(EditAnywhere, Category = "Sandbox")
	int EffectId;

protected:

	virtual void BeginPlay() override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:

	virtual void Tick(float DeltaTime) override;

	virtual void PostLoadProperties() override;

	virtual void OnPlaceToWorld() override;

	virtual bool IsInteractive(const APawn* Source) override;

	virtual void MainInteraction(const APawn* Source) override;

	virtual bool PlaceToWorldClcPosition(const UWorld* World, const FVector& SourcePos, const FRotator& SourceRotation, const FHitResult& Res, FVector& Location, FRotator& Rotation, bool bFinal) const override;

	virtual bool CanTake(const AActor* Actor = nullptr) const override;

protected:

	UPROPERTY(ReplicatedUsing = OnRep_State)
	int ServerState = 0;

	UPROPERTY()
	int LocalState = -999;

	UFUNCTION()
	void OnRep_State();

	virtual void OnDisable();

	virtual void OnEnable();

};
