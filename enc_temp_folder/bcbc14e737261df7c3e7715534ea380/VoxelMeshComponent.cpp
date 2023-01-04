// Copyright blackw 2015-2020

#include "VoxelMeshComponent.h"

#include "SandboxVoxeldata.h"
#include "SandboxTerrainController.h"
#include "SandboxVoxeldata.h"

#include "Engine.h"
#include "DynamicMeshBuilder.h"
#include "PhysicsEngine/PhysicsSettings.h"
#include "PhysicsEngine/BodySetup.h"

#include "SceneManagement.h"


/** Resource array to pass  */
class FProcMeshVertexResourceArray : public FResourceArrayInterface {
public:
	FProcMeshVertexResourceArray(void* InData, uint32 InSize) : Data(InData) , Size(InSize) { }

	virtual const void* GetResourceData() const override { return Data; }
	virtual uint32 GetResourceDataSize() const override { return Size; }
	virtual void Discard() override { }
	virtual bool IsStatic() const override { return false; }
	virtual bool GetAllowCPUAccess() const override { return false; }
	virtual void SetAllowCPUAccess(bool bInNeedsCPUAccess) override { }

private:
	void* Data;
	uint32 Size;
};

/** Vertex Buffer */
class FProcMeshVertexBuffer : public FVertexBuffer {
public:
	TArray<FDynamicMeshVertex> Vertices;

	virtual void InitRHI() override {
		const uint32 SizeInBytes = Vertices.Num() * sizeof(FDynamicMeshVertex);

		FProcMeshVertexResourceArray ResourceArray(Vertices.GetData(), SizeInBytes);
		FRHIResourceCreateInfo CreateInfo(TEXT("FProcMeshVertexBuffer"), &ResourceArray);
		VertexBufferRHI = RHICreateVertexBuffer(SizeInBytes, BUF_Static, CreateInfo);
	}
};

/** Index Buffer */
class FProcMeshIndexBuffer : public FIndexBuffer {
public:
	TArray<int32> Indices;

	virtual void InitRHI() override {
		FRHIResourceCreateInfo CreateInfo(TEXT("FProcMeshIndexBuffer"));
		void* Buffer = nullptr;
		IndexBufferRHI = RHICreateAndLockIndexBuffer(sizeof(int32), Indices.Num() * sizeof(int32), BUF_Static, CreateInfo, Buffer);

		// Write the indices to the index buffer.		
		FMemory::Memcpy(Buffer, Indices.GetData(), Indices.Num() * sizeof(int32));
		RHIUnlockIndexBuffer(IndexBufferRHI);
	}
};

/** Class representing a single section of the proc mesh */
class FProcMeshProxySection
{
public:
	/** Material applied to this section */
	UMaterialInterface* Material;
	/** Vertex buffer for this section */
	FStaticMeshVertexBuffers VertexBuffers;
	/** Index buffer for this section */
	FProcMeshIndexBuffer IndexBuffer;
	/** Vertex factory for this section */
	FLocalVertexFactory VertexFactory;
	/** Whether this section is currently visible */
	bool bSectionVisible;

	FProcMeshProxySection(ERHIFeatureLevel::Type InFeatureLevel) : Material(NULL), VertexFactory(InFeatureLevel, "FProcMeshProxySection") , bSectionVisible(true) {}

	~FProcMeshProxySection() {
		this->VertexBuffers.PositionVertexBuffer.ReleaseResource();
		this->VertexBuffers.StaticMeshVertexBuffer.ReleaseResource();
		this->VertexBuffers.ColorVertexBuffer.ReleaseResource();
		this->IndexBuffer.ReleaseResource();
		this->VertexFactory.ReleaseResource();
	}
};

typedef TArray<FProcMeshProxySection*> TMeshPtrArray;

class FMeshProxyLodSection {
public:

	/** Array of material sections */
	TMeshPtrArray MaterialMeshPtrArray;

	/** */
	TMeshPtrArray NormalPatchPtrArray[6];

	/** Array of transition sections (todo: should be changed to mat sections)*/
	FProcMeshProxySection* transitionMesh[6];

	FMeshProxyLodSection() {
		for (auto i = 0; i < 6; i++) {
			transitionMesh[i] = nullptr;
			NormalPatchPtrArray[i].Empty();
		}
	}

	~FMeshProxyLodSection() {

		for (FProcMeshProxySection* MatSectionPtr : MaterialMeshPtrArray) {
			delete MatSectionPtr;
		}

		for (auto i = 0; i < 6; i++) {
			if (transitionMesh[i] != nullptr) {
				delete transitionMesh[i];
			}

			for (FProcMeshProxySection* Section : NormalPatchPtrArray[i]) {
				if (Section != nullptr) {
					delete Section;
				}
			}
		}
	}
};


