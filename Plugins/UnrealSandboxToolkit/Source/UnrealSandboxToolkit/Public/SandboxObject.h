
#pragma once

#include "Engine.h"
#include "GameFramework/Actor.h"
#include "ContainerComponent.h"
#include "SandboxObject.generated.h"

UCLASS(BlueprintType, Blueprintable)
class UNREALSANDBOXTOOLKIT_API ASandboxObject : public AActor {
	GENERATED_BODY()
	
public:	
	ASandboxObject();

	//UPROPERTY(EditAnywhere, Category = "Sandbox")
	UPROPERTY(Category = StaticMeshActor, VisibleAnywhere, BlueprintReadOnly, meta = (ExposeFunctionCategories = "Mesh,Rendering,Physics,Components|StaticMesh", AllowPrivateAccess = "true"))
	UStaticMeshComponent* SandboxRootMesh;

	UPROPERTY(EditAnywhere, Category = "Sandbox")
	UTexture2D* IconTexture;

	UPROPERTY(EditAnywhere, Category = "Sandbox")
	uint64 SandboxClassId;

	UPROPERTY(Replicated)
	uint64 SandboxNetUid;

	UPROPERTY(EditAnywhere, Category = "Sandbox")
	bool bStackable;

	UPROPERTY(EditAnywhere, Category = "Sandbox")
	uint32 MaxStackSize;

	UPROPERTY(EditAnywhere, Category = "Sandbox")
	TMap<FString, FString> PropertyMap;

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnSleep(UPrimitiveComponent* SleepingComponent, FName BoneName);

public:

	uint64 GetSandboxNetUid() const;

	uint64 GetSandboxClassId() const;

	virtual FString GetSandboxName();

	virtual int GetSandboxTypeId() const;

	virtual int GetMaxStackSize();
	
	virtual UTexture2D* GetSandboxIconTexture();

	virtual void TickInInventoryActive(float DeltaTime, UWorld* World, const FHitResult& HitResult);

	virtual void ActionInInventoryActive(UWorld* World, const FHitResult& HitResult);

	virtual bool IsInteractive(const APawn* Source = nullptr);

	virtual void MainInteraction(const APawn* Source = nullptr);

	virtual bool CanTake(const AActor* Actor = nullptr) const;

	virtual void OnTerrainChange();
    
	UContainerComponent* GetContainer(const FString& Name);

	template<class T>
	T* GetFirstComponentByName(FString ComponentName) {
		TArray<T*> Components;
		GetComponents<T>(Components);
		for (T* Component : Components) {
			if (Component->GetName() == ComponentName)
				return Component;
		}

		return nullptr;
	}

	void SetProperty(FString Key, FString Value);

	FString GetProperty(FString Key) const;

	void RemoveProperty(FString Key);

	virtual void PostLoadProperties();

	virtual void OnPlaceToWorld();

	virtual bool PlaceToWorldClcPosition(const UWorld* World, const FVector& SourcePos, const FRotator& SourceRotation, const FHitResult& Res, FVector& Location, FRotator& Rotation, bool bFinal = false) const;

	virtual const UStaticMeshComponent* GetMarkerMesh() const;
};


class IWearable {

public:

};


UCLASS()
class UNREALSANDBOXTOOLKIT_API ASandboxSkeletalModule : public ASandboxObject, public IWearable {
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, Category = "Sandbox")
	FName SkMeshBindName;

	UPROPERTY(EditAnywhere, Category = "Sandbox")
	USkeletalMesh* SkeletalMesh;

	UPROPERTY(EditAnywhere, Category = "Sandbox")
	bool bModifyFootPose;

	UPROPERTY(EditAnywhere, Category = "Sandbox")
	FRotator FootRotator;

	UPROPERTY(EditAnywhere, Category = "Sandbox")
	TMap<FString, float> MorphMap;

	UPROPERTY(EditAnywhere, Category = "Sandbox")
	TMap<FString, float> ParentMorphMap;

	int GetSandboxTypeId() const override;

	void GetFootPose(FRotator& LeftFootRotator, FRotator& RightFootRotator);

};