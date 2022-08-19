
#include "Clothing.h"


int AClothing::GetSandboxTypeId() const {
	return 500;
}

void AClothing::GetFootPose(FRotator& LeftFootRotator, FRotator& RightFootRotator) {
		LeftFootRotator = FootRotator;
		RightFootRotator = FootRotator;
}