#pragma once

#include "Engine.h"
#include "GameFramework/Actor.h"
#include "SandboxObjectMap.h"
#include "SandboxLevelController.generated.h"

USTRUCT()
struct FTempContainerStack {
	GENERATED_BODY()

	UPROPERTY()
	int SlotId;

	UPROPERTY()
	FContainerStack Stack;
};


USTRUCT()
struct UNREALSANDBOXTOOLKIT_API FSandboxObjectDescriptor {
	GENERATED_BODY()

	UPROPERTY()
	FTransform Transform;

	UPROPERTY()
	int ClassId;

	UPROPERTY()
	int TypeId;

	UPROPERTY()
	TMap<FString, FString> PropertyMap;

	UPROPERTY()
	TArray<FTempContainerStack> Container;

	static FSandboxObjectDescriptor MakeObjDescriptor(ASandboxObject* SandboxObject);
};


UCLASS()
class UNREALSANDBOXTOOLKIT_API ASandboxLevelController : public AActor {
	GENERATED_BODY()

public:
	ASandboxLevelController();

	virtual void BeginPlay() override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(EditAnywhere, Category = "UnrealSandbox Toolkit")
	FString MapName;

	UPROPERTY(EditAnywhere, Category = "UnrealSandbox Toolkit")
	USandboxObjectMap* ObjectMap;

	//TODO remove
	TSubclassOf<ASandboxObject> GetSandboxObjectByClassId(int32 ClassId);

	ASandboxObject* GetSandboxObject(uint64 ClassId);

	static ASandboxObject* GetDefaultSandboxObject(uint64 ClassId);

	static ASandboxLevelController* GetInstance();

	void PrepareMetaData();

	virtual ASandboxObject* SpawnSandboxObject(const int ClassId, const FTransform& Transform);

	virtual bool RemoveSandboxObject(ASandboxObject* Obj);

	ASandboxObject* GetObjectByNetUid(uint64 NetUid);

	ASandboxObject* SpawnPreparedObject(const FSandboxObjectDescriptor& ObjDesc);

	void SpawnEffect(const int32 EffectId, const FTransform& Transform);

protected:

	TMap<FString, TSubclassOf<ASandboxObject>> ObjectMapByClassName;

	//TMap<uint64, TSubclassOf<ASandboxObject>> ObjectMapById;

	virtual void SaveLevelJson();

	void SaveLevelYaml();

	virtual void SaveLevelJsonExt(TSharedRef<TJsonWriter<TCHAR>> JsonWriter);

	virtual void LoadLevelJson();

	virtual void LoadLevelJsonExt(TSharedPtr<FJsonObject> JsonParsed);

	void SaveObject(TSharedRef <TJsonWriter<TCHAR>> JsonWriter, const FSandboxObjectDescriptor& ObjDesc);

	virtual void SpawnPreparedObjects(const TArray<FSandboxObjectDescriptor>& ObjDescList);

	void SavePreparedObjects(const TArray<FSandboxObjectDescriptor>& ObjDescList);

	virtual void PrepareObjectForSave(TArray<FSandboxObjectDescriptor>& ObjDescList);

private:

	bool bIsMetaDataReady = false;

	static TMap<int32, TSubclassOf<ASandboxObject>> ObjectMapById;

	uint64 ObjectCounter;

	TMap<uint64, ASandboxObject*> GlobalObjectMap;

	static ASandboxLevelController* StaticSelf;

};