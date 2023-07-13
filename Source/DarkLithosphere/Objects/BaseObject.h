#pragma once

#include "CoreMinimal.h"
#include "SandboxObject.h"
#include "BaseObject.generated.h"


#define SandboxType_Tool		100
#define SandboxType_Door		200
#define SandboxType_Equipment	500

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