/**
*	Struct used to send update to mesh data
*	Arrays may be empty, in which case no update is performed.
*/
class FProcMeshSectionUpdateData {
public:
	/** Section to update */
	int32 TargetSection;
	/** New vertex information */
	TArray<FProcMeshVertex> NewVertexBuffer;
};

static void ConvertProcMeshToDynMeshVertex(FDynamicMeshVertex& Vert, const FProcMeshVertex& ProcVert) {
	Vert.Position.X = ProcVert.PositionX;
	Vert.Position.Y = ProcVert.PositionY;
	Vert.Position.Z = ProcVert.PositionZ;

	switch (ProcVert.MatIdx) {
		case 0:  Vert.Color = FColor(255, 0, 0, 0); break;
		case 1:  Vert.Color = FColor(0, 255, 0, 0); break;
		case 2:  Vert.Color = FColor(0, 0, 255, 0); break;
		default: Vert.Color = FColor(0, 0, 0, 0); break;
	}

	// ignore texture crd
	Vert.TextureCoordinate[0] = FVector2f(0.f, 0.f);

	// ignore tangent
	Vert.TangentX = FVector(1.f, 0.f, 0.f);
	FVector Normal(ProcVert.NormalX, ProcVert.NormalY, ProcVert.NormalZ);
	Vert.TangentZ = Normal;
	Vert.TangentZ.Vector.W = 0;
}

extern float LodScreenSizeArray[LOD_ARRAY_SIZE];

class FVoxelMeshSceneProxy final : public FPrimitiveSceneProxy {

private:
	/** Array of lod sections */
	TArray<FMeshProxyLodSection*> LodSectionArray;
	UBodySetup* BodySetup;
	FMaterialRelevance MaterialRelevance;
	FVector ZoneOrigin;
	float CullDistance = 20000.f;

	const FVector V[6] = {
		FVector(-USBT_ZONE_SIZE, 0, 0), // -X
		FVector(USBT_ZONE_SIZE, 0, 0),	// +X

		FVector(0, -USBT_ZONE_SIZE, 0), // -Y
		FVector(0, USBT_ZONE_SIZE, 0),	// +Y

		FVector(0, 0, -USBT_ZONE_SIZE), // -Z
		FVector(0, 0, USBT_ZONE_SIZE),	// +Z
	};

public:

	SIZE_T GetTypeHash() const override {
		static size_t UniquePointer;
		return reinterpret_cast<size_t>(&UniquePointer);
	}

	FVoxelMeshSceneProxy(UVoxelMeshComponent* Component) : FPrimitiveSceneProxy(Component), BodySetup(Component->GetBodySetup()), MaterialRelevance(Component->GetMaterialRelevance(GetScene().GetFeatureLevel())) {
		ZoneOrigin = Component->GetComponentLocation();

		ASandboxTerrainController* Controller = Cast<ASandboxTerrainController>(Component->GetAttachmentRootActor());
		if (Controller) {
			CullDistance = Controller->ActiveAreaSize * 1.5 * USBT_ZONE_SIZE;
		}

		// Copy each section
		CopyAll(Component);
	}

	virtual ~FVoxelMeshSceneProxy() {
		for (FMeshProxyLodSection* Section : LodSectionArray) {
			if (Section != nullptr) {
				delete Section;
			}
		}
	}

	template<class T>
	void CopyMaterialMesh(UVoxelMeshComponent* Component, TMap<unsigned short, T>& MaterialMap, TMeshPtrArray& TargetMeshPtrArray, std::function<UMaterialInterface*(T)> GetMaterial) {
		UMaterialInterface* DefaultMaterial = UMaterial::GetDefaultMaterial(MD_Surface);

		for (auto& Element : MaterialMap) {
			unsigned short MatId = Element.Key;

			T& Section = Element.Value;

			TMeshMaterialSection& SrcMaterialSection = static_cast<TMeshMaterialSection&>(Section);
			FProcMeshSection& SourceMaterialSection = SrcMaterialSection.MaterialMesh;

			UMaterialInterface* Material = GetMaterial(Section);
			if (Material == nullptr) {
                Material = DefaultMaterial;
            }

			FProcMeshProxySection* NewMaterialProxySection = new FProcMeshProxySection(GetScene().GetFeatureLevel());
			NewMaterialProxySection->Material = Material;

			CopySection(SourceMaterialSection, NewMaterialProxySection, Component);
			TargetMeshPtrArray.Add(NewMaterialProxySection);
		}
	}

