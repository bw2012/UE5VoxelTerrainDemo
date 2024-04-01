// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Runtime/UMG/Public/UMG.h"
#include "Runtime/UMG/Public/UMGStyle.h"
#include "Runtime/UMG/Public/Slate/SObjectWidget.h"
#include "Runtime/UMG/Public/IUMGModule.h"
#include "Runtime/UMG/Public/Blueprint/UserWidget.h"


#include "Blueprint/UserWidget.h"
#include "HoverObjectWidget.generated.h"

/**
 * 
 */
UCLASS()
class DARKLITHOSPHERE_API UHoverObjectWidget : public UUserWidget {
	GENERATED_BODY()
	
protected:
	
	UFUNCTION(BlueprintCallable, Category = "Sandbox widget")
	FString GetSandboxHoverObjectText();

	UFUNCTION(BlueprintCallable, Category = "Sandbox widget")
	FString GetSandboxHoverObjectTextExt1();

	UFUNCTION(BlueprintCallable, Category = "Sandbox widget")
	FString GetSandboxHoverObjectCategory();

	UFUNCTION(BlueprintCallable, Category = "Sandbox widget")
	FString GetSandboxHoverObjectDescription();


private:

	
};
