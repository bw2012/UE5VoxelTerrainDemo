
#pragma once

#include "Runtime/UMG/Public/UMG.h"
#include "Runtime/UMG/Public/UMGStyle.h"
#include "Runtime/UMG/Public/Slate/SObjectWidget.h"
#include "Runtime/UMG/Public/IUMGModule.h"
#include "Runtime/UMG/Public/Blueprint/UserWidget.h"

#include "Blueprint/UserWidget.h"
#include "CraftItemWidget.generated.h"

/**
 * 
 */
UCLASS()
class DARKLITHOSPHERE_API UCraftRecipeItemWidget : public UUserWidget {
	GENERATED_BODY()
	
protected:

	UFUNCTION(BlueprintCallable, Category = "Sandbox")
	UTexture2D* GetCraftReceipeIcon(int ReceipeIndex);

	UFUNCTION(BlueprintCallable, Category = "Sandbox")
	FString GetCraftReceipeName(int ReceipeIndex);

	UFUNCTION(BlueprintCallable, Category = "Sandbox")
	void SetPlayerControllerExtMode(int Index);

private:

	
};
