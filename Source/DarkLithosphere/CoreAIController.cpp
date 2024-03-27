// Fill out your copyright notice in the Description page of Project Settings.

#include "CoreAIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
//#include "Classes/Kismet/KismetSystemLibrary.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Vector.h"
#include "NavigationSystem.h"
#include "SandboxObject.h"
#include "BaseCharacter.h"
#include "DrawDebugHelpers.h"


ASandboxObject* FindNearestLightSource(UWorld* World, FVector Origin, const float Radius) {
	float MinDistance = MAX_FLT;
	ASandboxObject* Res = nullptr;

	for (TActorIterator<ASandboxObject> ActorItr(World); ActorItr; ++ActorItr) {
		ASandboxObject* Object = *ActorItr;
		TArray<ULightComponent*> LightComponents;
		Object->GetComponents<ULightComponent>(LightComponents);
		for (auto* LightComponent : LightComponents) {
			float Distance = FVector::Distance(Origin, LightComponent->GetComponentLocation());
			if (Distance < MinDistance) {
				MinDistance = Distance;
				Res = Object;
			}
		}

	}

	return Res;
}

AActor* FindNearestInterestingActor(UWorld* World, FVector Origin, const float Radius) {
	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
	TArray<AActor*> ActorsToIgnore;
	TArray<AActor*> OutActors;

	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECollisionChannel::ECC_WorldDynamic));
	UKismetSystemLibrary::SphereOverlapActors(World, Origin, Radius, ObjectTypes, nullptr, ActorsToIgnore, OutActors);

	if (OutActors.Num() > 0) {
		AActor* ResultActor = nullptr;
		float MinDistance = MAX_FLT;
		for (auto* Actor : OutActors) {
			float Distance = FVector::Distance(Actor->GetActorLocation(), Origin);
			if (Distance < MinDistance) {
				MinDistance = Distance;
				ResultActor = Actor;
			}
		}

		return ResultActor;
	}

	return nullptr;
}





UBTService_SetMovementSpeed::UBTService_SetMovementSpeed(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
	NodeName = "Set MovementSpeed";
	bNotifyBecomeRelevant = true;
}

void UBTService_SetMovementSpeed::OnBecomeRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) {
	Super::OnBecomeRelevant(OwnerComp, NodeMemory);

	ACharacter* Npc = OwnerComp.GetAIOwner()->GetCharacter();
	if (Npc) {
		Npc->GetCharacterMovement()->MaxWalkSpeed = 400;
	}
}

FString UBTService_SetMovementSpeed::GetStaticDescription() const {
	return "Set pawn movement speed";
}



//=========================================================================================================================================


UBTTask_SetFocusToPoint::UBTTask_SetFocusToPoint(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
	NodeName = "Set Focus to Point";
}

EBTNodeResult::Type UBTTask_SetFocusToPoint::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) {

	FVector Pos = OwnerComp.GetBlackboardComponent()->GetValueAsVector(BlackboardKey.SelectedKeyName);
	OwnerComp.GetAIOwner()->SetFocalPoint(Pos);
	return EBTNodeResult::Succeeded;
}

FString UBTTask_SetFocusToPoint::GetStaticDescription() const {
	return "Set Focus to point";
}

//=========================================================================================================================================



UBTTask_SelectWalkTarget::UBTTask_SelectWalkTarget(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
	NodeName = "Select Walk Target";
	WalkTargetRadius = 1000;
	LightFindingRadius = 10000;
	NightWalkingRadius = 300;
}

