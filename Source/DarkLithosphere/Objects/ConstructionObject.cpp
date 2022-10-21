#include "ConstructionObject.h"


int GetTargetClassId(const AActor* Actor) {
	const ASandboxObject* TargetObject = Cast<ASandboxObject>(Actor);
	if (TargetObject) {
		return TargetObject->GetSandboxClassId();;
	}

	return 0;
}

void TransformMode0(FVector& NewLocation, FRotator& NewRotation, const FHitResult& TraceResult) {
	NewLocation = TraceResult.Location;
}

void TransformMode1(FVector& NewLocation, FRotator& NewRotation, const AActor* TargetObject, const FHitResult& TraceResult) {
	FVector TargetObjLocation = TargetObject->GetActorLocation();
	FVector CursorLocation = TraceResult.Location;

	NewRotation = NewRotation = TargetObject->GetActorRotation();
	const FRotator R = TargetObject->GetActorRotation().GetInverse();
	const FVector Test = R.RotateVector(TargetObjLocation - CursorLocation);

	FRotator ExtRotation = FRotator(0, 0, 0);
	if (FMath::Abs(Test.X) > FMath::Abs(Test.Y)) {
		if (Test.X > 0) {
			ExtRotation = FRotator(0, 90, 0);
		}

		if (Test.X <= 0) {
			ExtRotation = FRotator(0, -90, 0);
		}
	} else {
		if (Test.Y > 0) {
			ExtRotation = FRotator(0, 180, 0);
		}

		if (Test.Y <= 0) {
			ExtRotation = FRotator(0, 0, 0);
		}
	}

	NewRotation += ExtRotation;
	NewLocation = TargetObjLocation;
}

void TransformMode2(FVector& NewLocation, FRotator& NewRotation, const AActor* TargetObject) {
	NewLocation = TargetObject->GetActorLocation();
	NewLocation.Z += 300;
	NewRotation = TargetObject->GetActorRotation();
}

void TransformMode3(FVector& NewLocation, FRotator& NewRotation, const AActor* TargetObject, const FHitResult& TraceResult) {
	TransformMode1(NewLocation, NewRotation, TargetObject, TraceResult);
	NewLocation.Z += 300;
}

void TransformMode4(FVector& NewLocation, FRotator& NewRotation, const AActor* TargetObject, const FHitResult& TraceResult) {
	const FVector TargetObjLocation = TargetObject->GetActorLocation();
	const FVector CursorLocation = TraceResult.Location;

	NewLocation = TargetObjLocation;
	NewRotation = TargetObject->GetActorRotation();

	FVector Offset(0, 0, 0);

	const FRotator R = TargetObject->GetActorRotation().GetInverse();
	const FVector Test = R.RotateVector(TargetObjLocation - CursorLocation);
	if (FMath::Abs(Test.X) > FMath::Abs(Test.Y)) {
		if (Test.X > 0) {
			Offset.X = -300;
		}

		if (Test.X <= 0) {
			Offset.X = 300;
		}
	} else {
		if (Test.Y > 0) {
			Offset.Y = -300;
		}
		 
		if (Test.Y <= 0) {
			Offset.Y = 300;
		}
	}

	Offset = NewRotation.RotateVector(Offset);
	NewLocation += Offset;
}

void TransformMode5(FVector& NewLocation, FRotator& NewRotation, const AActor* TargetObject, const FHitResult& TraceResult) {
	NewLocation = TargetObject->GetActorLocation();
	NewRotation = TargetObject->GetActorRotation();
}

const UActorComponent* FindBPComponentsByName(const TSubclassOf<AActor> InActorClass, FString Name) {
	// Cast the actor class to a UBlueprintGeneratedClass
	const UBlueprintGeneratedClass* ActorBlueprintGeneratedClass = Cast<UBlueprintGeneratedClass>(InActorClass);

	// Use UBrintGeneratedClass->SimpleConstructionScript->GetAllNodes() to get an array of USCS_Nodes
	const TArray<USCS_Node*>& ActorBlueprintNodes = ActorBlueprintGeneratedClass->SimpleConstructionScript->GetAllNodes();

	// Iterate through the array looking for the USCS_Node whose ComponentClass matches the component you're looking for
	for (USCS_Node* Node : ActorBlueprintNodes) {
		const UActorComponent* Component = Cast<UActorComponent>(Node->ComponentTemplate);
		if (Component) {
			FName NodeName = Node->GetVariableName();
			if (NodeName.ToString() == Name) {
				return Component;
			}
		}
	}

	return nullptr;
}

