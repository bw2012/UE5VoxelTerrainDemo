
#include "MainGameInstance.h"
#include "Kismet/GameplayStatics.h" // UGameplayStatics


FString UMainGameInstance::GetBkgProgressString() {
	return BkgProgressString;
}

void UMainGameInstance::SetBkgProgressString(FString Str) {
	BkgProgressString = Str;
}

FString UMainGameInstance::GetProgressString() {
	return ProgressString;
}

void UMainGameInstance::SetProgressString(FString Str) {
	ProgressString = Str;
}

float UMainGameInstance::GetProgress() {
	return Progress;
}

void UMainGameInstance::SetProgress(float Val) {
	Progress = Val;
}

void UMainGameInstance::SetMessageString(FString Title, FString Str) {
	MessageString = Str;
	MessageTitle = Title;
	bFatalMessage = true;
}

FString UMainGameInstance::GetMessageString() {
	return MessageString;
}

bool UMainGameInstance::IsFatalMessage() {
	return bFatalMessage;
}

FString UMainGameInstance::GetMessageTitle() {
	return MessageTitle;
}

void UMainGameInstance::Init() {
	Super::Init();

	GetEngine()->OnNetworkFailure().AddUObject(this, &UMainGameInstance::HandleNetworkFailure);
}

void UMainGameInstance::HandleNetworkFailure(UWorld* World, UNetDriver* NetDriver, ENetworkFailure::Type FailureType, const FString& ErrorString) {

	if (FailureType == ENetworkFailure::OutdatedClient) {
		SetMessageString(TEXT("Network failure:"), TEXT("Outdated client"));
	}

}

void UMainGameInstance::Mute() {
	if (MutedSoundMix) {
		UGameplayStatics::SetBaseSoundMix(this, MutedSoundMix);
	}
}

void UMainGameInstance::Unmute() {
	if (UnmutedSoundMix) {
		UGameplayStatics::SetBaseSoundMix(this, UnmutedSoundMix);
	}
}

