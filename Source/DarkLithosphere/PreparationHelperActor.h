#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PreparationHelperActor.generated.h"

UCLASS()
class DARKLITHOSPHERE_API APreparationHelperActor : public AActor {
	GENERATED_BODY()
	
public:	

	APreparationHelperActor();

protected:

	virtual void BeginPlay() override;

public:	

	UPROPERTY(EditAnywhere, Category = "Sandbox")
	TSubclassOf<class UUserWidget> FatalMessageWidget;

	virtual void Tick(float DeltaTime) override;

private:
	void CheckUpdates();

};
