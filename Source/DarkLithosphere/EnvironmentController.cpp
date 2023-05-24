
#include "EnvironmentController.h"
#include "TerrainController.h"


void AEnvironmentController::BeginPlay() {
	Super::BeginPlay();


}

void AEnvironmentController::Tick(float DeltaTime) {
	Super::Tick(DeltaTime);

	TSandboxGameTime Time = ClcGameTimeOfDay();

	if (bIsNightPrev != IsNight()) {
		if (IsNight()) {
			GEngine->AddOnScreenDebugMessage(-1, 10, FColor::White, TEXT("Night"));

			if (Night1AmbientSound) {
				Night1AmbientSound->GetAudioComponent()->FadeIn(5);
			}

			if (TerrainController) {
				/*
				TArray<FVector> R = TerrainController->Test(FVector(0), 12000.f);

				if (R.Num() > 0) {
					int MaxNum = FMath::RandRange(1, 5);
					for (int I = 0; I < MaxNum; I++) {
						int Idx = FMath::RandRange(0, R.Num());
						//UE_LOG(LogTemp, Log, TEXT("TEST -> %d "), Idx);
						FVector ZonePos = R[Idx];
						//DrawDebugBox(GetWorld(), ZonePos, FVector(USBT_ZONE_SIZE / 2), FColor(255, 255, 255, 0), true);

						if (TestActor) {
							FVector Pos(ZonePos);
							Pos.Z += 1000;
							FRotator Rotation(0, 0, 0);
							FTransform Transform(Rotation, Pos, FVector(1));

							UClass* SpawnClass = TestActor->ClassDefaultObject->GetClass();
							AActor* NewActor = (ASandboxObject*)GetWorld()->SpawnActor(SpawnClass, &Transform);
						}
					}
				}
				*/
			}

		} else {
			GEngine->AddOnScreenDebugMessage(-1, 10, FColor::White, TEXT("Day"));

			if (Night1AmbientSound) {
				Night1AmbientSound->GetAudioComponent()->FadeOut(2, 0.1);
			}

		}
	}

	bIsNightPrev = IsNight();

}

float AEnvironmentController::ClcHeightFactor() const {
	if (HeightCurve) {
		float HeightFactor = HeightCurve->GetFloatValue(PlayerZLevel);
		//UE_LOG(LogTemp, Log, TEXT("HeightFactor -> %f %f"), HeightFactor, PlayerZLevel);
		return HeightFactor;
	}

	return 1.f;
}

void AEnvironmentController::UpdatePlayerPosition(FVector Pos, APlayerController* Controller) {
	Super::UpdatePlayerPosition(Pos, Controller);

	if (TerrainController) {
		const float Level = TerrainController->GetGroundLevel(Pos);
		const float P = Pos.Z - Level;
		PlayerZLevel = (Pos.Z - Level) / 1000;
		//UE_LOG(LogTemp, Log, TEXT("TEST -> %f"), P);

		if (AmbientSound) {
			float Value = 0;
			if (P < -1000) {
				Value = P / -3000;
				if (Value > 1) {
					Value = 1;
				}
			}
			AmbientSound->GetAudioComponent()->SetFloatParameter(TEXT("Z"), Value);
		}

		if (PlayerZLevel < -2.5) { 
			if (!IsCaveMode()) {
				// bad UE5 GlobalDistanceField performance workaround
				Controller->ConsoleCommand("r.AOGlobalDistanceField 0", true); 
			}

			SetCaveMode(true);
		} else {
			if (IsCaveMode()) {
				Controller->ConsoleCommand("r.AOGlobalDistanceField 1", true);
			}

			SetCaveMode(false);
		}
	}

	if (HeightCurve) {
		//HeightFactor = HeightCurve->GetFloatValue(ZZ);
		//UE_LOG(LogTemp, Log, TEXT("HeightFactor -> %f %f %f"), Zc, ZZ, HeightFactor);
	}

}