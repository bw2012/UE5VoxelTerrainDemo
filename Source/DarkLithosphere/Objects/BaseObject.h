#pragma once

#include "CoreMinimal.h"
#include "SandboxObject.h"
#include "BaseObject.generated.h"

/**
 * 
 */
UCLASS()
class DARKLITHOSPHERE_API ABaseObject : public ASandboxObject
{
	GENERATED_BODY()
	

public:

	virtual bool VisibleInHand(FTransform& Transform);

	virtual bool IsContainer();

	virtual FName GetContainerWidgetName();

};
