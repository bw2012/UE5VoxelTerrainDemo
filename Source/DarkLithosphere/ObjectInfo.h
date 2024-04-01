#pragma once


#include "Engine.h"
#include "Engine/DataTable.h"
#include "ObjectInfo.generated.h"



UENUM(BlueprintType)
enum class FTerrainObjectMiningMode : uint8 {
	None = 0	UMETA(DisplayName = "None"),
	Spawn = 1	UMETA(DisplayName = "Spawn to world"),
	Inventory = 2	UMETA(DisplayName = "Add to inventory")
};


USTRUCT(BlueprintType)
struct DARKLITHOSPHERE_API FCoreObjectInfo : public FTableRowBase {
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Sandbox")
	FString CommonName;

	UPROPERTY(EditAnywhere, Category = "Sandbox")
	FString ChemicalFormula; //Fe2O3 Fe₂O₃

	UPROPERTY(EditAnywhere, Category = "Sandbox")
	FString CategoryName;

	UPROPERTY(EditAnywhere, Category = "Sandbox")
	FString Description;

	UPROPERTY(EditAnywhere, Category = "Sandbox")
	uint32 CrashEffectId = 0;

	UPROPERTY(EditAnywhere, Category = "Sandbox")
	bool bMiningDestroy = true;

	UPROPERTY(EditAnywhere, Category = "Sandbox")
	uint32 MiningCrashEffectId = 0;
};

USTRUCT(BlueprintType)
struct DARKLITHOSPHERE_API FObjectInfo : public FCoreObjectInfo {
	GENERATED_BODY()

};

/**
 * 
 */
USTRUCT(BlueprintType)
struct DARKLITHOSPHERE_API FTerrainObjectInfo : public FCoreObjectInfo {
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Sandbox")
	bool bCanTake = false;

	UPROPERTY(EditAnywhere, Category = "Sandbox")
	uint64 SandboxObjectId = 0; 

	UPROPERTY(EditAnywhere, Category = "Sandbox")
	FTerrainObjectMiningMode MiningMode;
};

