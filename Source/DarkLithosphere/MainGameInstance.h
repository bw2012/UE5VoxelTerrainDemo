
#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "MainGameInstance.generated.h"

/**
 * 
 */
UCLASS()
class DARKLITHOSPHERE_API UMainGameInstance : public UGameInstance {
	GENERATED_BODY()
	

public:

	UFUNCTION(BlueprintCallable, Category = "Sandbox")
	FString GetBkgProgressString();

	void SetBkgProgressString(FString Str);

	UFUNCTION(BlueprintCallable, Category = "Sandbox")
	FString GetProgressString();

	void SetProgressString(FString Str);

	UFUNCTION(BlueprintCallable, Category = "Sandbox")
	float GetProgress();

	void SetProgress(float Val);

private:

	FString BkgProgressString;

	FString ProgressString;

	float Progress;

};