	void CopyAll(UVoxelMeshComponent* Component) {
		UMaterialInterface* DefaultMaterial = UMaterial::GetDefaultMaterial(MD_Surface);
		ASandboxTerrainController* TerrainController = Cast<ASandboxTerrainController>(Component->GetAttachmentRootActor());
		
		// if not terrain
		if (TerrainController == nullptr) {
			// grab default material
			DefaultMaterial = Component->GetMaterial(0);
			if (DefaultMaterial == NULL) {
				DefaultMaterial = UMaterial::GetDefaultMaterial(MD_Surface);
			}
		}

		const int32 NumSections = Component->MeshSectionLodArray.Num();

		if (NumSections == 0) {
			return;
		}

		LodSectionArray.AddZeroed(NumSections);

		for (int SectionIdx = 0; SectionIdx < NumSections; SectionIdx++) {
			FMeshProxyLodSection* NewLodSection = new FMeshProxyLodSection();

			// copy regular material mesh
			TMaterialSectionMap& MaterialMap = Component->MeshSectionLodArray[SectionIdx].RegularMeshContainer.MaterialSectionMap;
			CopyMaterialMesh<TMeshMaterialSection>(Component, MaterialMap, NewLodSection->MaterialMeshPtrArray,
				[&TerrainController, &DefaultMaterial](TMeshMaterialSection Ms) {return (TerrainController) ? TerrainController->GetRegularTerrainMaterial(Ms.MaterialId) : DefaultMaterial; });

			// copy transition material mesh
			TMaterialTransitionSectionMap& MaterialTransitionMap = Component->MeshSectionLodArray[SectionIdx].RegularMeshContainer.MaterialTransitionSectionMap;
			CopyMaterialMesh<TMeshMaterialTransitionSection>(Component, MaterialTransitionMap, NewLodSection->MaterialMeshPtrArray,
				[&TerrainController, &DefaultMaterial](TMeshMaterialTransitionSection Ms) {return (TerrainController) ? TerrainController->GetTransitionTerrainMaterial(Ms.MaterialIdSet) : DefaultMaterial; });

			for (auto i = 0; i < 6; i++) {
				// copy regular material mesh
				TMaterialSectionMap& LodMaterialMap = Component->MeshSectionLodArray[SectionIdx].TransitionPatchArray[i].MaterialSectionMap;
				CopyMaterialMesh<TMeshMaterialSection>(Component, LodMaterialMap, NewLodSection->NormalPatchPtrArray[i],
					[&TerrainController, &DefaultMaterial](TMeshMaterialSection Ms) {return (TerrainController) ? TerrainController->GetRegularTerrainMaterial(Ms.MaterialId) : DefaultMaterial; });

				// copy transition material mesh
				TMaterialTransitionSectionMap& LodMaterialTransitionMap = Component->MeshSectionLodArray[SectionIdx].TransitionPatchArray[i].MaterialTransitionSectionMap;
				CopyMaterialMesh<TMeshMaterialTransitionSection>(Component, LodMaterialTransitionMap, NewLodSection->NormalPatchPtrArray[i],
					[&TerrainController, &DefaultMaterial](TMeshMaterialTransitionSection Ms) {return (TerrainController) ? TerrainController->GetTransitionTerrainMaterial(Ms.MaterialIdSet) : DefaultMaterial; });
			}

			// Save ref to new section
			LodSectionArray[SectionIdx] = NewLodSection;
		}
	}

	FORCEINLINE void CopySection(FProcMeshSection& SrcSection, FProcMeshProxySection* NewSection, UVoxelMeshComponent* Component) {
		if (SrcSection.ProcIndexBuffer.Num() > 0 && SrcSection.ProcVertexBuffer.Num() > 0) {

			// Copy data from vertex buffer
			const int32 NumVerts = SrcSection.ProcVertexBuffer.Num();

			// Allocate verts
			TArray<FDynamicMeshVertex> Vertices;
			Vertices.SetNumUninitialized(NumVerts);
			// Copy verts
			for (int VertIdx = 0; VertIdx < NumVerts; VertIdx++) {
				const FProcMeshVertex& ProcVert = SrcSection.ProcVertexBuffer[VertIdx];
				FDynamicMeshVertex& Vert = Vertices[VertIdx];
				ConvertProcMeshToDynMeshVertex(Vert, ProcVert);
			}

			// Copy index buffer
			NewSection->IndexBuffer.Indices = SrcSection.ProcIndexBuffer;

			// Init vertex factory
			//NewSection->VertexFactory.Init(&NewSection->VertexBuffer);
			NewSection->VertexBuffers.InitFromDynamicVertex(&NewSection->VertexFactory, Vertices);

			// Enqueue initialization of render resource
			BeginInitResource(&NewSection->VertexBuffers.PositionVertexBuffer);
			BeginInitResource(&NewSection->VertexBuffers.StaticMeshVertexBuffer);
			BeginInitResource(&NewSection->VertexBuffers.ColorVertexBuffer);
			BeginInitResource(&NewSection->IndexBuffer);
			BeginInitResource(&NewSection->VertexFactory);

			// Grab material
			if (NewSection->Material == nullptr) {
				NewSection->Material = UMaterial::GetDefaultMaterial(MD_Surface);
			}

			// Copy visibility info
			NewSection->bSectionVisible = true;
		}
	}

