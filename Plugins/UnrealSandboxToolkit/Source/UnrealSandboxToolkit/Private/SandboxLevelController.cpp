

#include "SandboxLevelController.h"
#include "SandboxObject.h"
#include "ContainerComponent.h"

#include <string>
#include <vector>

TMap<int32, TSubclassOf<ASandboxObject>> ASandboxLevelController::ObjectMapById;
ASandboxLevelController* ASandboxLevelController::StaticSelf;

ASandboxLevelController::ASandboxLevelController() {
	MapName = TEXT("World 0");
	ObjectCounter = 1;
}

void ASandboxLevelController::BeginPlay() {
	Super::BeginPlay();
	ObjectCounter = 1;
	GlobalObjectMap.Empty();
	PrepareMetaData();

	if (!StaticSelf) {
		// TODO warning
	}

	StaticSelf = this;
}

void ASandboxLevelController::EndPlay(const EEndPlayReason::Type EndPlayReason) {
	Super::EndPlay(EndPlayReason);
	StaticSelf = nullptr;
}

void ASandboxLevelController::Tick(float DeltaTime) {
	Super::Tick(DeltaTime);
}

void ASandboxLevelController::SaveObject(TSharedRef <TJsonWriter<TCHAR>> JsonWriter, const FSandboxObjectDescriptor& ObjDesc) {
	JsonWriter->WriteObjectStart();
	JsonWriter->WriteObjectStart("Object");

	//JsonWriter->WriteValue("Name", SandboxObjectName);
	//JsonWriter->WriteValue("Class", Name);
	JsonWriter->WriteValue("ClassId", ObjDesc.ClassId);
	JsonWriter->WriteValue("TypeId", ObjDesc.TypeId);

	JsonWriter->WriteArrayStart("Location");
	JsonWriter->WriteValue(ObjDesc.Transform.GetLocation().X);
	JsonWriter->WriteValue(ObjDesc.Transform.GetLocation().Y);
	JsonWriter->WriteValue(ObjDesc.Transform.GetLocation().Z);
	JsonWriter->WriteArrayEnd();

	JsonWriter->WriteArrayStart("Rotation");
	JsonWriter->WriteValue(ObjDesc.Transform.GetRotation().Rotator().Pitch);
	JsonWriter->WriteValue(ObjDesc.Transform.GetRotation().Rotator().Yaw);
	JsonWriter->WriteValue(ObjDesc.Transform.GetRotation().Rotator().Roll);
	JsonWriter->WriteArrayEnd();

	JsonWriter->WriteArrayStart("Scale");
	JsonWriter->WriteValue(ObjDesc.Transform.GetScale3D().X);
	JsonWriter->WriteValue(ObjDesc.Transform.GetScale3D().Y);
	JsonWriter->WriteValue(ObjDesc.Transform.GetScale3D().Z);
	JsonWriter->WriteArrayEnd();

	if (ObjDesc.Container.Num() > 0) {
		JsonWriter->WriteArrayStart("Containers");

		JsonWriter->WriteObjectStart();
		JsonWriter->WriteObjectStart("Container");

		FString Name = GetName();
		JsonWriter->WriteValue("Name", TEXT("Container"));

		JsonWriter->WriteArrayStart("Content");

		for (const auto& TmpStack : ObjDesc.Container) {
			if (TmpStack.Stack.Amount > 0) {
				JsonWriter->WriteObjectStart();

				const ASandboxObject* SandboxObject = Cast<ASandboxObject>(TmpStack.Stack.GetObject());
				FString ClassName = SandboxObject->GetClass()->GetName();
				JsonWriter->WriteValue("SlotId", TmpStack.SlotId);
				JsonWriter->WriteValue("Class", ClassName);
				JsonWriter->WriteValue("ClassId", (int64)SandboxObject->GetSandboxClassId());
				JsonWriter->WriteValue("TypeId", SandboxObject->GetSandboxTypeId());
				JsonWriter->WriteValue("Amount", TmpStack.Stack.Amount);

				JsonWriter->WriteObjectEnd();
			}
		}
		JsonWriter->WriteArrayEnd();

		JsonWriter->WriteObjectEnd();
		JsonWriter->WriteObjectEnd();

		JsonWriter->WriteArrayEnd();
	}

	if (ObjDesc.PropertyMap.Num() > 0) {
		JsonWriter->WriteObjectStart("Properties");
		for (auto& Itm : ObjDesc.PropertyMap) {
			JsonWriter->WriteValue(Itm.Key, Itm.Value);
		}
		JsonWriter->WriteObjectEnd();
	}

	JsonWriter->WriteObjectEnd();
	JsonWriter->WriteObjectEnd();
}

