// Copyright 1998-2021 Epic Games, Inc. All Rights Reserved.

#include "Door.h"
#include "Components/BoxComponent.h"
#include "Kismet/KismetMathLibrary.h"

#include "DrawDebugHelpers.h"

// Sets default values
ADoor::ADoor() {
    // Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
    PrimaryActorTick.bCanEverTick = true;

    //Create our Default Components
    //DoorFrame = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DoorFrameMesh"));
    Door = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DoorMesh"));
    DoorTimelineComp = CreateDefaultSubobject<UTimelineComponent>(TEXT("DoorTimelineComp"));
    DoorProxVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("DoorProximityVolume"));

    //Setup our Attachments
    //SandboxRootMesh->SetupAttachment(RootComponent);
    Door->AttachToComponent(SandboxRootMesh, FAttachmentTransformRules::KeepRelativeTransform);
    DoorProxVolume->AttachToComponent(SandboxRootMesh, FAttachmentTransformRules::KeepRelativeTransform);

    MainSound = CreateDefaultSubobject<UAudioComponent>(TEXT("MainSound"));
    MainSound->AttachToComponent(SandboxRootMesh, FAttachmentTransformRules::KeepRelativeTransform);
}

// Called when the game starts or when spawned
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
}

void ADoor::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult) {
    //DoorTimelineComp->Play();
}

void ADoor::OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex) {
    //GEngine->AddOnScreenDebugMessage(-1, 10, FColor::White, TEXT("ADoor::OnOverlapEnd"));
    //DoorTimelineComp->Reverse();
}

// Called every frame
void ADoor::Tick(float DeltaTime) {
    Super::Tick(DeltaTime);
}

bool ADoor::IsInteractive(const APawn* Source) {
    return true;
}

void ADoor::MainInteraction(const APawn* Source) {
    if (Source) {

        if (DoorState == 0) {
            FVector CharacterLocation = Source->GetActorLocation();
            FVector DoorLocation = DoorProxVolume->GetComponentLocation();
            FVector Test = UKismetMathLibrary::GetDirectionUnitVector(DoorLocation, CharacterLocation);
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
            MainSound->Play();
        } else {
            DoorState = 0;
            DoorTimelineComp->Reverse();
            MainSound->Play();
        }
    }
}

const UStaticMeshComponent* ADoor::GetMarkerMesh() const {
    return Door;
}