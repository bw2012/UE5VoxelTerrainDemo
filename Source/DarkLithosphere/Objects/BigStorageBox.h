

#pragma once

#include "CoreMinimal.h"
#include "StorageBox.h"
#include "BigStorageBox.generated.h"

/**
 * 
 */
UCLASS()
class DARKLITHOSPHERE_API ABigStorageBox : public AStorageBox {
	GENERATED_BODY()
	
public:

	virtual FName GetContainerWidgetName();

};
