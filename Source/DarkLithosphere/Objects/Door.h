// Copyright 1998-2021 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ConstructionObject.h"
#include "Components/TimelineComponent.h"
#include "Door.generated.h"

UCLASS()
class DARKLITHOSPHERE_API ADoor : public AConstructionObject {
    GENERATED_BODY()

public:

    ADoor();

    //Variable to hold Curve asset
    UPROPERTY(EditAnywhere)
    UCurveFloat* DoorTimelineFloatCurve;

protected:

    virtual void BeginPlay() override;

    /*MeshComponents to represents Door assets*/
    //UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Sandbox")
    //UStaticMeshComponent* DoorFrame;

    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Sandbox")
    UStaticMeshComponent* Door;

    //TimelineComponent to animate door meshes
    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Sandbox")
    UTimelineComponent* DoorTimelineComp;

    //BoxComponent which will be used as our proximity volume.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sandbox")
    class UBoxComponent* DoorProxVolume;

    //UPROPERTY(EditAnywhere, Category = "Sandbox")
    //UAudioComponent* MainSound;

    UPROPERTY(EditAnywhere, Category = "Sandbox")
    int EffectId;

    //Float Track Signature to handle our update track event
    FOnTimelineFloat UpdateFunctionFloat;

    //Function which updates our Door's relative location with the timeline graph
    UFUNCTION()
    void UpdateTimelineComp(float Output);

    /*Begin and End Overlap Events for our DoorProxVolume*/
    UFUNCTION()
    void OnOverlapBegin(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

    UFUNCTION()
    void OnOverlapEnd(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

public:

    virtual void Tick(float DeltaTime) override;

    virtual bool IsInteractive(const APawn* Source);

    virtual void MainInteraction(const APawn* Source);

    void DoorInteraction(const FVector& PlayerPos);

    UFUNCTION(NetMulticast, Reliable)
    void MulticastRpcDoorInteraction(const FVector& PlayerPos);

    virtual const UStaticMeshComponent* GetMarkerMesh() const;

    int GetSandboxTypeId() const override;

    UFUNCTION()
    void OnRep_Rotation();

private:

    bool bOpenDirection;

    UPROPERTY(Replicated)
    int DoorState = 0;

    UPROPERTY(ReplicatedUsing = OnRep_Rotation)
    FRotator ReplicatedRotation;
};