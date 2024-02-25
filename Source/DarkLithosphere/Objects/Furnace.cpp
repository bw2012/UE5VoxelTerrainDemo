
#include "Furnace.h"
#include "../LevelController.h"
#include "Net/UnrealNetwork.h"


AFurnace::AFurnace() {
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	Lifetime = 0;
	bIsActive = false;
	ProcessTime = 0;
	CurrentReceipeId = 0;
}

void AFurnace::BeginPlay() {
	Super::BeginPlay();

	Timestamp = FPlatformTime::Seconds();

	Lifetime = 0;
	const auto& Param = GetProperty(TEXT("Lifetime"));
	if (Param != "") {
		Lifetime = FCString::Atof(*Param);
	}

	FFurnaceReceipe Receipe;
	Receipe.RawMatClass = 22;
	Receipe.ProductClass = 23;
	Receipe.ProcessingTime = 10;

	FurnaceReceipMap.Add(1, Receipe);
}

bool AFurnace::CanTake(const AActor* Actor) const {
	return false;
}

bool AFurnace::IsContainer() {
	return true;
}

FName AFurnace::GetContainerWidgetName() {
	return TEXT("furnace");
}

void AFurnace::PostLoadProperties() {
	const auto& Param = GetProperty(TEXT("Lifetime"));
	if (Param != "") {
		Lifetime = FCString::Atof(*Param);
	}
}

int FindReceipe(const TMap<int, FFurnaceReceipe>& FurnaceReceipMap, int ClassId) {
	for (const auto& Itm : FurnaceReceipMap) {
		int ReceipeId = Itm.Key;
		FFurnaceReceipe R = Itm.Value;
		if (R.RawMatClass == ClassId) {
			return ReceipeId;
		}
	}

	return -1;
}

ALevelController* GetLevelController(UWorld* World) {
	for (TActorIterator<ALevelController> ActorItr(World); ActorItr; ++ActorItr) {
		return *ActorItr;
		ALevelController* Controller = Cast<ALevelController>(*ActorItr);
		if (Controller) {
			return Controller;
		}
	}

	return nullptr;
}

const ASandboxObject* GetStackObject(UContainerComponent* Container, int SlotId) {
	auto* Stack = Container->GetSlot(SlotId);
	if (Stack) {
		return ASandboxLevelController::GetDefaultSandboxObject(Stack->SandboxClassId);
	}

	return nullptr;
}

void AFurnace::Tick(float DeltaTime) {
	Super::Tick(DeltaTime);

	if (GetNetMode() == NM_Client) {
		return;
	}

	ServerPerform();
}

void AFurnace::OnRep_State() {
	if (ServerState != LocalState) {

		if (ServerState > 0) {
			SetActive();
		} else {
			SetInactive();
		}

		LocalState = ServerState;
	}
}

void AFurnace::SetInactive() {
	auto* Mesh = GetFirstComponentByName<UStaticMeshComponent>(TEXT("RockMesh"));
	if (Mesh) {
		Mesh->SetVisibility(false, true);
	}

	auto* Audio = GetFirstComponentByName<UAudioComponent>(TEXT("Audio"));
	if (Audio) {
		Audio->SetVolumeMultiplier(0);
	}
}

void AFurnace::SetActive() {
	auto* Mesh = GetFirstComponentByName<UStaticMeshComponent>(TEXT("RockMesh"));
	if (Mesh) {
		Mesh->SetVisibility(true, true);
	}

	auto* Audio = GetFirstComponentByName<UAudioComponent>(TEXT("Audio"));
	if (Audio) {
		Audio->SetVolumeMultiplier(1);
	}
}

void AFurnace::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const {
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AFurnace, ServerState);
}

void AFurnace::ServerPerform() {
	static const int FuelSlot = 0;
	static const int RawMaterialSlot = 1;
	static const int ProductSlot1 = 2;

	double T = FPlatformTime::Seconds();
	double Delta = T - Timestamp;
	if (Delta > 1) {
		Timestamp = T;
		if (Lifetime > 0) {
			Lifetime -= Delta;
		}

		//UE_LOG(LogTemp, Warning, TEXT("Lifetime: %f"), Lifetime);
		//UE_LOG(LogTemp, Warning, TEXT("ProcessTime: %f"), ProcessTime);

		UContainerComponent* Container = GetContainer(TEXT("ObjectContainer"));
		if (Container) {
			if (Lifetime <= 0) {
				auto* Stack = Container->GetSlot(FuelSlot);
				bool bHasFuel = false;
				if (Stack && Stack->Amount > 0) {
					auto* Obj = ASandboxLevelController::GetDefaultSandboxObject(Stack->SandboxClassId);
					if (Obj) {
						uint64 ClassId = Obj->GetSandboxClassId();
						if (ClassId == 12 || ClassId == 18) {
							Container->DecreaseObjectsInContainer(FuelSlot, 1);
							Lifetime += 20;

							if (!bIsActive) {
								SetActive();
								ServerState = 1;
								bIsActive = true;
							}

							bHasFuel = true;
						}
					}
				}

				if (!bHasFuel) {
					if (bIsActive) {
						SetInactive();
						ServerState = 0;
						bIsActive = false;
					}
				}
			} else {
				// doing job
				const float ProcessFinish = 10;
				if (CurrentReceipeId == 0) {
					const ASandboxObject* RawMatObj = GetStackObject(Container, RawMaterialSlot);
					if (RawMatObj) {
						uint64 ClassId = RawMatObj->GetSandboxClassId();
						UE_LOG(LogTemp, Warning, TEXT("FindReceipe: %d"), ClassId);
						int ReceipeId = FindReceipe(FurnaceReceipMap, ClassId);
						if (ReceipeId > 0) {
							// start job
							UE_LOG(LogTemp, Warning, TEXT("ReceipeId: %d "), ReceipeId);
							ProcessTime = 0;
							CurrentReceipeId = ReceipeId;
						}
					}
				} else {
					if (ProcessTime < ProcessFinish) {
						ProcessTime += Delta;
					} else {
						bool bSuccess = false;
						auto& Receipe = FurnaceReceipMap[CurrentReceipeId];

						const ASandboxObject* Product = GetStackObject(Container, ProductSlot1);
						if (Product) {
							if (Product->GetSandboxClassId() == Receipe.ProductClass) {
								Container->ChangeAmount(ProductSlot1, 1);
								bSuccess = true;
							}
						}

						if (!bSuccess) {
							auto* ProductStack1 = Container->GetSlot(ProductSlot1);
							if (!ProductStack1 || ProductStack1->Amount == 0) {
								FContainerStack NewStack;
								NewStack.Amount = 1;
								NewStack.SandboxClassId = Receipe.ProductClass;
								Container->SetStackDirectly(NewStack, ProductSlot1);
								bSuccess = true;
							}
						}

						if (bSuccess) {
							Container->DecreaseObjectsInContainer(RawMaterialSlot, 1);
							CurrentReceipeId = 0;
							ProcessTime = 0;
						}
					}
				}
			}
		}
	}
}

bool AFurnace::IsZoneAnchor() {
	return true;
}