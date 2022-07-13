
#include "BaseObject.h"
//#include "SpawnHelper.h"


bool ABaseObject::VisibleInHand(FTransform& Transform) {
	return false;
}

bool ABaseObject::IsContainer() {
	return false;
}

FName ABaseObject::GetContainerWidgetName() {
	return TEXT("container");
}
