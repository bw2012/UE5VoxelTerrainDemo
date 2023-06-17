
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EntryHelper.generated.h"

UCLASS()
class DARKLITHOSPHERE_API AEntryHelper : public AActor {
	GENERATED_BODY()
	
public:	

	AEntryHelper();

protected:

	virtual void BeginPlay() override;

public:	

	virtual void Tick(float DeltaTime) override;

public:

	UPROPERTY(EditAnywhere, Category = "Sandbox")
	TSubclassOf<class UUserWidget> FatalMessageWidget;

private:
	void CheckUpdates();

	bool bFirstTick = true;

};
