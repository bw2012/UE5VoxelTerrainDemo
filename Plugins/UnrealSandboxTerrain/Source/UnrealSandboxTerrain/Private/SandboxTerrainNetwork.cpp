
#include "SandboxTerrainController.h"
#include "VoxelDataInfo.hpp"
#include "TerrainZoneComponent.h"
#include "TerrainData.hpp"
#include "TerrainClientComponent.h"

void AppendDataToBuffer(TValueDataPtr Data, FBufferArchive& Buffer) {
	TArray<uint8> Array;
	Array.SetNumUninitialized(Data->size());
	for (int I = 0; I < Data->size(); I++) {
		Array[I] = Data->at(I);
	}

	int32 Size = Array.Num();
	Buffer << Size;
	Buffer.Append(Array);

	UE_LOG(LogSandboxTerrain, Warning, TEXT("Size %d"), Size);
}

void ASandboxTerrainController::NetworkSerializeVd(FBufferArchive& Buffer, const TVoxelIndex& Index) {
	TVoxelDataInfoPtr VdInfoPtr = TerrainData->GetVoxelDataInfo(Index);
	// TODO: shared lock Vd
	VdInfoPtr->Lock();

	int32 State = (int32)VdInfoPtr->DataState;
	Buffer << State;

	if (VdInfoPtr->DataState == TVoxelDataState::READY_TO_LOAD) {
		TVoxelData* Vd = LoadVoxelDataByIndex(Index);
		TValueDataPtr Data = SerializeVd(Vd);
		AppendDataToBuffer(Data, Buffer);
		delete Vd;
	} else if (VdInfoPtr->DataState == TVoxelDataState::LOADED) {
		TValueDataPtr Data = SerializeVd(VdInfoPtr->Vd);
		AppendDataToBuffer(Data, Buffer);
	} 

	VdInfoPtr->Unlock();
}

// spawn received zone on client
void ASandboxTerrainController::NetworkSpawnClientZone(const TVoxelIndex& Index, FArrayReader& RawVdData) {
	FVector Pos = GetZonePos(Index);

	FMemoryReader BinaryData = FMemoryReader(RawVdData, true);
	BinaryData.Seek(RawVdData.Tell());

	int32 State;
	BinaryData << State;
	UE_LOG(LogSandboxTerrain, Warning, TEXT("Client: Vd state %d"), State);

	TVoxelDataState ServerVdState = (TVoxelDataState)State;
	if (ServerVdState == TVoxelDataState::UNGENERATED) {
		TArray<TSpawnZoneParam> SpawnList;
		TSpawnZoneParam SpawnZoneParam;
		SpawnZoneParam.Index = Index;
		SpawnZoneParam.TerrainLodMask = 0;
		SpawnList.Add(SpawnZoneParam);
		BatchSpawnZone(SpawnList);
	}

	if (ServerVdState == TVoxelDataState::READY_TO_LOAD || ServerVdState == TVoxelDataState::LOADED) {
		int32 Size;
		BinaryData << Size;
		UE_LOG(LogSandboxTerrain, Warning, TEXT("Size %d"), Size);

		TVoxelDataInfoPtr VdInfoPtr = TerrainData->GetVoxelDataInfo(Index);
		VdInfoPtr->Lock();


		VdInfoPtr->Unlock();
	}


	/*
	TVoxelDataInfo VdInfo;
	VdInfo.Vd = NewVoxelData();
	VdInfo.Vd->setOrigin(Pos);

	FMemoryReader BinaryData = FMemoryReader(RawVdData, true);
	BinaryData.Seek(RawVdData.Tell());
	//deserializeVoxelDataFast(*VdInfo.Vd, BinaryData, true);

	VdInfo.DataState = TVoxelDataState::GENERATED;
	VdInfo.SetChanged();
	VdInfo.Vd->setCacheToValid();

	//TerrainData->RegisterVoxelData(VdInfo, Index);
	//TerrainData->RegisterVoxelData(VdInfo, Index);

	if (VdInfo.Vd->getDensityFillState() == TVoxelDataFillState::MIXED) {
		TMeshDataPtr MeshDataPtr = GenerateMesh(VdInfo.Vd);
		
		InvokeSafe([=]() {
			UTerrainZoneComponent* Zone = AddTerrainZone(Pos);
			//Zone->ApplyTerrainMesh(MeshDataPtr);
		});
		
	}
	*/

}

void ASandboxTerrainController::OnClientConnected() {
	FString Text = TEXT("Connected to voxel data server");

	GEngine->AddOnScreenDebugMessage(-1, 10, FColor::Yellow, Text);
	UE_LOG(LogSandboxTerrain, Warning, TEXT("%s"), *Text);

	TVoxelIndex Index(0, 0, 0);
	TerrainClientComponent->RequestVoxelData(Index);
}