bool TestNewPosition(const TSubclassOf<ASandboxObject> SubclassSandboxObject, const UWorld* World, const FVector& Pos, const FRotator& Rotator) {
	const UActorComponent* ActorComponent = FindBPComponentsByName(SubclassSandboxObject, "TestVolume");
	const UBoxComponent* Box = Cast<UBoxComponent>(ActorComponent);

	if (Box) {
		FTransform Transform(Rotator);
		FBoxSphereBounds BoxSphereBounds = Box->CalcBounds(FTransform(Rotator, Pos));
		BoxSphereBounds.Origin += Transform.TransformPosition(Box->GetRelativeLocation());

		//DrawDebugBox(World, BoxSphereBounds.Origin, (BoxSphereBounds.BoxExtent - 5), FColor::Purple, false, 1, 0, 1);

		TArray<FOverlapResult> OverlapArray;
		World->OverlapMultiByChannel(OverlapArray, BoxSphereBounds.Origin, FQuat::Identity, ECC_WorldDynamic, FCollisionShape::MakeBox(BoxSphereBounds.BoxExtent - 5));
		for (auto& Overlap : OverlapArray) {
			AActor* Actor = Overlap.GetActor();
			auto* Component = Overlap.GetComponent();

			UE_LOG(LogTemp, Warning, TEXT("Overlap actor -> %s"), *Actor->GetClass()->GetName());
			if (Component) {
				UE_LOG(LogTemp, Warning, TEXT("Overlap actor -> %s"), *Component->GetName());
			}
			
			if (Actor->GetClass()->GetName() == "BP_Terrain_C") {
				continue;
			}

			return false;

			//ASandboxObject* SandboxObject = Cast<ASandboxObject>(Overlap.Actor.Get());
			//if (SandboxObject) {
				//ASandboxTerrainController* Terrain = Cast<ASandboxTerrainController>(Overlap.Actor.Get());

			//}
		}
	}

	return true;
}

bool AConstructionObject::PlaceToWorldClcPosition(const UWorld* World, const FVector& SourcePos, const FRotator& SourceRotation, const FHitResult& TraceResult, FVector& NewLocation, FRotator& NewRotation, bool bFinal) const {
	NewLocation = TraceResult.Location;

	uint64 TargetObjClassId = GetTargetClassId(TraceResult.GetActor());
	AActor* TargetObject = TraceResult.GetActor();
	AConstructionObject* TargetConstruction = Cast<AConstructionObject>(TargetObject);

	if (TargetConstruction) {
		auto TargetConstructionType = TargetConstruction->ConstructionType;

		if (ConstructionType == EConstructionType::Door) {
			if (TargetConstruction->GetSandboxClassId() == 114) { // FIXME
				TransformMode5(NewLocation, NewRotation, TargetObject, TraceResult);
				return true;
			}
		}

		if (ConstructionType == EConstructionType::Ramp) {
			if (TargetConstructionType == EConstructionType::Foundation) {
				TransformMode1(NewLocation, NewRotation, TargetObject, TraceResult);
				return true;
			}

			if (TargetConstructionType == EConstructionType::Ceiling) {
				TransformMode3(NewLocation, NewRotation, TargetObject, TraceResult);
				return true;
			}
		}

		if (ConstructionType == EConstructionType::Ceiling) {
			if (TargetConstructionType == EConstructionType::Wall) {
				TransformMode5(NewLocation, NewRotation, TargetObject, TraceResult);
				return true;
			}

			if (TargetConstructionType == EConstructionType::Ceiling) {
				TransformMode4(NewLocation, NewRotation, TargetObject, TraceResult);
				return true;
			}
		}

		if (ConstructionType == EConstructionType::Wall) {
			if (TargetConstructionType == EConstructionType::Foundation) {
				TransformMode1(NewLocation, NewRotation, TargetObject, TraceResult);
				return true;
			}

			if (TargetConstructionType == EConstructionType::Wall) {
				TransformMode2(NewLocation, NewRotation, TargetObject);
				return true;
			}

			if (TargetConstructionType == EConstructionType::Ceiling) {
				TransformMode3(NewLocation, NewRotation, TargetObject, TraceResult);
				return true;
			}
		}

		if (ConstructionType == EConstructionType::Foundation) {
			if (TargetConstructionType == EConstructionType::Foundation) {
				TransformMode4(NewLocation, NewRotation, TargetObject, TraceResult);
				return TestNewPosition(GetClass(), World, NewLocation, NewRotation);
			}

			NewRotation = SourceRotation;

			AActor* Actor = TraceResult.GetActor();
			FString Name = Actor->GetClass()->GetName();
			UE_LOG(LogTemp, Warning, TEXT("Hit: %s"), *Name);

			if (Name == "Landscape" || Name == "BP_Terrain_C") {
				TransformMode0(NewLocation, NewRotation, TraceResult);
				return true;
			}
		}
	} else {
		if (ConstructionType == EConstructionType::Foundation) {
			NewRotation = SourceRotation;
			FString Name = TargetObject->GetClass()->GetName();
			//UE_LOG(LogTemp, Warning, TEXT("Hit: %s"), *Name);

			if (Name == "Landscape" || Name == "BP_Terrain_C") {
				TransformMode0(NewLocation, NewRotation, TraceResult);
				return true;
			}
		}
	}

	return false;

}

bool AConstructionObject::CanTake(const AActor* actor) const {
	return false;
}

void AConstructionObject::OnTerrainChange() {
	// do nothing
}