#pragma once

#include "CoreMinimal.h"
#include "BaseObject.h"
#include "ConstructionObject.generated.h"


UENUM(BlueprintType)
enum class EConstructionType : uint8 {
	Cube = 0			UMETA(DisplayName = "Cube"),
	Foundation = 1		UMETA(DisplayName = "Foundation"),
	Wall = 2			UMETA(DisplayName = "Wall"),
	Ceiling = 3			UMETA(DisplayName = "Ceiling"),
	Ramp = 4			UMETA(DisplayName = "Ramp"),
	Door = 5			UMETA(DisplayName = "Door"),
	UpDownStairs = 6	UMETA(DisplayName = "Up/Down Stairs"),
};


/**
 * 
 */
UCLASS()
class DARKLITHOSPHERE_API AConstructionObject : public ABaseObject {
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, Category = "Sandbox")
	EConstructionType ConstructionType;
	
	virtual bool PlaceToWorldClcPosition(const UWorld* World, const FVector& SourcePos, const FRotator& SourceRotation, const FHitResult& Res, FVector& Location, FRotator& Rotation, bool bFinal) const override;

	virtual bool CanTake(const AActor* Actor = nullptr) const;

	virtual void OnTerrainChange() override;

	virtual void OnPlaceToWorld() override;
	
};
