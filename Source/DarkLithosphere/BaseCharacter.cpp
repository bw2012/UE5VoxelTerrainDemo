// Fill out your copyright notice in the Description page of Project Settings.

#include "BaseCharacter.h"
#include "Objects/BaseObject.h"
#include "MainPlayerController.h"
#include "TerrainController.h"
#include "LevelController.h"
#include "VitalSystemComponent.h"


ABaseCharacter::ABaseCharacter(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
	PrimaryActorTick.bCanEverTick = true;

	CursorMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CursorMesh"));
	CursorMesh->SetRelativeLocation(FVector(400.0f, 0.f, 0.f));
	CursorMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	CursorMesh->SetCollisionProfileName(TEXT("NoCollision"));
	CursorMesh->SetCanEverAffectNavigation(false);
	CursorMesh->SetVisibleFlag(false);
	CursorMesh->SetupAttachment(GetCapsuleComponent());
	CursorMesh->SetWorldRotation(FRotator());

	CursorToWorld = CreateDefaultSubobject<UDecalComponent>("CursorToWorld");
	CursorToWorld->SetupAttachment(RootComponent);
	static ConstructorHelpers::FObjectFinder<UMaterial> DecalMaterialAsset(TEXT("Material'/Game/ThirdParty/TopDownCPP/Blueprints/M_Cursor_Decal.M_Cursor_Decal'"));
	if (DecalMaterialAsset.Succeeded()) {
		CursorToWorld->SetDecalMaterial(DecalMaterialAsset.Object);
	}

	CursorToWorld->DecalSize = FVector(5.f, 20.f, 20.f);
	CursorToWorld->SetRelativeRotation(FRotator(90.0f, 0.0f, 0.0f).Quaternion());
	CursorToWorld->SetVisibility(false);
}

// Called when the game starts or when spawned
void ABaseCharacter::BeginPlay() {
	Super::BeginPlay();

	UCharacterMovementComponent* MovCmp = Cast<UCharacterMovementComponent>(GetMovementComponent());
	if (MovCmp) {
		//MovCmp->bServerAcceptClientAuthoritativePosition = true;
		MovCmp->bIgnoreClientMovementErrorChecksAndCorrection = true;
	}

	bFirstRun = true;

	MakeModularSkList();

	SelectActiveInventorySlot(0);

	UShapeComponent* RightHandCollisionComponent = GetFirstComponentByName<UShapeComponent>(TEXT("RightHandCollision"));
	if (RightHandCollisionComponent) {
		//RightHandCollisionComponent->OnComponentBeginOverlap.AddDynamic(this, &ABaseCharacter::BeginOverlap);
	}

	GrassTraceLastPos = GetActorLocation();

	USkeletalMeshComponent* MeshComponent = GetMesh();
	UAnimInstance* AnimInstance = MeshComponent->GetAnimInstance();
	AnimInstance->OnPlayMontageNotifyBegin.AddDynamic(this, &ABaseCharacter::OnNotifyBeginReceived);
	AnimInstance->OnPlayMontageNotifyEnd.AddDynamic(this, &ABaseCharacter::OnNotifyEndReceived);

}

void ConservateContainer(const UContainerComponent* Container, TArray<FTempContainerStack>& ConservedContainer) {
	if (Container) {
		int SlotId = 0;
		for (const auto& Stack : Container->Content) {
			if (Stack.Amount > 0) {
				ConservedContainer.Add(FTempContainerStack{SlotId, FContainerStack{Stack.Amount, Stack.SandboxClassId} });
			}
			SlotId++;
		}
	}
}

void ABaseCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason) {
	Super::EndPlay(EndPlayReason);

	if (GetNetMode() == NM_DedicatedServer || GetNetMode() == NM_ListenServer) {
		for (TActorIterator<ALevelController> ActorItr(GetWorld()); ActorItr; ++ActorItr) {
			ALevelController* Lc = Cast<ALevelController>(*ActorItr);
			if (Lc) {

				FCharacterLoadInfo Conserved;
				Conserved.TypeId = SandboxTypeId;
				Conserved.SandboxPlayerUid = SandboxPlayerUid;
				Conserved.Location = GetActorLocation();
				Conserved.Rotation = GetActorRotation();

				ConservateContainer(GetContainer("Inventory"), Conserved.Inventory);
				ConservateContainer(GetContainer("Equipment"), Conserved.Equipment);

				UE_LOG(LogTemp, Warning, TEXT("Server: character conservation: %s"), *Conserved.SandboxPlayerUid);

				if (Lc) {
					Lc->CharacterConservation(Conserved);
				}

				break;
			}
		}

	}
}

