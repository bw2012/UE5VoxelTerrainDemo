// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "BehaviorTree/Tasks/BTTask_BlackboardBase.h"
#include "SandboxEnvironment.h"
#include "CoreAIController.generated.h"

UCLASS(config = Game)
class DARKLITHOSPHERE_API UBTTask_SelectWalkTarget : public UBTTask_BlackboardBase {
	GENERATED_UCLASS_BODY()

	UPROPERTY(config, Category = Node, EditAnywhere, meta = (ClampMin = "0.0", UIMin = "0.0"))
	float WalkTargetRadius;

	UPROPERTY(config, Category = Node, EditAnywhere, meta = (ClampMin = "0.0", UIMin = "0.0"))
	float LightFindingRadius;

	UPROPERTY(config, Category = Node, EditAnywhere, meta = (ClampMin = "0.0", UIMin = "0.0"))
	float NightWalkingRadius;

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

};

UCLASS(config = Game)
class DARKLITHOSPHERE_API UBTTask_SelectHumanWalkTarget : public UBTTask_BlackboardBase {
	GENERATED_UCLASS_BODY()

	UPROPERTY(config, Category = Node, EditAnywhere, meta = (ClampMin = "0.0", UIMin = "0.0"))
	float LightFindingRadius;

	UPROPERTY(config, Category = Node, EditAnywhere, meta = (ClampMin = "0.0", UIMin = "0.0"))
	float WalkTargetRadius;

	UPROPERTY(config, Category = Node, EditAnywhere, meta = (ClampMin = "0.0", UIMin = "0.0"))
	float NightWalkingRadius;

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

};

UCLASS(config = Game)
class DARKLITHOSPHERE_API UBTTask_SelectZombieWalkTarget : public UBTTask_BlackboardBase {
	GENERATED_UCLASS_BODY()

	UPROPERTY(config, Category = Node, EditAnywhere, meta = (ClampMin = "0.0", UIMin = "0.0"))
	float LightFindingRadius;

	UPROPERTY(config, Category = Node, EditAnywhere, meta = (ClampMin = "0.0", UIMin = "0.0"))
	float DayWalkTargetRadius;

	UPROPERTY(config, Category = Node, EditAnywhere, meta = (ClampMin = "0.0", UIMin = "0.0"))
	float NightWalkingRadius;

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

};

UCLASS(config = Game)
class DARKLITHOSPHERE_API UBTTask_SelectGhostTarget : public UBTTask_BlackboardBase {
	GENERATED_UCLASS_BODY()

	UPROPERTY(config, Category = Node, EditAnywhere, meta = (ClampMin = "0.0", UIMin = "0.0"))
	float FindingRadius;

	UPROPERTY(config, Category = Node, EditAnywhere, meta = (ClampMin = "0.0", UIMin = "0.0"))
	float NightFloatingRadius;

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
};


/**
 * 
 */
UCLASS()
class DARKLITHOSPHERE_API ACoreAIController : public AAIController
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, Category = "Sandbox")
	UBehaviorTree* MainBehaviourTree;

	ASandboxEnvironment* Environment;

	void StopBehaviorTree();

	void RestartBehaviorTree();

	bool IsNight() const;

protected:

	virtual void BeginPlay() override;
	
};
