
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

	UPROPERTY(EditAnywhere, Category = "Sandbox")
	USoundMix* MutedSoundMix;

	UPROPERTY(EditAnywhere, Category = "Sandbox")
	USoundMix* UnmutedSoundMix;

	void Init() override;

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

	void HandleNetworkFailure(UWorld* World, UNetDriver* NetDriver, ENetworkFailure::Type FailureType, const FString& ErrorString);

	void Mute();

	void Unmute();

private:

	FString BkgProgressString;

	FString ProgressString;

	FString MessageTitle;

	FString MessageString;

	float Progress;

	bool bFatalMessage = false;

};