void ASandboxLevelController::SavePreparedObjects(const TArray<FSandboxObjectDescriptor>& ObjDescList) {
	UE_LOG(LogTemp, Log, TEXT("----------- save level json -----------"));

	FString JsonStr;
	FString FileName = TEXT("level.json");
	FString SavePath = FPaths::ProjectSavedDir();
	FString FullPath = SavePath + TEXT("/Map/") + MapName + TEXT("/") + FileName;

	UE_LOG(LogTemp, Log, TEXT("level json path -> %s"), *FullPath);

	TSharedRef <TJsonWriter<TCHAR>> JsonWriter = TJsonWriterFactory<>::Create(&JsonStr);

	JsonWriter->WriteObjectStart();

	SaveLevelJsonExt(JsonWriter);

	JsonWriter->WriteArrayStart("SandboxObjectList");

	for (const auto& ObjDesc : ObjDescList) {
		SaveObject(JsonWriter, ObjDesc);
	}

	JsonWriter->WriteArrayEnd();

	JsonWriter->WriteObjectEnd();
	JsonWriter->Close();

	FFileHelper::SaveStringToFile(*JsonStr, *FullPath);
}

FSandboxObjectDescriptor FSandboxObjectDescriptor::MakeObjDescriptor(ASandboxObject* SandboxObject) {
	FSandboxObjectDescriptor ObjDesc;
	ObjDesc.ClassId = SandboxObject->GetSandboxClassId();
	ObjDesc.TypeId = SandboxObject->GetSandboxTypeId();
	ObjDesc.Transform = SandboxObject->GetTransform();
	ObjDesc.PropertyMap = SandboxObject->PropertyMap;

	UContainerComponent* Container = SandboxObject->GetContainer(TEXT("ObjectContainer"));
	if (Container) {
		TArray<FContainerStack> Content = Container->Content;
		int SlotId = 0;
		for (const auto& Stack : Content) {
			if (Stack.Amount > 0) {
				if (Stack.GetObject()) {
					FTempContainerStack TempContainerStack;
					TempContainerStack.Stack = Stack;
					TempContainerStack.SlotId = SlotId;
					ObjDesc.Container.Add(TempContainerStack);
				}
			}

			SlotId++;
		}
	}

	return ObjDesc;
}

void ASandboxLevelController::PrepareObjectForSave(TArray<FSandboxObjectDescriptor>& ObjDescList) {
	for (TActorIterator<ASandboxObject> ActorItr(GetWorld()); ActorItr; ++ActorItr) {
		ASandboxObject* SandboxObject = Cast<ASandboxObject>(*ActorItr);
		if (SandboxObject) {
			if (SandboxObject->GetParentActor()) {
				continue;
			}

			FSandboxObjectDescriptor ObjDesc = FSandboxObjectDescriptor::MakeObjDescriptor(SandboxObject);
			ObjDescList.Add(ObjDesc);
		}
	}
}

void ASandboxLevelController::SaveLevelJson() {
	TArray<FSandboxObjectDescriptor> ObjDescList;
	PrepareObjectForSave(ObjDescList);
	SavePreparedObjects(ObjDescList);
}

void ASandboxLevelController::SaveLevelJsonExt(TSharedRef <TJsonWriter<TCHAR>> JsonWriter) {

}

void ASandboxLevelController::PrepareMetaData() {
	if (bIsMetaDataReady) {
		return;
	}

	if (!ObjectMap) {
		UE_LOG(LogTemp, Error, TEXT("ASandboxLevelController::LoadLevelJson() -----  ObjectMap == NULL"));
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("----------- load object map -----------"));

	ObjectMapByClassName.Empty();
	ObjectMapById.Empty();

	TArray<TSubclassOf<ASandboxObject>> ObjectListCopy = ObjectMap->ObjectList;
	ObjectListCopy.Sort([](const TSubclassOf<ASandboxObject>& ip1, const TSubclassOf<ASandboxObject>& ip2) {
		ASandboxObject* SandboxObject1 = Cast<ASandboxObject>(ip1->ClassDefaultObject);
		ASandboxObject* SandboxObject2 = Cast<ASandboxObject>(ip2->ClassDefaultObject);
		uint64 ClassId1 = SandboxObject1->GetSandboxClassId();
		uint64 ClassId2 = SandboxObject2->GetSandboxClassId();
		return  ClassId1 < ClassId2;
	});

	for (const TSubclassOf<ASandboxObject>& SandboxObjectSubclass : ObjectListCopy) {
		//TSubclassOf<ASandboxObject> SandboxObjectSubclass = Elem.Value;
		ASandboxObject* SandboxObject = Cast<ASandboxObject>(SandboxObjectSubclass->ClassDefaultObject);
		uint64 ClassId = SandboxObject->GetSandboxClassId();
		FString ClassName = SandboxObjectSubclass->ClassDefaultObject->GetClass()->GetName();

		if (ClassId == 0) {
			UE_LOG(LogTemp, Error, TEXT("ClassName -> %s has no class id"), *ClassName);
			continue;
		}

		UE_LOG(LogTemp, Log, TEXT("%s -> %d"), *ClassName, ClassId);
		ObjectMapByClassName.Add(ClassName, SandboxObjectSubclass);
		ObjectMapById.Add(ClassId, SandboxObjectSubclass);
	}

	bIsMetaDataReady = true;
}