	void SetSectionVisibility_RenderThread(int32 SectionIndex, bool bNewVisibility) {
		check(IsInRenderingThread());

		if (SectionIndex < LodSectionArray.Num() && LodSectionArray[SectionIndex] != nullptr) {
			//	Sections[SectionIndex]->bSectionVisible = bNewVisibility;
		}
	}

	//================================================================================================
	// Draw main zone as static mesh
	//================================================================================================

	void DrawStaticMeshSection(FStaticPrimitiveDrawInterface* PDI, FProcMeshProxySection* Section, int LODIndex) {
		FMaterialRenderProxy* MaterialInstance = Section->Material->GetRenderProxy();

		FMeshBatch Mesh;
		Mesh.bWireframe = false;
		Mesh.VertexFactory = &Section->VertexFactory;
		Mesh.MaterialRenderProxy = MaterialInstance;
		Mesh.ReverseCulling = IsLocalToWorldDeterminantNegative();
		Mesh.Type = PT_TriangleList;
		Mesh.DepthPriorityGroup = SDPG_World;
		Mesh.bCanApplyViewModeOverrides = false;
		Mesh.LODIndex = LODIndex;
		Mesh.bDitheredLODTransition = false;

		FMeshBatchElement& BatchElement = Mesh.Elements[0];
		BatchElement.IndexBuffer = &Section->IndexBuffer;
		BatchElement.FirstIndex = 0;
		BatchElement.NumPrimitives = Section->IndexBuffer.Indices.Num() / 3;
		BatchElement.MinVertexIndex = 0;
		BatchElement.MaxVertexIndex = Section->VertexBuffers.PositionVertexBuffer.GetNumVertices() - 1;

		const float ScreenSize = LodScreenSizeArray[LODIndex];
		//PDI->DrawMesh(Mesh, MAX_FLT); // no LOD
		PDI->DrawMesh(Mesh, ScreenSize); 
	}

	void DrawStaticLodSection(FStaticPrimitiveDrawInterface* PDI, const FMeshProxyLodSection* LodSection, int LODIndex) {
		if (LodSection != nullptr) {
			for (FProcMeshProxySection* MatSection : LodSection->MaterialMeshPtrArray) {
				if (MatSection != nullptr) {
					DrawStaticMeshSection(PDI, MatSection, LODIndex);
				}
			}
		}
	}

	virtual void DrawStaticElements(FStaticPrimitiveDrawInterface* PDI) {
		if (LodSectionArray.Num() > 0) {
			for (int LODIndex = 0; LODIndex < LodSectionArray.Num(); LODIndex++) {
				const FMeshProxyLodSection* LodSection = LodSectionArray[LODIndex];
				DrawStaticLodSection(PDI, LodSection, LODIndex);
			}
		}
	}

	void DrawBox(FMeshElementCollector& Collector, int32 ViewIndex, const FLinearColor Color) const {
		FPrimitiveDrawInterface* PDI = Collector.GetPDI(ViewIndex);
		auto Box = FBox{ ZoneOrigin - FVector(1.f, 1.f, 1.f) * 500.f, ZoneOrigin + FVector(1.f, 1.f, 1.f) * 500.f };
		DrawWireBox(PDI, Box, Color, SDPG_World, 8.f, 0.f, false);
	}
	
	int32 ComputeLodIndexByScreenSize(const FSceneView* View, const FVector& Origin) const {
		const FBoxSphereBounds& ProxyBounds = GetBounds();
		const float ScreenSize = ComputeBoundsScreenSize(Origin, ProxyBounds.SphereRadius, *View);

		int32 I = 0;
		for (int LODIndex = 0; LODIndex < LOD_ARRAY_SIZE; LODIndex++) { // TODO: fix LODIndex < 4
			if (ScreenSize < LodScreenSizeArray[LODIndex]) {
				I = LODIndex;
			}
		}

		return I;
	}

	//================================================================================================
	// Draw transvoxel patches as dynamic mesh  
	//================================================================================================

