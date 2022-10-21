
#include "CubeObject.h"
//#include "SpawnHelper.h"


ACubeObject::ACubeObject() {
	PrimaryActorTick.bCanEverTick = false;

	//TODO
	auto SoundObj = ConstructorHelpers::FObjectFinder<USoundCue>(TEXT("/Game/Sandbox/Object/Cube/A_StoneCubeImpact2_Cue"));
	if (SoundObj.Object != nullptr) {
		Sound = SoundObj.Object;
	}
}

void ACubeObject::OnPlaceToWorld() {
	if (Sound) {
		FVector Location = GetActorLocation();
		// UE_LOG(LogTemp, Warning, TEXT("%f %f %f"), Location.X, Location.Y, Location.Z);
		UGameplayStatics::PlaySoundAtLocation(this, Sound, GetActorLocation(), FRotator::ZeroRotator, 1, 1, 0.0f, nullptr, nullptr);
	}
}


bool ACubeObject::PlaceToWorldClcPosition(const UWorld* World, const FVector& SourcePos, const FRotator& SourceRotation, const FHitResult& Res, FVector& Location, FRotator& Rotation, bool bFinal) const {
	Location = Res.Location;

	/*
	ASandboxObject* TargetObject = Cast<ASandboxObject>(Res.Actor);
	if (TargetObject) {
		if (TargetObject->GetSandboxClassId() == GetSandboxClassId()) {
			FVector TargetActorPos = Res.Actor->GetActorLocation();
			FRotator TargetActorRotator = Res.Actor->GetActorRotation();

			Location = TargetActorPos;
			Rotation = TargetActorRotator;

			if (bFinal) {

				UE_LOG(LogTemp, Warning, TEXT("1 - %f %f %f"), TargetActorRotator.Pitch, TargetActorRotator.Yaw, TargetActorRotator.Roll);


				FRotator Ext = SelectRotation();

				UE_LOG(LogTemp, Warning, TEXT("2 - %f %f %f"), Ext.Pitch, Ext.Yaw, Ext.Roll);
				Rotation += Ext;
			}

			FVector Normal = Res.ImpactNormal;
			Location = Location + (Normal * 60);
			return true;
		}
	}

	FVector Normal = Res.ImpactNormal;
	Location = Location + (Normal * 30);

	ABaseCharacter* BaseCharacter = Cast<ABaseCharacter>(Pawn);
	if (BaseCharacter) {
		CalculateTargetRotation(BaseCharacter, Rotation);
	}

	if (bFinal) {
		Rotation += SelectRotation();
	}
	*/

	return true;
	
}