/*
void ABaseCharacter::BeginOverlap(UPrimitiveComponent* OverlappedComponent,	AActor* OtherActor,	UPrimitiveComponent* OtherComp,	int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult) {
	if (OtherActor) {
		UE_LOG(LogTemp, Warning, TEXT("test1: %s"), *OtherActor->GetName());
		UE_LOG(LogTemp, Warning, TEXT("test2: %s"), *OtherComp->GetName());
		UE_LOG(LogTemp, Warning, TEXT("test3: %d"), OtherBodyIndex);

		if (bFromSweep) {

			UE_LOG(LogTemp, Warning, TEXT("test4:"));
		}

		//OtherActor->GetName();
	}
}
*/


void ABaseCharacter::Tick(float DeltaTime) {
	Super::Tick(DeltaTime);

	// OnPossess not works on client. workaround
	if (GetNetMode() == NM_Client) {
		if (bFirstRun) {
			RebuildEquipment();
			bFirstRun = false;
		}
	}

	double T = FPlatformTime::Seconds();
	if (IdleSound) {
		double Delta = T - LastIdleSound;
		if (Delta > 3) {
			PlaySoundCue(IdleSound);
			LastIdleSound = T;
		}
	}

	FVector Pos = GetActorLocation();

	float Distance = FVector::Distance(GetActorLocation(), GrassTraceLastPos);
	if (Distance > 25) {
		static const float D = 5;
		FVector Location = GetActorLocation();
		Location.Z = Location.Z - GetCapsuleComponent()->GetScaledCapsuleHalfHeight() + D;
		GrassTraceLastPos = GetActorLocation();

		//DrawDebugPoint(GetWorld(), Location, 5, FColor(255, 0, 0, 255), false, 5);

		/*
		FVector TestPoint(Location);
		TestPoint.Z -= D;
		TArray<struct FHitResult> OutHits;
		bool bIsOverlap = GetWorld()->SweepMultiByChannel(OutHits, Location, TestPoint, FQuat(), ECC_Visibility, FCollisionShape::MakeSphere(20)); // ECC_Visibility
		if (bIsOverlap) {
			for (const FHitResult& Overlap : OutHits) {
				ATerrainController* Terrian = Cast<ATerrainController>(Overlap.GetActor());
				if (Terrian) {
					Terrian->MakeFlattenGrassTrace(Overlap);
				}
			}
		}
		*/
	}

	TArray<UContainerComponent*> Components;
	GetComponents<UContainerComponent>(Components);
	for (UContainerComponent* Container : Components) {
		if (Container->IsUpdated()) {
			OnContainerUpdate(Container);
			Container->ResetUpdatedFlag();
		}
	}
}

void ABaseCharacter::OnContainerUpdate(UContainerComponent* Container) {
	FString Name = Container->GetName();
	if (Name == TEXT("Equipment")) {
		RebuildEquipment();
	}
}

int ABaseCharacter::GetTypeId() {
	return SandboxTypeId;
}

bool ABaseCharacter::IsDead() {
	return State < 0;
}

UContainerComponent* ABaseCharacter::GetContainer(FString Name) {
	TArray<UContainerComponent*> Components;
	GetComponents<UContainerComponent>(Components);
	for (UContainerComponent* Container : Components) {
		if (Container->GetName() == Name) {
			return Container;
		}
	}

	return nullptr;
}

/*
void ABaseCharacter::OnDeath() {
	//GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetMovementComponent()->StopMovementImmediately();
	GetMesh()->SetAnimationMode(EAnimationMode::AnimationSingleNode);
	GetMesh()->PlayAnimation(DeathAnim, false);
	GetMesh()->SetPlayRate(1.f);

	//AMainPlayerController* C = Cast<AMainPlayerController>(GetController());
	//if (C) {
	//	C->OnDeath();
	//}
}
*/

