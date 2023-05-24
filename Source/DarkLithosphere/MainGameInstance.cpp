
#include "MainGameInstance.h"

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