	FORCENOINLINE virtual void GetDynamicMeshElements(const TArray<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, uint32 VisibilityMap, FMeshElementCollector& Collector) const override {
		if (LodSectionArray.Num() == 0) {
			return;
		}

		for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ViewIndex++) {
			if (VisibilityMap & (1 << ViewIndex)) {
				const FSceneView* View = Views[ViewIndex];
				const FBoxSphereBounds& ProxyBounds = GetBounds();
				int32 LodIndex = ComputeLodIndexByScreenSize(View, ProxyBounds.Origin);

				if (LodIndex == 0) {
					//DrawBox(Collector, ViewIndex, FLinearColor::Red);
				}

				if (LodIndex > 0) {
					// draw transition patches
					for (auto i = 0; i < 6; i++) {
						const FVector  NeighborZoneOrigin = ZoneOrigin + V[i];
						const auto NeighborLodIndex = ComputeLodIndexByScreenSize(View, NeighborZoneOrigin);
						if (NeighborLodIndex < LodIndex) {
							FMeshProxyLodSection* LodSectionProxy = LodSectionArray[LodIndex];
							if (LodSectionProxy != nullptr) {
								for (FProcMeshProxySection* MatSection : LodSectionProxy->NormalPatchPtrArray[i]) {
									if (MatSection != nullptr && MatSection->Material != nullptr) {
										//FMaterialRenderProxy* MaterialProxy = bWireframe ? WireframeMaterialInstance : MatSection->Material->GetRenderProxy();
										//DrawDynamicMeshSection(MatSection, Collector, MaterialProxy, bWireframe, ViewIndex);
										DrawDynamicMeshSection(MatSection, Collector, MatSection->Material->GetRenderProxy(), false, ViewIndex);
										// TODO: check: ��������� ��������� ��� lod = 0 
									}
								}
							}
						}
					}
				}
			}
		}
	}

	FORCEINLINE void DrawDynamicMeshSection(const FProcMeshProxySection* Section, FMeshElementCollector& Collector, FMaterialRenderProxy* MaterialProxy, bool bWireframe, int32 ViewIndex) const {
		if (Section->VertexBuffers.PositionVertexBuffer.GetNumVertices() == 0) return;

		// Draw the mesh.
		FMeshBatch& Mesh = Collector.AllocateMesh();
		FMeshBatchElement& BatchElement = Mesh.Elements[0];
		BatchElement.IndexBuffer = &Section->IndexBuffer;
		Mesh.bWireframe = bWireframe;
		Mesh.VertexFactory = &Section->VertexFactory;
		Mesh.MaterialRenderProxy = MaterialProxy;

		bool bHasPrecomputedVolumetricLightmap;
		FMatrix PreviousLocalToWorld;
		int32 SingleCaptureIndex;
		bool bOutputVelocity;
		GetScene().GetPrimitiveUniformShaderParameters_RenderThread(GetPrimitiveSceneInfo(), bHasPrecomputedVolumetricLightmap, PreviousLocalToWorld, SingleCaptureIndex, bOutputVelocity);

		FDynamicPrimitiveUniformBuffer& DynamicPrimitiveUniformBuffer = Collector.AllocateOneFrameResource<FDynamicPrimitiveUniformBuffer>();
		DynamicPrimitiveUniformBuffer.Set(GetLocalToWorld(), PreviousLocalToWorld, GetBounds(), GetLocalBounds(), true, bHasPrecomputedVolumetricLightmap, DrawsVelocity(), bOutputVelocity);
		BatchElement.PrimitiveUniformBufferResource = &DynamicPrimitiveUniformBuffer.UniformBuffer;

		BatchElement.FirstIndex = 0;
		BatchElement.NumPrimitives = Section->IndexBuffer.Indices.Num() / 3;
		BatchElement.MinVertexIndex = 0;
		BatchElement.MaxVertexIndex = Section->VertexBuffers.PositionVertexBuffer.GetNumVertices() - 1;
		Mesh.ReverseCulling = IsLocalToWorldDeterminantNegative();
		Mesh.Type = PT_TriangleList;
		Mesh.DepthPriorityGroup = SDPG_World;
		Mesh.bCanApplyViewModeOverrides = false;
		Collector.AddMesh(ViewIndex, Mesh);	
	}

	virtual FPrimitiveViewRelevance GetViewRelevance(const FSceneView* View) const {
		FPrimitiveViewRelevance Result;
		const float Distance = FVector::Distance(View->ViewMatrices.GetViewOrigin(), ZoneOrigin);
		Result.bDrawRelevance = (Distance <= CullDistance) && IsShown(View);
		Result.bShadowRelevance = IsShadowCast(View);
		Result.bDynamicRelevance = true;
		Result.bStaticRelevance = true;
		Result.bRenderInMainPass = ShouldRenderInMainPass();
		Result.bUsesLightingChannels = GetLightingChannelMask() != GetDefaultLightingChannelMask();
		Result.bRenderCustomDepth = ShouldRenderCustomDepth();
		MaterialRelevance.SetPrimitiveViewRelevance(Result);
		return Result;
	}

	virtual bool CanBeOccluded() const override {
		return !MaterialRelevance.bDisableDepthTest;
	}

	virtual uint32 GetMemoryFootprint(void) const {
		return(sizeof(*this) + GetAllocatedSize());
	}

	uint32 GetAllocatedSize(void) const {
		return(FPrimitiveSceneProxy::GetAllocatedSize());
	}
};


