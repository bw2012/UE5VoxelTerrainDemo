
#include "StorageBox.h"



bool AStorageBox::IsContainer() {
	return true;
}

bool AStorageBox::PlaceToWorldClcPosition(const UWorld* World, const FVector& SourcePos, const FRotator& SourceRotation, const FHitResult& Res, FVector& Location, FRotator& Rotation, bool bFinal) const {
	Super::PlaceToWorldClcPosition(World, SourcePos, SourceRotation, Res, Location, Rotation, bFinal);

	auto* Component = Res.GetComponent();
	if (Component) {
		FString ComponentName = Component->GetClass()->GetName();
		if (ComponentName == "TerrainInstancedStaticMesh") {
			return false;
		}
	}

	if (Res.Normal.Z < 0.45) {
		return false;
	}

	return true;

	return true;
}

