
#include "EnvironmentController.h"


void AEnvironmentController::Tick(float DeltaTime) {
	Super::Tick(DeltaTime);


	TSandboxGameTime Time = ClcGameTimeOfDay();


	if (bIsNightPrev != IsNight()) {
		if (IsNight()) {
			GEngine->AddOnScreenDebugMessage(-1, 10, FColor::White, TEXT("Night"));
			//Night1AmbientSound->GetAudioComponent()->Play();
		} else {
			GEngine->AddOnScreenDebugMessage(-1, 10, FColor::White, TEXT("Day"));
			//Night1AmbientSound->GetAudioComponent()->FadeOut(2, 0.1);
		}
	}

	bIsNightPrev = IsNight();

}