// ================================================================================================================================================
// UVoxelMeshComponent
// ================================================================================================================================================

UVoxelMeshComponent::UVoxelMeshComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
	bLodFlag = USBT_ENABLE_LOD;
	bUseComplexAsSimpleCollision = true;
}

FPrimitiveSceneProxy* UVoxelMeshComponent::CreateSceneProxy() {
	FVoxelMeshSceneProxy* NewProxy = new FVoxelMeshSceneProxy(this);
	MeshSectionLodArray.Empty(); // clear mesh data to reduce memory usage
	return NewProxy;
}

void UVoxelMeshComponent::PostLoad() {
	Super::PostLoad();

	if (ProcMeshBodySetup && IsTemplate()) {
		ProcMeshBodySetup->SetFlags(RF_Public);
	}
}

int32 UVoxelMeshComponent::GetNumMaterials() const {
	return LocalMaterials.Num();
}

void UVoxelMeshComponent::GetUsedMaterials(TArray<UMaterialInterface*>& OutMaterials, bool bGetDebugMaterials) const {
    UMaterialInterface* DefaultMaterial = UMaterial::GetDefaultMaterial(MD_Surface);
    OutMaterials.Add(DefaultMaterial);
	OutMaterials.Append(LocalMaterials);
}

void UVoxelMeshComponent::SetMeshData(TMeshDataPtr NewMeshDataPtr, const TTerrainLodMask TerrainLodMask) {
	ASandboxTerrainController* TerrainController = Cast<ASandboxTerrainController>(GetAttachmentRootActor());

	LocalMaterials.Empty();
	LocalMaterials.Reserve(10);

    static const auto DummyMesh = TMeshLodSection();
	MeshSectionLodArray.SetNum(LOD_ARRAY_SIZE, false);

	if (NewMeshDataPtr) {
		auto LodIndex = 0;
		for (auto& SectionLOD : NewMeshDataPtr->MeshSectionLodArray) {
            const auto* SourceMesh = &SectionLOD;
            bool bIgnoreLod = TerrainLodMask & (1 << LodIndex);
            if(bIgnoreLod){
                SourceMesh = &DummyMesh;
            }
            
			MeshSectionLodArray[LodIndex].WholeMesh = SourceMesh->WholeMesh;
			MeshSectionLodArray[LodIndex].RegularMeshContainer.MaterialSectionMap = SourceMesh->RegularMeshContainer.MaterialSectionMap;
			MeshSectionLodArray[LodIndex].RegularMeshContainer.MaterialTransitionSectionMap = SourceMesh->RegularMeshContainer.MaterialTransitionSectionMap;
            
			if (TerrainController != nullptr) {
				for (auto& Element : SourceMesh->RegularMeshContainer.MaterialSectionMap) {
                    UMaterialInterface* Material = TerrainController->GetRegularTerrainMaterial(Element.Key);
					LocalMaterials.Add(Material);
				}
                
				for (const auto& Element : SourceMesh->RegularMeshContainer.MaterialTransitionSectionMap) {
					LocalMaterials.Add(TerrainController->GetTransitionTerrainMaterial(Element.Value.MaterialIdSet));
				}
			}
			if (bLodFlag) {
				for (auto i = 0; i < 6; i++) {
					MeshSectionLodArray[LodIndex].TransitionPatchArray[i].MaterialSectionMap = SourceMesh->TransitionPatchArray[i].MaterialSectionMap;
					MeshSectionLodArray[LodIndex].TransitionPatchArray[i].MaterialTransitionSectionMap = SourceMesh->TransitionPatchArray[i].MaterialTransitionSectionMap;
                    
                    if (TerrainController != nullptr) {
                        for (auto& Element : SourceMesh->TransitionPatchArray[i].MaterialSectionMap) {
                            UMaterialInterface* Material = TerrainController->GetRegularTerrainMaterial(Element.Key);
                            LocalMaterials.Add(Material);
                        }
                        
                        for (const auto& Element : SourceMesh->TransitionPatchArray[i].MaterialTransitionSectionMap) {
                            LocalMaterials.Add(TerrainController->GetTransitionTerrainMaterial(Element.Value.MaterialIdSet));
                        }
                    }
                    
				}
			}

			LodIndex++;
		}
	}

	UpdateLocalBounds(); // Update overall bounds
	MarkRenderStateDirty(); // New section requires recreating scene proxy
}

FBoxSphereBounds UVoxelMeshComponent::CalcBounds(const FTransform& LocalToWorld) const {
	return LocalBounds.TransformBy(LocalToWorld);
}

// ======================================================================
// collision 
// ======================================================================