void ABaseCharacter::PerformMainAttack() {
	if (MainAttackMontage) {
		if (!bIsAttacking) {
			UAnimMontage* AttackMontage = MainAttackMontage;
			if (!bIsEmptyHand) {

				if (TwoHandMeeleAttackMontage) {
					AttackMontage = TwoHandMeeleAttackMontage;
				} else {
					return;
				}
			}

			bIsAttacking = true;

			USkeletalMeshComponent* MeshComponent = GetMesh();
			if (MeshComponent) {
				UAnimInstance* AnimInstance = MeshComponent->GetAnimInstance();
				const float MontageLength = AnimInstance->Montage_Play(AttackMontage, 1.0f, EMontagePlayReturnType::MontageLength, 0.f);
				bool bPlayedSuccessfully = (MontageLength > 0.f);
				if (bPlayedSuccessfully) {

					FOnMontageEnded MontageEndedDelegate;
					MontageEndedDelegate.BindUObject(this, &ABaseCharacter::OnMontageEnded);
					AnimInstance->Montage_SetEndDelegate(MontageEndedDelegate, AttackMontage);
				}
			}
		}
	}
}

void ABaseCharacter::OnNotifyBeginReceived(FName NotifyName, const FBranchingPointNotifyPayload& BranchingPointNotifyPayload) {
	UE_LOG(LogTemp, Warning, TEXT("OnNotifyBeginReceived -> %s"), *NotifyName.ToString());

	if (MeeleAttackSound) {
		UGameplayStatics::SpawnSoundAtLocation(this, MeeleAttackSound, GetActorLocation(), FRotator::ZeroRotator, 1, 1, 0.0f, nullptr, nullptr, true);
	}
}


void ABaseCharacter::OnNotifyEndReceived(FName NotifyName, const FBranchingPointNotifyPayload& BranchingPointNotifyPayload) {

}

void ABaseCharacter::OnMontageEnded(UAnimMontage* Montage, bool bInterrupted) {
	bIsAttacking = false;
}

void ABaseCharacter::OnFinishPlayMainAttack() {
	bIsAttacking = false;
}

bool ABaseCharacter::CanRotateCamera() {
	AMainPlayerController* MainPlayerController = Cast<AMainPlayerController>(GetController());
	if (MainPlayerController->IsGuiMode()) {
		return false;
	}

	if (MainPlayerController->IsGameInputBlocked()) {
		return false;
	}

	return true;
}

bool ABaseCharacter::CanMove() {
	AMainPlayerController* MainPlayerController = Cast<AMainPlayerController>(GetController());
	if (MainPlayerController->IsGuiMode()) {
		return false;
	}

	if (MainPlayerController->IsGameInputBlocked()) {
		return false;
	}

	if (IsDead()) {
		return false;
	}

	//if (bIsAttacking) {
	//	return false;
	//}

	//return Super::CanMove();

	return true;
}


void ABaseCharacter::SelectActiveInventorySlot(int SlotId) {
	if (SlotId >= 0) {
		UContainerComponent* MainInventory = nullptr;
		TArray<UContainerComponent*> Components;
		GetComponents<UContainerComponent>(Components);

		for (UContainerComponent* Container : Components) {
			// always use only first container
			if (Container->GetName() == TEXT("Inventory")) {
				MainInventory = Container;
				break;
			}
		}

		/*
		ABaseObject* Obj = nullptr;
		if (MainInventory) {
			FContainerStack* Stack = MainInventory->GetSlot(SlotId);
			if (Stack != nullptr) {
				if (Stack->Amount > 0) {
					const ASandboxObject* Obj2 = Stack->GetObject();
					if (Obj2 != nullptr) {
						Obj = Obj2;
					}
				}
			}
		}

		if (Obj) {
			FTransform Transform;
			bool bInHand = Obj->VisibleInHand(Transform);
			bIsEmptyHand = !bInHand;
			UStaticMesh* MeshInHand = nullptr;
			if (bInHand) {
				MeshInHand = Obj->SandboxRootMesh->GetStaticMesh();
			}

			UStaticMeshComponent* StaticMeshComponent = GetFirstComponentByName<UStaticMeshComponent>(TEXT("RightHandStaticMesh"));
			if (StaticMeshComponent) {
				StaticMeshComponent->SetStaticMesh(MeshInHand);
			}
		}*/
	} else {
		UStaticMeshComponent* StaticMeshComponent = GetFirstComponentByName<UStaticMeshComponent>(TEXT("RightHandStaticMesh"));
		if (StaticMeshComponent) {
			StaticMeshComponent->SetStaticMesh(nullptr);
			bIsEmptyHand = true;
		}
	}

	
}

