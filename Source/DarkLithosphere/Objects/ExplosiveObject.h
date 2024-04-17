#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BaseObject.h"
#include "ExplosiveObject.generated.h"


#define EXPLOSIVE_OBJ_STATE_INACTIVE	0
#define EXPLOSIVE_OBJ_STATE_ACTIVE		1
#define EXPLOSIVE_OBJ_STATE_PERFORM		2

UCLASS()
class DARKLITHOSPHERE_API AExplosiveObject : public ABaseObject
{
	GENERATED_BODY()

public:
	AExplosiveObject();

protected:
	virtual void BeginPlay() override;

public:

	UPROPERTY(EditAnywhere, Category = "Sandbox")
	float BaseDamage = 100.f;

	UPROPERTY(EditAnywhere, Category = "Sandbox")
	float TerrainDamageRadius = 300.f;

	UPROPERTY(EditAnywhere, Category = "Sandbox")
	TSubclassOf<USandboxDamageType> DamageType;

	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser);

	virtual void Tick(float DeltaTime) override;

	virtual bool CanTake(const AActor* Actor = nullptr) const override;

	virtual void PostLoadProperties() override;

	virtual bool IsZoneAnchor() override;

	virtual bool IsInteractive(const APawn* Source) override;

	virtual void MainInteraction(const APawn* Source) override;

	virtual void PerformExplosion();

	int GetExplosiveState();

	float GetDamageRadius();

	void PerformActivation();

private:

	UPROPERTY(ReplicatedUsing = OnRep_State)
	int ServerState = 0;

	UPROPERTY()
	int LocalState = -999;

	UFUNCTION()
	void OnRep_State();

	void ServerPerform();

};
