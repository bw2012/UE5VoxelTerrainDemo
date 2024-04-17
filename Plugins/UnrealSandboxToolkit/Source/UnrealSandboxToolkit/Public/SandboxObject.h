
#pragma once

#include "Engine.h"
#include "GameFramework/Actor.h"
#include "ContainerComponent.h"
#include "Engine/DamageEvents.h"
#include "SandboxObject.generated.h"



#define DAMAGE_ENABLE_PHYS_THRESHOLD 1.f



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
	FString SandboxNetUid;

	UPROPERTY(EditAnywhere, Category = "Sandbox")
	bool bStackable;

	UPROPERTY(EditAnywhere, Category = "Sandbox")
	uint32 MaxStackSize;

	UPROPERTY(EditAnywhere, Category = "Sandbox")
	TMap<FString, FString> PropertyMap;

	UPROPERTY(EditAnywhere, Category = "Sandbox")
	bool bCanPlaceSandboxObject;

	UPROPERTY(EditAnywhere, Category = "Sandbox")
	float DamageEnablePhysThreshold = DAMAGE_ENABLE_PHYS_THRESHOLD;

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnSleep(UPrimitiveComponent* SleepingComponent, FName BoneName);

	//UFUNCTION()
	//void OnTakeRadialDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* InstigatedBy, AActor* DamageCauser);

public:

	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser);

	FString GetSandboxNetUid() const;

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

	void EnablePhysics();
    
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

//TODO
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

	UPROPERTY(EditAnywhere, Category = "Sandbox")
	TMap<FString, float> AffectParamMap;

	virtual float GetAffectParam(const FString& ParamName) const;

	int GetSandboxTypeId() const override;

	void GetFootPose(FRotator& LeftFootRotator, FRotator& RightFootRotator);

};


class UNREALSANDBOXTOOLKIT_API ASandboxObjectUtils {

public:

	static TArray<ASandboxObject*> GetContainerContent(AActor* AnyActor, const FString& Name);
};

UCLASS()
class UNREALSANDBOXTOOLKIT_API USandboxDamageType : public UDamageType {
	GENERATED_UCLASS_BODY()


public:

	UPROPERTY(EditAnywhere, Category = "Sandbox")
	float FireDamageFactor = 1.f;

	UPROPERTY(EditAnywhere, Category = "Sandbox")
	float ExplosionDamageFactor = 1.f;

	UPROPERTY(EditAnywhere, Category = "Sandbox")
	float HitDamageFactor = 1.f;

};
