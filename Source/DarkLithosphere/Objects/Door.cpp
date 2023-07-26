// Copyright 1998-2021 Epic Games, Inc. All Rights Reserved.

#include "Door.h"
#include "Components/BoxComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Net/UnrealNetwork.h"
#include "../LevelController.h"
#include "DrawDebugHelpers.h"

ADoor::ADoor() {
    PrimaryActorTick.bCanEverTick = true;
    bReplicates = true;
    SetReplicateMovement(true);

    //Create our Default Components
    //DoorFrame = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DoorFrameMesh"));
    Door = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DoorMesh"));
    DoorTimelineComp = CreateDefaultSubobject<UTimelineComponent>(TEXT("DoorTimelineComp"));
    DoorProxVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("DoorProximityVolume"));

    //Setup our Attachments
    //SandboxRootMesh->SetupAttachment(RootComponent);
    Door->AttachToComponent(SandboxRootMesh, FAttachmentTransformRules::KeepRelativeTransform);
    DoorProxVolume->AttachToComponent(SandboxRootMesh, FAttachmentTransformRules::KeepRelativeTransform);

    //MainSound = CreateDefaultSubobject<UAudioComponent>(TEXT("MainSound"));
    //MainSound->AttachToComponent(SandboxRootMesh, FAttachmentTransformRules::KeepRelativeTransform);
}

void ADoor::BeginPlay() {
    Super::BeginPlay();

    UpdateFunctionFloat.BindDynamic(this, &ADoor::UpdateTimelineComp);

    //If we have a float curve, bind it's graph to our update function
    if (DoorTimelineFloatCurve) {
        DoorTimelineComp->AddInterpFloat(DoorTimelineFloatCurve, UpdateFunctionFloat);
    }

    //Binding our Proximity Box Component to our Overlap Functions
    DoorProxVolume->OnComponentBeginOverlap.AddDynamic(this, &ADoor::OnOverlapBegin);
    DoorProxVolume->OnComponentEndOverlap.AddDynamic(this, &ADoor::OnOverlapEnd);
}

void ADoor::UpdateTimelineComp(float Output) {
    FRotator DoorNewRotation = FRotator(0.0f, Output, 0.f);

    if (bOpenDirection) {
        DoorNewRotation = DoorNewRotation.GetInverse();
    }

    Door->SetRelativeRotation(DoorNewRotation);
    ReplicatedRotation = DoorNewRotation;
}

void ADoor::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult) {
    //DoorTimelineComp->Play();
}

void ADoor::OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex) {
    //GEngine->AddOnScreenDebugMessage(-1, 10, FColor::White, TEXT("ADoor::OnOverlapEnd"));
    //DoorTimelineComp->Reverse();
}

void ADoor::OnRep_Rotation() {
    Door->SetRelativeRotation(ReplicatedRotation); // SetReplicateMovement not working
}

void ADoor::Tick(float DeltaTime) {
    Super::Tick(DeltaTime);
}

bool ADoor::IsInteractive(const APawn* Source) {
    return true;
}

void ADoor::DoorInteraction(const FVector& PlayerPos) {
    if (DoorState == 0) {
        FVector DoorLocation = DoorProxVolume->GetComponentLocation();
        FVector Test = UKismetMathLibrary::GetDirectionUnitVector(DoorLocation, PlayerPos);
        FVector DoorForward = GetActorForwardVector();
        const static FRotator Rotator(0, 90, 0);
        DoorForward = Rotator.RotateVector(DoorForward);

        //DrawDebugLine(GetWorld(), DoorLocation, DoorLocation + Test * 100, FColor(255, 0, 0, 0), false, 5);
        //DrawDebugLine(GetWorld(), DoorLocation, DoorLocation + DoorForward * 100, FColor(255, 200, 0, 0), false, 5);

        bool Dir = DoorForward.Equals(Test, 1);
        if (Dir) {
            bOpenDirection = true; // back
        } else {
            bOpenDirection = false; // front
        }

        DoorState = 1;
        DoorTimelineComp->Play();

        ASandboxLevelController* LevelController = ASandboxLevelController::GetInstance();
        if (LevelController) {
            LevelController->SpawnEffect(EffectId, GetActorTransform());
        }
    } else {
        DoorState = 0;
        DoorTimelineComp->Reverse();

        ASandboxLevelController* LevelController = ASandboxLevelController::GetInstance();
        if (LevelController) {
            LevelController->SpawnEffect(EffectId, GetActorTransform());
        }
    }
}

void ADoor::MulticastRpcDoorInteraction_Implementation(const FVector& PlayerPos) {
    DoorInteraction(PlayerPos);
}

void ADoor::MainInteraction(const APawn* Source) {
    if (GetNetMode() == NM_Client) {
        return;
    }

    if (Source) {
        DoorInteraction(Source->GetActorLocation());
    }
}

const UStaticMeshComponent* ADoor::GetMarkerMesh() const {
    return Door;
}

void ADoor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const {
    DOREPLIFETIME(ADoor, DoorState);
    DOREPLIFETIME(ADoor, ReplicatedRotation);
}

int ADoor::GetSandboxTypeId() const {
    return SandboxType_Door;
}