void UVoxelMeshComponent::AddCollisionSection(struct FTriMeshCollisionData* CollisionData, const FProcMeshSection& MeshSection, const int32 MatId, const int32 VertexBase) {

	// Copy vert data
	for (int32 VertIdx = 0; VertIdx < MeshSection.ProcVertexBuffer.Num(); VertIdx++) {
		FProcMeshVertex Vertex = MeshSection.ProcVertexBuffer[VertIdx];
		FVector Position(Vertex.PositionX, Vertex.PositionY, Vertex.PositionZ);
		CollisionData->Vertices.Add((FVector3f)Position);

		// Copy UV if desired
		//if (bCopyUVs) {
		//	CollisionData->UVs[0].Add(TriMeshData.ProcVertexBuffer[VertIdx].UV0);
		//}
	}

	// Copy triangle data
	const int32 NumTriangles = MeshSection.ProcIndexBuffer.Num() / 3;
	for (int32 TriIdx = 0; TriIdx < NumTriangles; TriIdx++) {
		// Need to add base offset for indices
		FTriIndices Triangle;
		Triangle.v0 = MeshSection.ProcIndexBuffer[(TriIdx * 3) + 0] + VertexBase;
		Triangle.v1 = MeshSection.ProcIndexBuffer[(TriIdx * 3) + 1] + VertexBase;
		Triangle.v2 = MeshSection.ProcIndexBuffer[(TriIdx * 3) + 2] + VertexBase;
		CollisionData->Indices.Add(Triangle);

		// Also store material info
		CollisionData->MaterialIndices.Add(MatId);
	}
}

bool UVoxelMeshComponent::GetPhysicsTriMeshData(struct FTriMeshCollisionData* CollisionData, bool InUseAllTriData) {
	int32 VertexBase = 0; 
						  
	bool bCopyUVs = UPhysicsSettings::Get()->bSupportUVFromHitResults;
	if (bCopyUVs) {
		CollisionData->UVs.AddZeroed(1); // only one UV channel
	}

	const TMeshLodSection& CollisionSection = CollisionLodSection;
	for (const auto& Elem : CollisionSection.RegularMeshContainer.MaterialSectionMap) {
		int32 MatId = (int32)Elem.Key;
		const TMeshMaterialSection& MaterialSection = Elem.Value;
		const FProcMeshSection& MeshSection = MaterialSection.MaterialMesh;
		AddCollisionSection(CollisionData, MeshSection, MatId, VertexBase);

		// Remember the base index that new verts will be added from in next section
		VertexBase = CollisionData->Vertices.Num();
	}

	for (const auto& Elem : CollisionSection.RegularMeshContainer.MaterialTransitionSectionMap) {
		int32 MatId = (int32)Elem.Key;
		const TMeshMaterialTransitionSection& MaterialSection = Elem.Value;
		const FProcMeshSection& MeshSection = MaterialSection.MaterialMesh;
		AddCollisionSection(CollisionData, MeshSection, MatId, VertexBase);

		// Remember the base index that new verts will be added from in next section
		VertexBase = CollisionData->Vertices.Num();
	}

	CollisionData->bFlipNormals = true;
	CollisionData->bDeformableMesh = true;
	CollisionData->bFastCook = true;
	return true;
}

void UVoxelMeshComponent::UpdateLocalBounds() {
	//FBox LocalBox(EForceInit::ForceInitToZero);
	//CollisionLodSection.WholeMesh.SectionLocalBox;
	//LocalBox += CollisionLodSection.WholeMesh.SectionLocalBox;
	//LocalBounds = LocalBox.IsValid ? FBoxSphereBounds(LocalBox) : FBoxSphereBounds(FVector(0, 0, 0), FVector(500, 500, 500), 500); // fallback to reset box sphere bounds

	const FVector BoxExt(USBT_ZONE_SIZE / 2.f);
	const double Radius = BoxExt.Length();
	LocalBounds = FBoxSphereBounds(FVector(0), BoxExt, Radius);

	UpdateBounds();
	MarkRenderTransformDirty();
}


bool UVoxelMeshComponent::ContainsPhysicsTriMeshData(bool InUseAllTriData) const {
	if (CollisionLodSection.RegularMeshContainer.MaterialSectionMap.Num() == 0) {
		return false;
	}

	return true;
}

void UVoxelMeshComponent::CreateProcMeshBodySetup() {
	if (!ProcMeshBodySetup) {
		// The body setup in a template needs to be public since the property is Tnstanced and thus is the archetype of the instance meaning there is a direct reference
		ProcMeshBodySetup = NewObject<UBodySetup>(this, NAME_None, (IsTemplate() ? RF_Public : RF_NoFlags));
		ProcMeshBodySetup->BodySetupGuid = FGuid::NewGuid();
		ProcMeshBodySetup->bGenerateMirroredCollision = false;
		ProcMeshBodySetup->bDoubleSidedGeometry = true;
		ProcMeshBodySetup->CollisionTraceFlag = bUseComplexAsSimpleCollision ? CTF_UseComplexAsSimple : CTF_UseDefault;
	}
}

