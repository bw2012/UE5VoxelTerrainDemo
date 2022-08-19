
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

	UPROPERTY(EditAnywhere, Category = "Sandbox")
	bool bStackable;

	UPROPERTY(EditAnywhere, Category = "Sandbox")
	uint32 MaxStackSize;

	UPROPERTY(EditAnywhere, Category = "Sandbox")
	TMap<FString, FString> PropertyMap;

public:

	virtual FString GetSandboxName();

	virtual uint64 GetSandboxClassId() const;

	virtual int GetSandboxTypeId() const;

	virtual int GetMaxStackSize();
	
	virtual UTexture2D* GetSandboxIconTexture();

	virtual void TickInInventoryActive(float DeltaTime, UWorld* World, const FHitResult& HitResult);

	virtual void ActionInInventoryActive(UWorld* World, const FHitResult& HitResult);

	virtual void ActionInInventoryActive2(UWorld* World, const FHitResult& HitResult);

	virtual bool IsInteractive(const APawn* Source = nullptr);

	virtual void MainInteraction(const APawn* Source = nullptr);

	virtual bool CanTake(AActor* actor);

	virtual void InformTerrainChange(int32 item);
    
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

	FString GetProperty(FString Key);

	void RemoveProperty(FString Key);

	virtual void PostLoadProperties();

	virtual void OnPlaceToWorld();

	virtual bool PlaceToWorldClcPosition(const FVector& SourcePos, const FRotator& SourceRotation, const FHitResult& Res, FVector& Location, FRotator& Rotation, bool bFinal = false) const;

};


class IWearable {

public:



};