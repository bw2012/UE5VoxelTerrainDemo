

#pragma once

#include "CoreMinimal.h"
#include "SandboxObject.h"
#include "Clothing.generated.h"

/**
 * 
 */
UCLASS()
class DARKLITHOSPHERE_API AClothing : public ASandboxObject, public IWearable
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, Category = "Sandbox")
	USkeletalMesh* SkeletalMesh;

	UPROPERTY(EditAnywhere, Category = "Sandbox")
	bool bModifyFootPose;

	UPROPERTY(EditAnywhere, Category = "Sandbox")
	FRotator FootRotator;

	UPROPERTY(EditAnywhere, Category = "Sandbox")
	TMap<FString, float> MorphMap;

	int GetSandboxTypeId() const override;
	
	void GetFootPose(FRotator& LeftFootRotator, FRotator& RightFootRotator);

};
