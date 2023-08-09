
#pragma once

#include "CoreMinimal.h"
#include "BaseObject.h"
#include "TechHelper.h"
#include "GameFramework/Actor.h"
#include "ElectricDevice.generated.h"


UENUM(BlueprintType)
enum class FElectricDeviceLinkType : uint8 {
	None = 0		UMETA(DisplayName = "None"),
	Generator = 1	UMETA(DisplayName = "Generator"),
	Transmitter = 2	UMETA(DisplayName = "Transmitter"),
	Endpoint = 3	UMETA(DisplayName = "Endpoint"),
	Target = 4		UMETA(DisplayName = "Target"),
};


UCLASS()
class DARKLITHOSPHERE_API AElectricDevice : public ABaseObject {
	GENERATED_BODY()
	
public:	

	AElectricDevice();

public:	

	UPROPERTY(EditAnywhere, Category = "Sandbox")
	int EffectId;

	UPROPERTY(EditAnywhere, Category = "Sandbox")
	FElectricDeviceLinkType LinkType;

	UPROPERTY(EditAnywhere, Category = "Sandbox")
	float ElectricLinkDistance;

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

	virtual int GetElectricDeviceType();

	int GetElectricDeviceServerState();

	void SetElectricDeviceServerState(int NewState);

protected:

	void SetFlagActive(int FlagActive);

	UPROPERTY()
	ATechHelper* TechHelper;

	UPROPERTY(ReplicatedUsing = OnRep_State)
	int ServerState = 0;

	UPROPERTY(ReplicatedUsing = OnRep_FlagActive)
	int ServerFlagActive = 0;

	UPROPERTY()
	int LocalState = -999;

	UFUNCTION()
	void OnRep_State();

	UFUNCTION()
	void OnRep_FlagActive();

	virtual void OnHandleState();

};