void ASandboxLevelController::LoadLevelJson() {
	if (!ObjectMap) {
		return;
	}

	PrepareMetaData();

	UE_LOG(LogTemp, Log, TEXT("----------- load level json -----------"));

	FString FileName = TEXT("level.json");
	FString SavePath = FPaths::ProjectSavedDir();
	FString FullPath = SavePath + TEXT("/Map/") + MapName + TEXT("/") + FileName;

	FString JsonRaw;
	if (!FFileHelper::LoadFileToString(JsonRaw, *FullPath)) {
		UE_LOG(LogTemp, Error, TEXT("Error loading json file"));
	}

	TArray<FSandboxObjectDescriptor> ObjDescList;

	TSharedPtr<FJsonObject> JsonParsed;
	TSharedRef<TJsonReader<>> JsonReader = TJsonReaderFactory<>::Create(JsonRaw);
	if (FJsonSerializer::Deserialize(JsonReader, JsonParsed)) {
		TArray <TSharedPtr<FJsonValue>> SandboxObjectList = JsonParsed->GetArrayField("SandboxObjectList");
		for (int Idx = 0; Idx < SandboxObjectList.Num(); Idx++) {
			TSharedPtr<FJsonObject> ObjPtr = SandboxObjectList[Idx]->AsObject();
			TSharedPtr<FJsonObject> SandboxObjectPtr = ObjPtr->GetObjectField(TEXT("Object"));

			FSandboxObjectDescriptor ObjDesc;

			ObjDesc.ClassId = SandboxObjectPtr->GetIntegerField(TEXT("ClassId"));
			ObjDesc.TypeId = SandboxObjectPtr->GetIntegerField(TEXT("TypeId"));

			FVector Location;
			TArray <TSharedPtr<FJsonValue>> LocationValArray = SandboxObjectPtr->GetArrayField("Location");
			Location.X = LocationValArray[0]->AsNumber();
			Location.Y = LocationValArray[1]->AsNumber();
			Location.Z = LocationValArray[2]->AsNumber();

			FRotator Rotation;
			TArray <TSharedPtr<FJsonValue>>  RotationValArray = SandboxObjectPtr->GetArrayField("Rotation");
			Rotation.Pitch = RotationValArray[0]->AsNumber();
			Rotation.Yaw = RotationValArray[1]->AsNumber();
			Rotation.Roll = RotationValArray[2]->AsNumber();

			FVector Scale;
			TArray <TSharedPtr<FJsonValue>> ScaleValArray = SandboxObjectPtr->GetArrayField("Scale");
			Scale.X = ScaleValArray[0]->AsNumber();
			Scale.Y = ScaleValArray[1]->AsNumber();
			Scale.Z = ScaleValArray[2]->AsNumber();

			ObjDesc.Transform = FTransform(Rotation, Location, Scale);

			const TSharedPtr<FJsonObject>* ObjPropJson = nullptr;
			bool bHasProperties = SandboxObjectPtr->TryGetObjectField("Properties", ObjPropJson);
			if (bHasProperties) {
				for (auto& Itm : (*ObjPropJson)->Values) {
					FString Key = Itm.Key;
					FString Value = Itm.Value->AsString();
					ObjDesc.PropertyMap.Add(Key, Value);
				}
			}

			const TArray<TSharedPtr<FJsonValue>>* ContainersJsonArray = nullptr;
			bool bHasContainers = SandboxObjectPtr->TryGetArrayField("Containers", ContainersJsonArray);
			if (bHasContainers) {
				for (TSharedPtr<FJsonValue> ContainerJsonValue : *ContainersJsonArray) {
					TSharedPtr<FJsonObject> ContainerJsonObject = ContainerJsonValue->AsObject()->GetObjectField(TEXT("Container"));
					FString ContainerName = ContainerJsonObject->GetStringField("Name");
					TArray<TSharedPtr<FJsonValue>> ContentArray = ContainerJsonObject->GetArrayField("Content");
					//UE_LOG(LogTemp, Warning, TEXT("ContainerName %s"), *ContainerName);

					for (TSharedPtr<FJsonValue> ContentJsonValue : ContentArray) {
						int SlotId = ContentJsonValue->AsObject()->GetIntegerField("SlotId");
						int ClassId = ContentJsonValue->AsObject()->GetIntegerField("ClassId");
						int Amount = ContentJsonValue->AsObject()->GetIntegerField("Amount");

						FContainerStack Stack;
						Stack.Amount = Amount;
						Stack.SandboxClassId = ClassId;

						FTempContainerStack TempContainerStack;
						TempContainerStack.Stack = Stack;
						TempContainerStack.SlotId = SlotId;

						ObjDesc.Container.Add(TempContainerStack);
					}
				}
			}
			ObjDescList.Add(ObjDesc);
		}

		LoadLevelJsonExt(JsonParsed);
	}

	SpawnPreparedObjects(ObjDescList);
}