void UVoxelMeshComponent::AddCollisionConvexMesh(TArray<FVector> ConvexVerts) {
	if (ConvexVerts.Num() >= 4) {
		// New element
		FKConvexElem NewConvexElem;
		// Copy in vertex info
		NewConvexElem.VertexData = ConvexVerts;
		// Update bounding box
		NewConvexElem.ElemBox = FBox(NewConvexElem.VertexData);
		// Add to array of convex elements
		CollisionConvexElems.Add(NewConvexElem);
		// Refresh collision
		UpdateCollision();
	}
}

UBodySetup* UVoxelMeshComponent::CreateBodySetupHelper() {
	// The body setup in a template needs to be public since the property is Tnstanced and thus is the archetype of the instance meaning there is a direct reference
	UBodySetup* NewBodySetup = NewObject<UBodySetup>(this, NAME_None, (IsTemplate() ? RF_Public : RF_NoFlags));
	NewBodySetup->BodySetupGuid = FGuid::NewGuid();

	NewBodySetup->bGenerateMirroredCollision = false;
	NewBodySetup->bDoubleSidedGeometry = true;
	NewBodySetup->CollisionTraceFlag = bUseComplexAsSimpleCollision ? CTF_UseComplexAsSimple : CTF_UseDefault;

	return NewBodySetup;
}

void UVoxelMeshComponent::FinishPhysicsAsyncCook(bool bSuccess, UBodySetup* FinishedBodySetup) {
	TArray<UBodySetup*> NewQueue;
	NewQueue.Reserve(AsyncBodySetupQueue.Num());

	int32 FoundIdx;
	if (AsyncBodySetupQueue.Find(FinishedBodySetup, FoundIdx)) {
		if (bSuccess) {
			//The new body was found in the array meaning it's newer so use it
			ProcMeshBodySetup = FinishedBodySetup;
			RecreatePhysicsState();

			//remove any async body setups that were requested before this one
			for (int32 AsyncIdx = FoundIdx + 1; AsyncIdx < AsyncBodySetupQueue.Num(); ++AsyncIdx) {
				NewQueue.Add(AsyncBodySetupQueue[AsyncIdx]);
			}

			AsyncBodySetupQueue = NewQueue;
		} else {
			AsyncBodySetupQueue.RemoveAt(FoundIdx);
		}
	}

	UpdateNavigationData();

	ASandboxTerrainController* TerrainController = Cast<ASandboxTerrainController>(GetAttachmentRootActor());
	if (TerrainController) {
		TerrainController->OnFinishAsyncPhysicsCook(ZoneIndex);
	}
}

void UVoxelMeshComponent::UpdateCollision() {
	// Abort all previous ones still standing
	for (UBodySetup* OldBody : AsyncBodySetupQueue) {
		OldBody->AbortPhysicsMeshAsyncCreation();
	}

	AsyncBodySetupQueue.Add(CreateBodySetupHelper());
	UBodySetup* UseBodySetup = AsyncBodySetupQueue.Last();

	// Fill in simple collision convex elements
	UseBodySetup->AggGeom.ConvexElems = CollisionConvexElems;

	// Set trace flag
	UseBodySetup->CollisionTraceFlag = bUseComplexAsSimpleCollision ? CTF_UseComplexAsSimple : CTF_UseDefault;
	UseBodySetup->CreatePhysicsMeshesAsync(FOnAsyncPhysicsCookFinished::CreateUObject(this, &UVoxelMeshComponent::FinishPhysicsAsyncCook, UseBodySetup));
}

UBodySetup* UVoxelMeshComponent::GetBodySetup() {
	CreateProcMeshBodySetup();
	return ProcMeshBodySetup;
}

void UVoxelMeshComponent::SetCollisionMeshData(TMeshDataPtr MeshDataPtr) {
	CollisionLodSection = MeshSectionLodArray[0]; 
	//UpdateLocalBounds();
	UpdateCollision();
}


TMaterialId UVoxelMeshComponent::GetMaterialIdFromCollisionFaceIndex(int32 FaceIndex) const {
	if (FaceIndex >= 0) {
		int32 TotalFaceCount = 0;
		const TMeshLodSection& CollisionSection = CollisionLodSection;
		for (const auto& Elem : CollisionSection.RegularMeshContainer.MaterialSectionMap) {
			int32 MatId = (int32)Elem.Key;
			const TMeshMaterialSection& MaterialSection = Elem.Value;
			const FProcMeshSection& MeshSection = MaterialSection.MaterialMesh;

			int32 NumFaces = MeshSection.ProcIndexBuffer.Num() / 3;
			TotalFaceCount += NumFaces;

			if (FaceIndex < TotalFaceCount) {
				return MaterialSection.MaterialId;
			}

		}
	}

	return 0;
}