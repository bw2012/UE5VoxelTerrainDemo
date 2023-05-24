
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

	void SetMessageString(FString Title, FString Str);

	UFUNCTION(BlueprintCallable, Category = "Sandbox")
	FString GetMessageString();

	UFUNCTION(BlueprintCallable, Category = "Sandbox")
	FString GetMessageTitle();

	bool IsFatalMessage();

private:

	FString BkgProgressString;

	FString ProgressString;

	FString MessageTitle;

	FString MessageString;

	float Progress;

	bool bFatalMessage = false;

};
