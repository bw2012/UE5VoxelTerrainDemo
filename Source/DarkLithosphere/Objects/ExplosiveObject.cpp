#include "ExplosiveObject.h"
#include "../TerrainController.h"
#include "../LevelController.h"
#include "Net/UnrealNetwork.h"




AExplosiveObject::AExplosiveObject() {
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	ServerState = EXPLOSIVE_OBJ_STATE_INACTIVE;
}

void AExplosiveObject::BeginPlay() {
	Super::BeginPlay();
	ServerState = EXPLOSIVE_OBJ_STATE_INACTIVE;
}

bool AExplosiveObject::CanTake(const AActor* Actor) const {
	return ServerState == 0;
}

void AExplosiveObject::PostLoadProperties() {
	const auto& Param = GetProperty(TEXT("Lifetime"));
	if (Param != "") {
		//Lifetime = FCString::Atof(*Param);
	}
}

void AExplosiveObject::Tick(float DeltaTime) {
	Super::Tick(DeltaTime);

	if (GetNetMode() == NM_Client) {
		return;
	}

	ServerPerform();
}

void AExplosiveObject::OnRep_State() {
	if (ServerState != LocalState) {

		if (ServerState > 0) {
			//SetActive();
		} else {
			//SetInactive();
		}

		LocalState = ServerState;
	}
}

void AExplosiveObject::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const {
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AExplosiveObject, ServerState);
}

void AExplosiveObject::ServerPerform() {
	
}

bool AExplosiveObject::IsZoneAnchor() {
	return true;
}

bool AExplosiveObject::IsInteractive(const APawn* Source) {
	return true;
}

void AExplosiveObject::MainInteraction(const APawn* Source) {
	ALevelController* Lvl = Cast<ALevelController>(ASandboxLevelController::GetInstance());
	if (Lvl && SandboxRootMesh) {
		ServerState = EXPLOSIVE_OBJ_STATE_ACTIVE;

		FVector Pos = SandboxRootMesh->GetSocketLocation(TEXT("Pos1"));
		ASandboxEffect* Effect = Lvl->SpawnEffect(8, FTransform(Pos));

		Effect->AttachToComponent(SandboxRootMesh, FAttachmentTransformRules(EAttachmentRule::KeepWorld, true));

		EnablePhysics();
		FTimerHandle FuzeTimerHandle;
		GetWorld()->GetTimerManager().SetTimer(FuzeTimerHandle, this, &AExplosiveObject::PerformExplosion, 5, false);
	}
}

void AExplosiveObject::PerformActivation() {
	if (ServerState == EXPLOSIVE_OBJ_STATE_ACTIVE) {
		return;
	}

	ServerState = EXPLOSIVE_OBJ_STATE_ACTIVE;

	ALevelController* Lvl = Cast<ALevelController>(ASandboxLevelController::GetInstance());
	if (Lvl && SandboxRootMesh) {
		FVector Pos = SandboxRootMesh->GetSocketLocation(TEXT("Pos1"));
		ASandboxEffect* Effect = Lvl->SpawnEffect(8, FTransform(Pos));

		Effect->AttachToComponent(SandboxRootMesh, FAttachmentTransformRules(EAttachmentRule::KeepWorld, true));

		EnablePhysics();
		FTimerHandle FuzeTimerHandle;
		GetWorld()->GetTimerManager().SetTimer(FuzeTimerHandle, this, &AExplosiveObject::PerformExplosion, 5, false);
	}
}

int AExplosiveObject::GetExplosiveState() {
	return ServerState;
}

float AExplosiveObject::TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser) {
	float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
	UE_LOG(LogTemp, Warning, TEXT("TakeDamage: %s %f"), *GetName(), ActualDamage);

	USandboxDamageType* SandboxDamageType = Cast<USandboxDamageType>(DamageEvent.DamageTypeClass->GetDefaultObject());

	if (SandboxDamageType) {
		float ExplosionDamage = ActualDamage * SandboxDamageType->ExplosionDamageFactor;
		if (ExplosionDamage > 45) {
			PerformExplosion();
		} else {
			float FireDamage = ActualDamage * SandboxDamageType->FireDamageFactor;
			if (ExplosionDamage > 10) {
				PerformActivation();
			}
		}
	}

	return DamageAmount;
}