ASandboxObject* ASandboxLevelController::SpawnPreparedObject(const FSandboxObjectDescriptor& ObjDesc) {
	ASandboxObject* NewObject = SpawnSandboxObject(ObjDesc.ClassId, ObjDesc.Transform);
	if (NewObject) {
		NewObject->PropertyMap = ObjDesc.PropertyMap;
		NewObject->PostLoadProperties();

		UContainerComponent* Container = NewObject->GetContainer(TEXT("ObjectContainer"));
		if (Container) {
			for (const auto& Itm : ObjDesc.Container) {
				Container->SetStackDirectly(Itm.Stack, Itm.SlotId);
			}
		}
	}

	return NewObject;
}

void ASandboxLevelController::SpawnPreparedObjects(const TArray<FSandboxObjectDescriptor>& ObjDescList) {
	for (const auto& ObjDesc : ObjDescList) {
		SpawnPreparedObject(ObjDesc);
	}
}

ASandboxObject* ASandboxLevelController::SpawnSandboxObject(const int ClassId, const FTransform& Transform) {
	if (GetNetMode() != NM_Client) {
		TSubclassOf<ASandboxObject> SandboxObject = GetSandboxObjectByClassId(ClassId);
		if (SandboxObject) {
			UClass* SpawnClass = SandboxObject->ClassDefaultObject->GetClass();
			ASandboxObject* NewObject = (ASandboxObject*)GetWorld()->SpawnActor(SpawnClass, &Transform);
			if (NewObject) {
				NewObject->SandboxNetUid = ObjectCounter;
				GlobalObjectMap.Add(ObjectCounter, NewObject);
				ObjectCounter++;
			}
			return NewObject;
		}
	}

	return nullptr;
}

void ASandboxLevelController::LoadLevelJsonExt(TSharedPtr<FJsonObject> JsonParsed) {

}

TSubclassOf<ASandboxObject> ASandboxLevelController::GetSandboxObjectByClassId(int32 ClassId) {
	if (!bIsMetaDataReady) {
		PrepareMetaData();
	}

	if (ObjectMapById.Contains(ClassId)) {
		return ObjectMapById[ClassId];
	}

	return nullptr;
}

ASandboxObject* ASandboxLevelController::GetDefaultSandboxObject(uint64 ClassId) {
	if (ObjectMapById.Contains(ClassId)) {
		return (ASandboxObject*)(ObjectMapById[ClassId]->GetDefaultObject());
	}

	return nullptr;
}

ASandboxLevelController* ASandboxLevelController::GetInstance() {
	return StaticSelf;
}

ASandboxObject* ASandboxLevelController::GetSandboxObject(uint64 ClassId) {
	if (!bIsMetaDataReady) {
		PrepareMetaData();
	}

	if (ObjectMapById.Contains(ClassId)) {
		return (ASandboxObject*)(ObjectMapById[ClassId]->GetDefaultObject());
	}

	return nullptr;
}

bool ASandboxLevelController::RemoveSandboxObject(ASandboxObject* Obj) {
	if (GetNetMode() != NM_Client) {
		if (Obj) {
			Obj->Destroy();
			return true;
		}
	}

	return false;
}

ASandboxObject* ASandboxLevelController::GetObjectByNetUid(uint64 NetUid) {
	if (GlobalObjectMap.Contains(NetUid)) {
		return GlobalObjectMap[NetUid];
	}

	return nullptr;
}

void ASandboxLevelController::SpawnEffect(const int32 EffectId, const FTransform& Transform) {
	if (GetNetMode() != NM_Client) {
		if (ObjectMap->Effects.Contains(EffectId)) {
			TSubclassOf<ASandboxEffect> Effect = ObjectMap->Effects[EffectId];
			if (Effect) {
				UClass* SpawnClass = Effect->ClassDefaultObject->GetClass();
				GetWorld()->SpawnActor(SpawnClass, &Transform);
			}
		}
	}
}