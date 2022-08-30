
#include "Furnace.h"
#include "../LevelController.h"

AFurnace::AFurnace() {
	PrimaryActorTick.bCanEverTick = true;
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
	Receipe.RawMatType = 0;
	Receipe.ProductClass = 23;
	Receipe.ProductType = 0;
	Receipe.ProcessingTime = 10;

	FurnaceReceipMap.Add(1, Receipe);
}

bool AFurnace::CanTake(AActor* actor) {
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

void AFurnace::Tick(float DeltaTime) {
	Super::Tick(DeltaTime);

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
					//fix1111
					/*
					auto* Obj = Stack->GetObject();
					if (Obj) {
						uint64 ClassId = Obj->GetSandboxClassId();
						if (ClassId == 12 || ClassId == 18) {
							Container->DecreaseObjectsInContainer(FuelSlot, 1);
							Lifetime += 10;

							if (!bIsActive) {
								auto* Mesh = GetFirstComponentByName<UStaticMeshComponent>(TEXT("RockMesh"));
								if (Mesh) {
									Mesh->SetVisibility(true, true);
								}

								auto* Audio = GetFirstComponentByName<UAudioComponent>(TEXT("Audio"));
								if (Audio) {
									Audio->SetVolumeMultiplier(1);
								}

								bIsActive = true;
							}

							bHasFuel = true;

						}
					}
					*/
				} 

				if(!bHasFuel){
					if (bIsActive) {
						auto* Mesh = GetFirstComponentByName<UStaticMeshComponent>(TEXT("RockMesh"));
						if (Mesh) {
							Mesh->SetVisibility(false, true);
						}

						auto* Audio = GetFirstComponentByName<UAudioComponent>(TEXT("Audio"));
						if (Audio) {
							Audio->SetVolumeMultiplier(0);
						}

						bIsActive = false;
					}
				}
			} else {
				// doing job
				const float ProcessFinish = 10;
				if (CurrentReceipeId == 0) {

					// TODO fix
					/*
					ASandboxObject* RawMatObj = Container->GetAvailableSlotObject(RawMaterialSlot);
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
					*/
				} else {
					if (ProcessTime < ProcessFinish) {
						ProcessTime += Delta;
					} else {
						bool bSuccess = false;
						auto& Receipe = FurnaceReceipMap[CurrentReceipeId];

						// TODO fix1111
						/*
						ASandboxObject* Product = Container->GetAvailableSlotObject(ProductSlot1);
						if (Product) {
							if (Product->GetSandboxClassId() == Receipe.ProductClass) {
								Container->ChangeAmount(ProductSlot1, 1);
								bSuccess = true;
							}
						} 
						*/

						if (!bSuccess) {
							auto* ProductStack1 = Container->GetSlot(ProductSlot1);
							if (!ProductStack1 || ProductStack1->Amount == 0) {
								ALevelController* Controller = GetLevelController(GetWorld());
								if (Controller) {
									TSubclassOf<ASandboxObject> NewObjS = Controller->GetSandboxObjectByClassId(Receipe.ProductClass); // TODO type Id
									if (NewObjS) {
										FContainerStack NewStack;
										NewStack.Amount = 1;
										// TODO fix1111
										//NewStack.ObjectClass = NewObjS;

										Container->SetStackDirectly(NewStack, ProductSlot1);
										bSuccess = true;
									}
								}
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