float AExplosiveObject::GetDamageRadius() {

	URadialForceComponent* ForceCmp = GetFirstComponentByName<URadialForceComponent>(TEXT("RadialForce"));
	if (ForceCmp) {
		return ForceCmp->Radius;
	}

	return 0.f;
}

//server only
void AExplosiveObject::PerformExplosion() {
	if (GetNetMode() == NM_Client) {
		return;
	}

	if (ServerState == EXPLOSIVE_OBJ_STATE_PERFORM) {
		return;
	}

	ALevelController* Lvl = Cast<ALevelController>(ASandboxLevelController::GetInstance());
	if (Lvl) {
		ServerState = EXPLOSIVE_OBJ_STATE_PERFORM;
		Lvl->SpawnEffect(7, FTransform(GetActorLocation()));

		URadialForceComponent* ForceCmp = GetFirstComponentByName<URadialForceComponent>(TEXT("RadialForce"));
		if (ForceCmp) {

			const float Radius = ForceCmp->Radius;

			TArray<struct FOverlapResult> Result;
			FCollisionQueryParams CollisionQueryParams = FCollisionQueryParams::DefaultQueryParam;
			CollisionQueryParams.bTraceComplex = false;

			const double Start = FPlatformTime::Seconds();
			bool bIsOverlap = GetWorld()->OverlapMultiByChannel(Result, GetActorLocation(), FQuat(), ECC_Visibility, FCollisionShape::MakeSphere(Radius)); 
			const double End = FPlatformTime::Seconds();
			const double Time = (End - Start) * 1000;
			//UE_LOG(LogVt, Log, TEXT("Trace terrain meshes: %d %d %d -> %f ms"), BaseZoneIndex.X, BaseZoneIndex.Y, BaseZoneIndex.Z, Time);

			UPROPERTY(EditAnywhere, Category = "Sandbox")
			float TerrainRadius = 300.f;
			float DamageRadius = Radius;
			float DamageAmount = BaseDamage;

			ATerrainController* MainTerrain = nullptr;

			if (bIsOverlap) {
				for (FOverlapResult& Overlap : Result) {
					if (Overlap.GetActor() == this) {
						UE_LOG(LogTemp, Warning, TEXT("skip self %s"), *GetName());
						continue;
					}

					ATerrainController* Terrain = Cast<ATerrainController>(Overlap.GetActor());
					if (!MainTerrain&& Terrain) {
						MainTerrain = Terrain;
					}

					AExplosiveObject* Explosive = Cast<AExplosiveObject>(Overlap.GetActor());
					if (Explosive) {
						if (FVector::Distance(GetActorLocation(), Explosive->GetActorLocation()) < 300) {
							DamageRadius += Explosive->GetDamageRadius() / 2;
							DamageAmount += Explosive->BaseDamage / 2;
							TerrainRadius += Explosive->TerrainDamageRadius / 4;

							Lvl->RemoveSandboxObject(Explosive);

							UE_LOG(LogTemp, Warning, TEXT("RemoveSandboxObject %s"), *Explosive->GetName());
						}
					}
				}
			}

			if (DamageRadius > 5000) {
				DamageRadius = 5000;
			}

			if (TerrainRadius > 1800) {
				TerrainRadius = 1800;
			}

			UE_LOG(LogTemp, Warning, TEXT("%s -> DamageAmount: %f, DamageRadius: %f, TerrainRadius: %f"), *GetName(), DamageAmount, DamageRadius, TerrainRadius);

			TArray<AActor*> IgnoreActors;
			IgnoreActors.Add(this);

			UGameplayStatics::ApplyRadialDamage(GetWorld(), DamageAmount, GetActorLocation(), DamageRadius, DamageType, IgnoreActors, this, nullptr, false, ECollisionChannel::ECC_Visibility);

			if (MainTerrain) {
				MainTerrain->DigTerrainRoundHole(GetActorLocation(), TerrainRadius);
			}

			ForceCmp->FireImpulse();
		}

		Lvl->RemoveSandboxObject(this);
	}

	


}