EBTNodeResult::Type UBTTask_SelectWalkTarget::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) {
	EBTNodeResult::Type NodeResult = EBTNodeResult::Succeeded;

	const ACoreAIController* CoreController = Cast<ACoreAIController>(OwnerComp.GetAIOwner());
	if (!CoreController) {
		return EBTNodeResult::Failed;
	}

	const FVector OwnerLocation = CoreController->GetPawn()->GetActorLocation();
	bool bIsNight = CoreController->IsNight();
	AActor* ResultActor = nullptr;

	if (bIsNight) {
		//UE_LOG(LogTemp, Warning, TEXT("bIsNight"));
		ResultActor = FindNearestLightSource(GetWorld(), OwnerLocation, WalkTargetRadius);
	} else {
		ResultActor = FindNearestInterestingActor(GetWorld(), OwnerLocation, LightFindingRadius);
	}

	/*
	if (ResultActor) {
			const float WalkRadius = (bIsNight) ? NightWalkingRadius : WalkTargetRadius * 0.95;
			FVector TargetLocation = ResultActor->GetActorLocation();
			FNavLocation ResultLocation;
			UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(GetWorld());
			bool bIsSuccess = NavSys->GetRandomReachablePointInRadius(ResultActor->GetActorLocation(), WalkRadius, ResultLocation, nullptr, FSharedConstNavQueryFilter());
			if (bIsSuccess) {
				//UE_LOG(LogTemp, Warning, TEXT("set blackboard destination -> %s %f %f %f"), *ResultActor->GetName(), ResultLocation.Location.X, ResultLocation.Location.Y, ResultLocation.Location.Z);
				OwnerComp.GetBlackboardComponent()->SetValue<UBlackboardKeyType_Vector>(BlackboardKey.GetSelectedKeyID(), ResultLocation.Location);
			}

	} else {
		// nothing found
		const float WalkRadius = (bIsNight) ? NightWalkingRadius : WalkTargetRadius * 2;
		FNavLocation ResultLocation;
		UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(GetWorld());
		bool bIsSuccess = NavSys->GetRandomReachablePointInRadius(OwnerLocation, WalkRadius, ResultLocation, nullptr, FSharedConstNavQueryFilter());
		if (bIsSuccess) {
			//UE_LOG(LogTemp, Warning, TEXT("set blackboard destination -> %f %f %f"), ResultLocation.Location.X, ResultLocation.Location.Y, ResultLocation.Location.Z);
			OwnerComp.GetBlackboardComponent()->SetValue<UBlackboardKeyType_Vector>(BlackboardKey.GetSelectedKeyID(), ResultLocation.Location);
		}
	}
	*/

	// nothing found
	const float WalkRadius = (bIsNight) ? NightWalkingRadius : WalkTargetRadius * 2;
	FNavLocation ResultLocation;
	UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(GetWorld());
	bool bIsSuccess = NavSys->GetRandomReachablePointInRadius(OwnerLocation, WalkRadius, ResultLocation, nullptr, FSharedConstNavQueryFilter());
	if (bIsSuccess) {
		UE_LOG(LogTemp, Warning, TEXT("set blackboard destination -> %f %f %f"), ResultLocation.Location.X, ResultLocation.Location.Y, ResultLocation.Location.Z);
		OwnerComp.GetBlackboardComponent()->SetValue<UBlackboardKeyType_Vector>(BlackboardKey.GetSelectedKeyID(), ResultLocation.Location);
	}

	return NodeResult;
}

//=========================================================================================================================================

UBTTask_SelectHumanWalkTarget::UBTTask_SelectHumanWalkTarget(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
	NodeName = "Select Walk Target";
	WalkTargetRadius = 1000;
	LightFindingRadius = 10000;
	NightWalkingRadius = 300;
}

EBTNodeResult::Type UBTTask_SelectHumanWalkTarget::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) {
	EBTNodeResult::Type NodeResult = EBTNodeResult::Succeeded;

	const ACoreAIController* CoreController = Cast<ACoreAIController>(OwnerComp.GetAIOwner());
	if (!CoreController) {
		return EBTNodeResult::Failed;
	}

	const FVector OwnerLocation = CoreController->GetPawn()->GetActorLocation();
	bool bIsNight = CoreController->IsNight();
	AActor* ResultActor = nullptr;

	if (bIsNight) {
		//UE_LOG(LogTemp, Warning, TEXT("bIsNight"));
		ResultActor = FindNearestLightSource(GetWorld(), OwnerLocation, WalkTargetRadius);
	} else {
		ResultActor = FindNearestInterestingActor(GetWorld(), OwnerLocation, LightFindingRadius);
	}

	if (ResultActor) {
		const float WalkRadius = (bIsNight) ? NightWalkingRadius : WalkTargetRadius * 0.95;
		FVector TargetLocation = ResultActor->GetActorLocation();
		FNavLocation ResultLocation;
		UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(GetWorld());
		bool bIsSuccess = NavSys->GetRandomReachablePointInRadius(ResultActor->GetActorLocation(), WalkRadius, ResultLocation, nullptr, FSharedConstNavQueryFilter());
		if (bIsSuccess) {
			//UE_LOG(LogTemp, Warning, TEXT("set blackboard destination -> %s %f %f %f"), *ResultActor->GetName(), ResultLocation.Location.X, ResultLocation.Location.Y, ResultLocation.Location.Z);
			OwnerComp.GetBlackboardComponent()->SetValue<UBlackboardKeyType_Vector>(BlackboardKey.GetSelectedKeyID(), ResultLocation.Location);
		}

	} else {
		// nothing found
		const float WalkRadius = (bIsNight) ? NightWalkingRadius : WalkTargetRadius * 2;
		FNavLocation ResultLocation;
		UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(GetWorld());
		bool bIsSuccess = NavSys->GetRandomReachablePointInRadius(OwnerLocation, WalkRadius, ResultLocation, nullptr, FSharedConstNavQueryFilter());
		if (bIsSuccess) {
			//UE_LOG(LogTemp, Warning, TEXT("set blackboard destination -> %f %f %f"), ResultLocation.Location.X, ResultLocation.Location.Y, ResultLocation.Location.Z);
			OwnerComp.GetBlackboardComponent()->SetValue<UBlackboardKeyType_Vector>(BlackboardKey.GetSelectedKeyID(), ResultLocation.Location);
		}
	}

	return NodeResult;
}

