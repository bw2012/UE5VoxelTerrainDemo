

#pragma once

#include "CoreMinimal.h"
#include "SandboxObject.h"
#include "BaseObject.h"
#include "StorageBox.generated.h"

/**
 * 
 */
UCLASS()
class DARKLITHOSPHERE_API AStorageBox : public ABaseObject {

	GENERATED_BODY()

public:

	virtual bool IsContainer() override;

	virtual bool PlaceToWorldClcPosition(const UWorld* World, const FVector& SourcePos, const FRotator& SourceRotation, const FHitResult& Res, FVector& Location, FRotator& Rotation, bool bFinal) const override;

};