bool ABaseCharacter::PlaySoundCue(USoundCue* SoundCue) {
	if (!bIsPlaySound) {
		UAudioComponent* AudioComponent = UGameplayStatics::SpawnSoundAtLocation(this, SoundCue, GetActorLocation(), FRotator::ZeroRotator, 1, 1, 0.0f, nullptr, nullptr, true);
		AudioComponent->OnAudioFinished.AddDynamic(this, &ABaseCharacter::OnFinishSound);
		bIsPlaySound = true;
		return true;
	}

	return false;
}

void ABaseCharacter::OnFinishSound() {
	bIsPlaySound = false;
}

//TODO to ASandboxCharacter
//==========================================================================================

void ABaseCharacter::RebuildMorphs() {
	USkeletalMeshComponent* CharacterMeshComponent = GetFirstComponentByName<USkeletalMeshComponent>(TEXT("CharacterMesh0"));
	if (CharacterMeshComponent) {
		CharacterMeshComponent->ClearMorphTargets();
	}

	for (const auto& Itm : DefaultMeshMorphs) {
		const FString& Name = Itm.Key;
		const float Val = Itm.Value;
		CharacterMeshComponent->SetMorphTarget(FName(*Name), Val);
	}

	//CharacterMeshComponent->SetMorphTarget("Fitness", 1.f);
	//CharacterMeshComponent->SetMorphTarget("Mikka Head", 1.f);
}

void ABaseCharacter::RebuildEquipment() {
	//UE_LOG(LogTemp, Warning, TEXT("RpcRebuildEquipment"));

	RebuildMorphs();

	//reset foot heel offset
	LeftFootRotator = DefaultFootRotator; 
	RightFootRotator = FRotator(-DefaultFootRotator.Pitch, -DefaultFootRotator.Yaw, DefaultFootRotator.Roll);

	// clear equipment
	for (FString ModularSkMeshName : ModularSkMeshArray) {
		USkeletalMeshComponent* SkeletalMeshComponent = GetFirstComponentByName<USkeletalMeshComponent>(ModularSkMeshName);
		SkeletalMeshComponent->SetSkeletalMesh(nullptr);
	}

	TSet<FString> UsedSkMeshSet;
	UContainerComponent* Equipment = GetFirstComponentByName<UContainerComponent>(TEXT("Equipment"));
	if (Equipment) {
		// rebuild
		TArray<uint64> ObjList = Equipment->GetAllObjects();
		for (uint64 ClassId : ObjList) {
			ASandboxObject* Obj = ASandboxLevelController::GetDefaultSandboxObject(ClassId);
			if (Obj) {
				const int TypeId = Obj->GetSandboxTypeId();
				if (TypeId == SandboxType_Equipment) {
					ASandboxSkeletalModule* Clothing = Cast<ASandboxSkeletalModule>(Obj);
					if (Clothing && Clothing->SkeletalMesh) {
						FString SkMeshBindName = TEXT("ModularSk") + Clothing->SkMeshBindName.ToString() + TEXT("Mesh");
						USkeletalMeshComponent* SkeletalMeshComponent = GetFirstComponentByName<USkeletalMeshComponent>(SkMeshBindName);
						if (SkeletalMeshComponent) {
							USkeletalMeshComponent* CharacterMeshComponent = GetFirstComponentByName<USkeletalMeshComponent>(TEXT("CharacterMesh0"));
							SkeletalMeshComponent->SetSkeletalMesh(Clothing->SkeletalMesh);
							SkeletalMeshComponent->SetMasterPoseComponent(CharacterMeshComponent, true);

							if (Clothing->bModifyFootPose) {
								Clothing->GetFootPose(LeftFootRotator, RightFootRotator);
							}

							for (auto& Entry : Clothing->MorphMap) {
								FString Name = Entry.Key;
								float Value = Entry.Value;
								//UE_LOG(LogTemp, Warning, TEXT("%s %f"), *Name, Value);
								SkeletalMeshComponent->SetMorphTarget(*Name, Value);
							}

							for (auto& Entry : Clothing->ParentMorphMap) {
								FString Name = Entry.Key;
								float Value = Entry.Value;
								//UE_LOG(LogTemp, Warning, TEXT("%s %f"), *Name, Value);
								CharacterMeshComponent->SetMorphTarget(*Name, Value);
							}

							UsedSkMeshSet.Add(Clothing->SkMeshBindName.ToString());
						}
					}
				}
			}
		}
	}

	for (TSubclassOf<ASandboxSkeletalModule> Itm : DefaultModularSkMesh) {
		ASandboxSkeletalModule* DefaultSkMesh = (ASandboxSkeletalModule*)Itm->GetDefaultObject();
		FName BindName = DefaultSkMesh->SkMeshBindName;

		if (!UsedSkMeshSet.Contains(BindName.ToString())) {
			//UE_LOG(LogTemp, Warning, TEXT("Default: %s"), *BindName.ToString());
			FString SkMeshBindName = TEXT("ModularSk") + BindName.ToString() + TEXT("Mesh");
			USkeletalMeshComponent* SkeletalMeshComponent = GetFirstComponentByName<USkeletalMeshComponent>(SkMeshBindName);
			if (SkeletalMeshComponent) {
				USkeletalMeshComponent* CharacterMeshComponent = GetFirstComponentByName<USkeletalMeshComponent>(TEXT("CharacterMesh0"));
				SkeletalMeshComponent->SetSkeletalMesh(DefaultSkMesh->SkeletalMesh);
				SkeletalMeshComponent->SetMasterPoseComponent(CharacterMeshComponent, true);
			}
		}
	}
}

