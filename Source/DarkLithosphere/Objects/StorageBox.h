

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

};
