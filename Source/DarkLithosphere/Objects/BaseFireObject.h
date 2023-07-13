

#pragma once

#include "CoreMinimal.h"
#include "BaseObject.h"
#include "BaseFireObject.generated.h"

/**
 * 
 */
UCLASS()
class DARKLITHOSPHERE_API ABaseFireObject : public ABaseObject {
	GENERATED_BODY()


public:
	ABaseFireObject();
	
public:

	UPROPERTY(EditAnywhere, Category = "Sandbox")
	float MaxLifetime;

	virtual void Tick(float DeltaTime) override;

	virtual bool CanTake(const AActor* Actor = nullptr) const override;

	virtual void PostLoadProperties() override;

	virtual void OnTerrainChange() override;

protected:

	virtual void BeginPlay() override;

	virtual const TArray<FString>& GetFlameComponentsName() const;

private:

	void SetFlameVisibility(bool Visibility);

	void SetFlameScale(FString Name, float Scale);

	void SetAllFlameScale(float Scale);

	double Timestamp;

	float InitialIntensity;

	UPROPERTY(Replicated)
	float Lifetime;

	UPROPERTY(ReplicatedUsing = OnRep_State)
	int State = 0;

	UFUNCTION()
	void OnRep_State();

	void SetBurnt();

};