//=========================================================================================================================================

UBTTask_SelectZombieWalkTarget::UBTTask_SelectZombieWalkTarget(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
	NodeName = "Select Zombie Walk Target";
	DayWalkTargetRadius = 300;
	LightFindingRadius = 10000;
	NightWalkingRadius = 2000;
}

EBTNodeResult::Type UBTTask_SelectZombieWalkTarget::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) {
	EBTNodeResult::Type NodeResult = EBTNodeResult::Succeeded;

	const ACoreAIController* CoreController = Cast<ACoreAIController>(OwnerComp.GetAIOwner());
	if (!CoreController) {
		return EBTNodeResult::Failed;
	}

	const FVector OwnerLocation = CoreController->GetPawn()->GetActorLocation();
	bool bIsNight = CoreController->IsNight();
	AActor* ResultActor = nullptr;

	
	if (bIsNight) {
		UE_LOG(LogTemp, Warning, TEXT("bIsNight"));
		ResultActor = FindNearestLightSource(GetWorld(), OwnerLocation, LightFindingRadius);
	} else {
		ResultActor = FindNearestInterestingActor(GetWorld(), OwnerLocation, DayWalkTargetRadius);
	}

	if (ResultActor) {
		const float WalkRadius = (bIsNight) ? NightWalkingRadius : DayWalkTargetRadius * 0.95;
		FVector TargetLocation = ResultActor->GetActorLocation();
		FNavLocation ResultLocation;
		UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(GetWorld());
		bool bIsSuccess = NavSys->GetRandomReachablePointInRadius(ResultActor->GetActorLocation(), WalkRadius, ResultLocation, nullptr, FSharedConstNavQueryFilter());
		if (bIsSuccess) {
			UE_LOG(LogTemp, Warning, TEXT("zombie destination -> %s %f %f %f"), *ResultActor->GetName(), ResultLocation.Location.X, ResultLocation.Location.Y, ResultLocation.Location.Z);
			//DrawDebugPoint(GetWorld(), ResultLocation.Location, 10.f, FColor(255, 0, 0, 0), false, 3);
			OwnerComp.GetBlackboardComponent()->SetValue<UBlackboardKeyType_Vector>(BlackboardKey.GetSelectedKeyID(), ResultLocation.Location);
		}

	} else {
		// nothing found
		const float WalkRadius = (bIsNight) ? NightWalkingRadius : DayWalkTargetRadius * 2;
		FNavLocation ResultLocation;
		UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(GetWorld());
		bool bIsSuccess = NavSys->GetRandomReachablePointInRadius(OwnerLocation, WalkRadius, ResultLocation, nullptr, FSharedConstNavQueryFilter());
		if (bIsSuccess) {
			UE_LOG(LogTemp, Warning, TEXT("zombie destination -> %f %f %f"), ResultLocation.Location.X, ResultLocation.Location.Y, ResultLocation.Location.Z);
			//DrawDebugPoint(GetWorld(), ResultLocation.Location, 10.f, FColor(255, 0, 0, 0), false, 3);
			OwnerComp.GetBlackboardComponent()->SetValue<UBlackboardKeyType_Vector>(BlackboardKey.GetSelectedKeyID(), ResultLocation.Location);
		}
	}

	return NodeResult;
}

//=========================================================================================================================================
// Ghost
//=========================================================================================================================================


UBTTask_SelectGhostTarget::UBTTask_SelectGhostTarget(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
	NodeName = "Select Ghost Target";
	FindingRadius = 2000;
	NightFloatingRadius = 2000;
}

EBTNodeResult::Type UBTTask_SelectGhostTarget::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) {
	EBTNodeResult::Type NodeResult = EBTNodeResult::Succeeded;

	const ACoreAIController* CoreController = Cast<ACoreAIController>(OwnerComp.GetAIOwner());
	if (!CoreController) {
		return EBTNodeResult::Failed;
	}

	const FVector OwnerLocation = CoreController->GetPawn()->GetActorLocation();
	bool bIsNight = CoreController->IsNight();
	AActor* ResultActor = nullptr;

	const float FloatingRadius = NightFloatingRadius;
	FNavLocation ResultLocation;
	UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(GetWorld());
	bool bIsSuccess = NavSys->GetRandomReachablePointInRadius(OwnerLocation, FloatingRadius, ResultLocation, nullptr, FSharedConstNavQueryFilter());
	if (bIsSuccess) {
		FVector Pos = ResultLocation.Location;

		Pos.Z += 100;

		UE_LOG(LogTemp, Warning, TEXT("Ghost target: %f %f %f"), Pos.X, Pos.Y, Pos.Z);
		//DrawDebugPoint(GetWorld(), Pos, 6.f, FColor(0, 255, 0, 0), false, 3);
		OwnerComp.GetBlackboardComponent()->SetValue<UBlackboardKeyType_Vector>(BlackboardKey.GetSelectedKeyID(), Pos);
	}

	return NodeResult;
}