void ABaseCharacter::MakeModularSkList() {
	TArray<USkeletalMeshComponent*> Components;
	GetComponents<USkeletalMeshComponent>(Components);
	for (USkeletalMeshComponent* Component : Components) {
		FString Name = Component->GetName();
		if (Name.StartsWith(TEXT("ModularSk"))) {
			ModularSkMeshArray.Add(Name);
		}
	}
}


void ABaseCharacter::SetCursorMesh(UStaticMesh* NewCursorMesh, UMaterialInstance* Material, const FVector& Location, const FRotator& Rotation, const FVector& Scale) {
	if (IsDead()) {
		return;
	}

	CursorMesh->SetStaticMesh(NewCursorMesh);
	CursorMesh->SetVisibility(true, true);
	CursorMesh->SetRelativeScale3D(Scale);
	CursorMesh->SetWorldLocationAndRotationNoPhysics(Location, Rotation);

	if (Material) {
		for (int I = 0; I < NewCursorMesh->GetStaticMaterials().Num(); ++I) {
			CursorMesh->SetMaterial(I, Material);
		}
	}
}

void ABaseCharacter::ResetCursorMesh() {
	CursorMesh->SetVisibility(false, true);
	CursorMesh->SetStaticMesh(nullptr);
	CursorMesh->SetRelativeScale3D(FVector(1));
}

int ABaseCharacter::GetSandboxTypeId() {
	return SandboxTypeId;
}

FString ABaseCharacter::GetSandboxPlayerUid() {
	return SandboxPlayerUid;
}

float ABaseCharacter::GetStaminaTickDelta() {
	if (GetGait() == EALSGait::Running) {
		return 2.f;
	}

	if (GetGait() == EALSGait::Sprinting) {
		return 10.f;
	}

	return 0.f;
}

void ABaseCharacter::OnStaminaExhausted() {
	SetDesiredGait(EALSGait::Walking);
}

void ABaseCharacter::OnFallDamage(float Velocity) {

	if (GetNetMode() == NM_Client) {
		UE_LOG(LogTemp, Warning, TEXT("OnFallDamage: client: %f"), Velocity);
	} else {
		UE_LOG(LogTemp, Warning, TEXT("OnFallDamage: server: %f"), Velocity);
		UVitalSystemComponent* Vs = GetFirstComponentByName<UVitalSystemComponent>(TEXT("VitalSystem"));
		if (Vs) {
			Vs->DamageFromFall(Velocity);
		}
	}

}

void ABaseCharacter::SandboxDamage(float Val) {
	UVitalSystemComponent* Vs = GetFirstComponentByName<UVitalSystemComponent>(TEXT("VitalSystem"));
	if (Vs) {
		//RightHandCollisionComponent->OnComponentBeginOverlap.AddDynamic(this, &ABaseCharacter::BeginOverlap);
	}
}

void ABaseCharacter::Kill() {
	if (DeathAnim) {
		SetViewMode(EALSViewMode::ThirdPerson);
		Replicated_PlayMontage(DeathAnim, 1.35);
		State = -1;
	}

	//ReplicatedRagdollStart();
}


//==========================================================================================