
#pragma once

#include "Runtime/UMG/Public/UMG.h"
#include "Runtime/UMG/Public/UMGStyle.h"
#include "Runtime/UMG/Public/Slate/SObjectWidget.h"
#include "Runtime/UMG/Public/IUMGModule.h"
#include "Runtime/UMG/Public/Blueprint/UserWidget.h"

#include "Blueprint/UserWidget.h"
#include "CraftTableWidget.generated.h"

/**
 *
 */
UCLASS()
class DARKLITHOSPHERE_API UCraftTableWidget : public UUserWidget {
	GENERATED_BODY()

protected:

	UFUNCTION(BlueprintCallable, Category = "SandboxInventory")
	void SetNextCraftPage(int32 Page = 1);

private:


};