//=========================================================================================================================================




void ACoreAIController::BeginPlay() {
	Super::BeginPlay();

	//GetDefault<UAISystem>()->bForgetStaleActors = true;

	UAIPerceptionComponent* AIPerceptionComponent = GetFirstComponentByName<UAIPerceptionComponent>(TEXT("AIPerception"));
	if (AIPerceptionComponent) {
		AIPerceptionComponent->OnPerceptionUpdated.AddDynamic(this, &ACoreAIController::OnPerceptionUpdated);
		AIPerceptionComponent->OnTargetPerceptionForgotten.AddDynamic(this, &ACoreAIController::OnTargetPerceptionForgotten);
		AIPerceptionComponent->OnTargetPerceptionUpdated.AddDynamic(this, &ACoreAIController::OnTargetPerceptionUpdated);
		AIPerceptionComponent->OnTargetPerceptionInfoUpdated.AddDynamic(this, &ACoreAIController::OnTargetPerceptionInfoUpdated);
	}

	// find first level controller
	for (TActorIterator<ASandboxEnvironment> ActorItr(GetWorld()); ActorItr; ++ActorItr) {
		Environment = *ActorItr;
	}

	ABaseCharacter* BaseCharacter = Cast<ABaseCharacter>(GetCharacter());
	if (BaseCharacter) {
		if (BaseCharacter->BehaviourTree) {
			UE_LOG(LogTemp, Warning, TEXT("Use BaseCharacter->BehaviourTree"));
			RunBehaviorTree(BaseCharacter->BehaviourTree);
			return;
		}
	}

	if (MainBehaviourTree) {
		UE_LOG(LogTemp, Warning, TEXT("Use MainBehaviourTree"));
		RunBehaviorTree(MainBehaviourTree);
	}
}

void ACoreAIController::OnPerceptionUpdated(const TArray<AActor*>& UpdatedActors) {
	UE_LOG(LogTemp, Warning, TEXT("OnPerceptionUpdated"));

	for (const auto Actor : UpdatedActors) {
		//UE_LOG(LogTemp, Warning, TEXT("UpdatedActor: %s"), *Actor->GetName());
	}
}

void ACoreAIController::OnTargetPerceptionForgotten(AActor* Actor) {
	UE_LOG(LogTemp, Warning, TEXT("OnTargetPerceptionForgotten: %s"), *Actor->GetName());
	TargetActor = nullptr;
}

void ACoreAIController::OnTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus) {
	UE_LOG(LogTemp, Warning, TEXT("OnTargetPerceptionUpdated: %s"), *Actor->GetName());

	bTargetActive = Stimulus.IsActive();
	StimulusLocation = Stimulus.StimulusLocation;

	if (TargetActor == nullptr) {
		TargetActor = Actor;
	}
}

void ACoreAIController::OnTargetPerceptionInfoUpdated(const FActorPerceptionUpdateInfo& UpdateInfo) {
	UE_LOG(LogTemp, Warning, TEXT("OnTargetPerceptionInfoUpdated"));
}

void ACoreAIController::Tick(float DeltaTime) {
	Super::Tick(DeltaTime);

	GetBlackboardComponent()->SetValueAsObject(TEXT("Target"), TargetActor);


	if (TargetActor) {
		FVector Pos = bTargetActive ? TargetActor->GetActorLocation() : StimulusLocation;

		FVector NpcPos = GetCharacter()->GetActorLocation();
		double Distance = FVector::Distance(NpcPos, TargetActor->GetActorLocation());

		if (Distance < 300.f) {
			Pos = TargetActor->GetActorLocation();
		}

		//DrawDebugPoint(GetWorld(), Pos, 5.f, bTargetActive ? FColor::White : FColor::Red, false, 1.f);
		GetBlackboardComponent()->SetValueAsVector(TEXT("Destination"), Pos);
	}

}

void ACoreAIController::StopBehaviorTree() {
	if (BrainComponent) {
		BrainComponent->StopLogic(TEXT(""));
	}
}

void ACoreAIController::RestartBehaviorTree() {
	if (BrainComponent) {
		BrainComponent->RestartLogic();
	}
}

bool ACoreAIController::IsNight() const {
	if(Environment) {
		return Environment->IsNight();
	}

	return false;
}
