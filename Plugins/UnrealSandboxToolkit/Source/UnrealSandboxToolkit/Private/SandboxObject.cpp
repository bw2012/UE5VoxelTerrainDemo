

#include "SandboxObject.h"
#include "SandboxLevelController.h"
#include "Net/UnrealNetwork.h"


ASandboxObject::ASandboxObject() {
	PrimaryActorTick.bCanEverTick = true;
	SandboxRootMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SandboxRootMesh"));
	MaxStackSize = 100;
	bStackable = true;
	RootComponent = SandboxRootMesh;
	bReplicates = true;
	SandboxNetUid = 0;
	bCanPlaceSandboxObject = true;
	SandboxRootMesh->SetLinearDamping(1.f);
	SandboxRootMesh->SetAngularDamping(10.f);
	SandboxRootMesh->BodyInstance.bGenerateWakeEvents = true;
}

static const FString DefaultSandboxObjectName = FString(TEXT("Sandbox object"));

void ASandboxObject::BeginPlay() {
	Super::BeginPlay();
	SandboxRootMesh->OnComponentSleep.AddDynamic(this, &ASandboxObject::OnSleep);
	//SandboxRootMesh->OnTakeRadialDamage.AddDynamic(this, &ASandboxObject::OnTakeRadialDamage);
	//SandboxRootMesh->OnTakeAnyDamage.AddDynamic(this, &ASandboxObject::OnTakeRadialDamage);
	//SandboxRootMesh->OnTakeAnyDamage.AddDynamic(this, &ASandboxObject::OnTakeRadialDamage);
}

void ASandboxObject::OnSleep(UPrimitiveComponent* SleepingComponent, FName BoneName) {
	//UE_LOG(LogTemp, Warning, TEXT("OnSleep"));
	SandboxRootMesh->SetSimulatePhysics(false);
}

float ASandboxObject::TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser) {
	float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
	
	if (DamageEnablePhysThreshold > 0 && ActualDamage > DamageEnablePhysThreshold) {
		EnablePhysics();
	}

	return ActualDamage;
}

//void ASandboxObject::OnTakeRadialDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* InstigatedBy, AActor* DamageCauser) {
//}

FString ASandboxObject::GetSandboxName() {
	return DefaultSandboxObjectName;
}

FString ASandboxObject::GetSandboxNetUid() const {
	return SandboxNetUid;
}

uint64 ASandboxObject::GetSandboxClassId() const {
	return SandboxClassId; 
}

int ASandboxObject::GetSandboxTypeId() const {
	return 0; 
}

int ASandboxObject::GetMaxStackSize() {
	if (!bStackable) {
		return 1;
	}

	return MaxStackSize; // default stack size
}

UTexture2D* ASandboxObject::GetSandboxIconTexture() {
	return NULL; 
}

void ASandboxObject::TickInInventoryActive(float DeltaTime, UWorld* World, const FHitResult& HitResult) {

}

void ASandboxObject::ActionInInventoryActive(UWorld* World, const FHitResult& HitResult) {

}

bool ASandboxObject::CanTake(const AActor* Actor) const {

	if (SandboxRootMesh->IsSimulatingPhysics()) {
		return false;
	}
	
	TArray<UContainerComponent*> ContainerComponents;
	GetComponents<UContainerComponent>(ContainerComponents);

	for (const auto& ContainerComponent : ContainerComponents) {
		if (!ContainerComponent->IsEmpty()) {
			return false;
		}
	}
	
	return true; 
}

void ASandboxObject::OnTerrainChange() {
	SandboxRootMesh->SetSimulatePhysics(true);
}

UContainerComponent* ASandboxObject::GetContainer(const FString& Name) { 
	return GetFirstComponentByName<UContainerComponent>(Name);
}

bool ASandboxObject::IsInteractive(const APawn* Source) {
	return false; // no;
}

void ASandboxObject::MainInteraction(const APawn* Source) {
	// do nothing
}

void ASandboxObject::SetProperty(FString Key, FString Value) {
	PropertyMap.Add(Key, Value);
}

FString ASandboxObject::GetProperty(FString Key) const {
	return PropertyMap.FindRef(Key);
}

void ASandboxObject::RemoveProperty(FString Key) {
	PropertyMap.Remove(Key);
}

void ASandboxObject::PostLoadProperties() {

}

void ASandboxObject::OnPlaceToWorld() {

}

bool ASandboxObject::PlaceToWorldClcPosition(const UWorld* World, const FVector& SourcePos, const FRotator& SourceRotation, const FHitResult& Res, FVector& Location, FRotator& Rotation, bool bFinal) const {
	Location = Res.Location;
	Rotation.Pitch = 0;
	Rotation.Roll = 0;
	Rotation.Yaw = SourceRotation.Yaw;
	return true;
}

void ASandboxObject::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const {
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ASandboxObject, SandboxNetUid);
}

const UStaticMeshComponent* ASandboxObject::GetMarkerMesh() const {
	return SandboxRootMesh;
}

void ASandboxObject::EnablePhysics() {
	if (SandboxRootMesh) {
		SandboxRootMesh->SetSimulatePhysics(true);
	}
}

int ASandboxSkeletalModule::GetSandboxTypeId() const {
	return 500;
}

void ASandboxSkeletalModule::GetFootPose(FRotator& LeftFootRotator, FRotator& RightFootRotator) {
	LeftFootRotator = FootRotator;
	RightFootRotator = FRotator(-FootRotator.Pitch, -FootRotator.Yaw, FootRotator.Roll);
}


float ASandboxSkeletalModule::GetAffectParam(const FString& ParamName) const {
	if (AffectParamMap.Contains(ParamName)) {
		return AffectParamMap[ParamName];
	}

	return 1;
}


template<class T>
T* GetFirstComponentByName(AActor* Actor, FString ComponentName) {
	TArray<T*> Components;
	Actor->GetComponents<T>(Components);
	for (T* Component : Components) {
		if (Component->GetName() == ComponentName)
			return Component;
	}

	return nullptr;
}

TArray<ASandboxObject*> ASandboxObjectUtils::GetContainerContent(AActor* AnyActor, const FString& Name) {
	TArray<ASandboxObject*> Result;

	UContainerComponent* Container = GetFirstComponentByName<UContainerComponent>(AnyActor, Name);
	if (Container) {
		TArray<uint64> ObjList = Container->GetAllObjects();
		for (uint64 ClassId : ObjList) {
			ASandboxObject* Obj = ASandboxLevelController::GetDefaultSandboxObject(ClassId);
			if (Obj) {
				Result.Add(Obj);
			}
		}
	}

	return Result;
}


USandboxDamageType::USandboxDamageType(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {

}