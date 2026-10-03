#include "VoidPrepareCommandlet.h"
#include "VoidField.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetImportTask.h"
#include "AssetToolsModule.h"
#include "IAssetTools.h"
#include "AssetCompilingManager.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "Factories/TextureFactory.h"
#include "MeshDescription.h"
#include "StaticMeshAttributes.h"
#include "PhysicsEngine/BodySetup.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceConstant.h"
#include "Materials/MaterialExpressionConstant.h"
#include "Materials/MaterialExpressionConstant3Vector.h"
#include "Materials/MaterialExpressionTextureSample.h"
#include "Materials/MaterialExpressionTextureCoordinate.h"
#include "Materials/MaterialExpressionLinearInterpolate.h"
#include "Materials/MaterialExpressionVertexColor.h"
#include "Materials/MaterialExpressionFresnel.h"
#include "Materials/MaterialExpressionPanner.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "Misc/Parse.h"
#include "Misc/FileHelper.h"
#include "HAL/FileManager.h"
#include "Serialization/JsonSerializer.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"

namespace {
bool Save(UObject* Asset) {
    if(!Asset) return false;
    UPackage* P=Asset->GetOutermost(); P->MarkPackageDirty();
    const FString Filename=FPackageName::LongPackageNameToFilename(P->GetName(),FPackageName::GetAssetPackageExtension());
    IFileManager::Get().MakeDirectory(*FPaths::GetPath(Filename),true);
    FSavePackageArgs Args; Args.TopLevelFlags=RF_Public|RF_Standalone; Args.SaveFlags=SAVE_NoError;
    return UPackage::SavePackage(P,Asset,*Filename,Args);
}
template<typename T> T* Expr(UMaterial* M) {
    T* E=NewObject<T>(M); M->GetExpressionCollection().AddExpression(E); return E;
}
UTexture2D* Texture(const FString& File,const FString& Name,TextureCompressionSettings Compression,bool SRGB) {
    const FString Folder=TEXT("/Game/VOID/Textures");
    if(auto* T=LoadObject<UTexture2D>(nullptr,*(Folder+TEXT("/")+Name+TEXT(".")+Name))) return T;
    UAssetImportTask* Task=NewObject<UAssetImportTask>();
    Task->Filename=FPaths::ProjectDir()/TEXT("ArtSource")/File;
    Task->DestinationPath=Folder; Task->DestinationName=Name;
    Task->bAutomated=true; Task->bAsync=false; Task->bReplaceExisting=false; Task->bSave=false;
    auto* Factory=NewObject<UTextureFactory>(); Factory->SuppressImportOverwriteDialog();
    Factory->CompressionSettings=Compression;
    Factory->ColorSpaceMode=SRGB ? ETextureSourceColorSpace::SRGB : ETextureSourceColorSpace::Linear;
    Factory->bDeferCompression=true; Task->Factory=Factory;
    FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools")).Get().ImportAssetTasks({Task});
    // Finish the factory's source build before changing texture settings; the
    // UE 5.8 source gamma contract must remain stable while a worker reads it.
    FAssetCompilingManager::Get().FinishAllCompilation();
    for(UObject* O:Task->GetObjects()) if(auto* T=Cast<UTexture2D>(O)) {
        T->CompressionSettings=Compression; T->SRGB=SRGB; T->LODGroup=TEXTUREGROUP_World;
        T->PostEditChange(); return Save(T)?T:nullptr;
    }
    UE_LOG(LogTemp,Error,TEXT("VOID_TEXTURE_IMPORT_FAILED %s"),*File); return nullptr;
}
UMaterialExpressionTextureSample* Sample(UMaterial* M,UTexture2D* T,EMaterialSamplerType Type,UMaterialExpression* UV=nullptr) {
    auto* E=Expr<UMaterialExpressionTextureSample>(M); E->Texture=T; E->SamplerType=Type;
    if(UV) E->Coordinates.Expression=UV; return E;
}
UMaterialExpressionLinearInterpolate* Blend(UMaterial* M,UMaterialExpression* A,UMaterialExpression* B,UMaterialExpression* Weight,int32 Index) {
    auto* L=Expr<UMaterialExpressionLinearInterpolate>(M); L->A.Expression=A; L->B.Expression=B;
    L->Alpha.Expression=Weight; L->Alpha.OutputIndex=Index; return L;
}
UMaterial* TerrainMaterial(const TSharedPtr<FJsonObject>& Manifest) {
    if(auto* Old=LoadObject<UMaterial>(nullptr,TEXT("/Game/VOID/M_Terrain.M_Terrain"))) {
        bool Changed=false; Old->SetMaterialUsage(Changed,MATUSAGE_Nanite);
        Old->PostEditChange(); return Save(Old)?Old:nullptr;
    }
    UMaterial* M=NewObject<UMaterial>(CreatePackage(TEXT("/Game/VOID/M_Terrain")),TEXT("M_Terrain"),RF_Public|RF_Standalone);
    auto* Weights=Expr<UMaterialExpressionVertexColor>(M);
    TArray<UMaterialExpression*> D,N,R;
    for(const auto& V:Manifest->GetArrayField(TEXT("terrain"))) {
        auto O=V->AsObject(); const FString ID=O->GetStringField(TEXT("id"));
        auto* Diff=Texture(O->GetStringField(TEXT("diffuse")),TEXT("T_")+ID+TEXT("_D"),TC_Default,true);
        auto* Normal=Texture(O->GetStringField(TEXT("normal")),TEXT("T_")+ID+TEXT("_N"),TC_Normalmap,false);
        auto* Rough=Texture(O->GetStringField(TEXT("roughness")),TEXT("T_")+ID+TEXT("_R"),TC_Masks,false);
        if(!Diff||!Normal||!Rough) return nullptr;
        D.Add(Sample(M,Diff,SAMPLERTYPE_Color)); N.Add(Sample(M,Normal,SAMPLERTYPE_Normal)); R.Add(Sample(M,Rough,SAMPLERTYPE_Masks));
    }
    if(D.Num()!=3) return nullptr;
    // Manifest order: sand, grass, rock. UVs are generated in world metres.
    M->GetEditorOnlyData()->BaseColor.Expression=Blend(M,Blend(M,D[0],D[1],Weights,2),D[2],Weights,1);
    M->GetEditorOnlyData()->Normal.Expression=Blend(M,Blend(M,N[0],N[1],Weights,2),N[2],Weights,1);
    M->GetEditorOnlyData()->Roughness.Expression=Blend(M,Blend(M,R[0],R[1],Weights,2),R[2],Weights,1);
    M->PreEditChange(nullptr); M->PostEditChange();
    bool Changed=false; M->SetMaterialUsage(Changed,MATUSAGE_Nanite);
    M->PreEditChange(nullptr); M->PostEditChange(); FAssetRegistryModule::AssetCreated(M);
    return Save(M)?M:nullptr;
}
UMaterial* WaterMaterial() {
    if(auto* Old=LoadObject<UMaterial>(nullptr,TEXT("/Game/VOID/M_Water.M_Water"))) return Old;
    UMaterial* M=NewObject<UMaterial>(CreatePackage(TEXT("/Game/VOID/M_Water")),TEXT("M_Water"),RF_Public|RF_Standalone);
    M->BlendMode=BLEND_Translucent; M->TwoSided=false;
    M->TranslucencyLightingMode=TLM_SurfacePerPixelLighting;
    auto* C=Expr<UMaterialExpressionConstant3Vector>(M); C->Constant=FLinearColor(.018f,.105f,.13f);
    auto* R=Expr<UMaterialExpressionConstant>(M); R->R=.09f;
    auto* F=Expr<UMaterialExpressionFresnel>(M); F->Exponent=4; F->BaseReflectFraction=.02f;
    auto* O=Expr<UMaterialExpressionLinearInterpolate>(M); O->ConstA=.45f; O->ConstB=.94f; O->Alpha.Expression=F;
    auto* UV=Expr<UMaterialExpressionTextureCoordinate>(M); UV->UTiling=42; UV->VTiling=36;
    auto* Pan=Expr<UMaterialExpressionPanner>(M); Pan->Coordinate.Expression=UV; Pan->SpeedX=.006f; Pan->SpeedY=.003f;
    auto* Norm=LoadObject<UTexture2D>(nullptr,TEXT("/Game/VOID/Textures/T_sand_03_N.T_sand_03_N"));
    if(Norm) M->GetEditorOnlyData()->Normal.Expression=Sample(M,Norm,SAMPLERTYPE_Normal,Pan);
    M->GetEditorOnlyData()->BaseColor.Expression=C;
    M->GetEditorOnlyData()->Roughness.Expression=R;
    M->GetEditorOnlyData()->Opacity.Expression=O;
    M->PreEditChange(nullptr); M->PostEditChange(); FAssetRegistryModule::AssetCreated(M);
    return Save(M)?M:nullptr;
}
UMaterial* FernMaterial(const TSharedPtr<FJsonObject>& Model) {
    if(auto* Old=LoadObject<UMaterial>(nullptr,TEXT("/Game/VOID/M_Fern.M_Fern"))) return Old;
    auto* Diff=Texture(TEXT("models/fern_02/textures/fern_02_diff_2k.jpg"),TEXT("T_Fern_D"),TC_Default,true);
    auto* Normal=Texture(TEXT("models/fern_02/textures/fern_02_nor_gl_2k.jpg"),TEXT("T_Fern_N"),TC_Normalmap,false);
    auto* ARM=Texture(TEXT("models/fern_02/textures/fern_02_arm_2k.jpg"),TEXT("T_Fern_ARM"),TC_Masks,false);
    auto* Alpha=Texture(Model->GetStringField(TEXT("opacity_mask")),TEXT("T_Fern_A"),TC_Masks,false);
    if(!Diff||!Normal||!ARM||!Alpha) return nullptr;
    Normal->bFlipGreenChannel=true; Normal->PostEditChange(); if(!Save(Normal)) return nullptr;
    UMaterial* M=NewObject<UMaterial>(CreatePackage(TEXT("/Game/VOID/M_Fern")),TEXT("M_Fern"),RF_Public|RF_Standalone);
    M->BlendMode=BLEND_Masked; M->TwoSided=true; M->OpacityMaskClipValue=.4f;
    M->SetShadingModel(MSM_TwoSidedFoliage);
    M->GetEditorOnlyData()->BaseColor.Expression=Sample(M,Diff,SAMPLERTYPE_Color);
    M->GetEditorOnlyData()->Normal.Expression=Sample(M,Normal,SAMPLERTYPE_Normal);
    M->GetEditorOnlyData()->Roughness.Expression=Sample(M,ARM,SAMPLERTYPE_Masks);
    M->GetEditorOnlyData()->Roughness.OutputIndex=2; // ARM green is roughness.
    M->GetEditorOnlyData()->OpacityMask.Expression=Sample(M,Alpha,SAMPLERTYPE_Masks);
    M->GetEditorOnlyData()->OpacityMask.OutputIndex=1; // Standalone mask red.
    auto* Subsurface=Expr<UMaterialExpressionConstant3Vector>(M); Subsurface->Constant=FLinearColor(.12f,.19f,.04f);
    M->GetEditorOnlyData()->SubsurfaceColor.Expression=Subsurface;
    // Populate the expression/texture cache before a usage change compiles it.
    M->PreEditChange(nullptr); M->PostEditChange();
    bool Changed=false; M->SetMaterialUsage(Changed,MATUSAGE_Nanite); M->SetMaterialUsage(Changed,MATUSAGE_InstancedStaticMeshes);
    M->PreEditChange(nullptr); M->PostEditChange(); FAssetRegistryModule::AssetCreated(M);
    return Save(M)?M:nullptr;
}
UStaticMesh* FieldMesh(const FVoidSoul& Soul,UMaterial* Material,bool Water=false,bool Rebuild=false) {
    const FString Package=Water?TEXT("/Game/VOID/Water"):TEXT("/Game/VOID/Soul_")+Soul.ID+TEXT("/Terrain");
    const FString Name=Water?TEXT("Water"):TEXT("Terrain");
    UStaticMesh* M=LoadObject<UStaticMesh>(nullptr,*(Package+TEXT(".")+Name));
    if(M && !Rebuild) return M;
    if(!M) M=NewObject<UStaticMesh>(CreatePackage(*Package),*Name,RF_Public|RF_Standalone);
    M->GetStaticMaterials().Reset();
    M->SetNumSourceModels(1); M->GetStaticMaterials().Add(FStaticMaterial(Material,TEXT("Surface"),TEXT("Surface")));
    FMeshDescription* Mesh=M->CreateMeshDescription(0,FMeshDescription());
    FStaticMeshAttributes A(*Mesh); A.Register(); A.GetVertexInstanceUVs().SetNumChannels(1);
    auto Pos=A.GetVertexPositions(); auto Norm=A.GetVertexInstanceNormals(); auto UV=A.GetVertexInstanceUVs();
    auto Color=A.GetVertexInstanceColors(); auto Tangent=A.GetVertexInstanceTangents(); auto Sign=A.GetVertexInstanceBinormalSigns();
    const FPolygonGroupID PG=Mesh->CreatePolygonGroup(); A.GetPolygonGroupMaterialSlotNames()[PG]=TEXT("Surface");
    const int32 N=Water?2:VoidField::Grid;
    TArray<FVertexID> Vertices; Vertices.Reserve(N*N);
    for(int32 Y=0;Y<N;Y++) for(int32 X=0;X<N;X++) {
        const double WX=Water?(-9000.0+X*18000.0):(-VoidField::HalfSize+X*2*VoidField::HalfSize/(N-1));
        const double WY=Water?(-7200.0+Y*14400.0):(-VoidField::HalfSize+Y*2*VoidField::HalfSize/(N-1));
        const FVertexID V=Mesh->CreateVertex(); Pos[V]=FVector3f(WX,WY,Water?0:VoidField::Height(WX,WY,Soul)); Vertices.Add(V);
    }
    auto Triangle=[&](int32 I,int32 J,int32 K) {
        TArray<FVertexInstanceID> VI;
        for(int32 V:{I,J,K}) {
            auto Instance=Mesh->CreateVertexInstance(Vertices[V]); VI.Add(Instance);
            FVector3f P=Pos[Vertices[V]];
            FVector Normal=Water?FVector::UpVector:VoidField::Normal(P.X,P.Y,Soul);
            Norm[Instance]=FVector3f(Normal);
            Tangent[Instance]=FVector3f((FVector::ForwardVector-Normal*Normal.X).GetSafeNormal()); Sign[Instance]=1;
            UV.Set(Instance,0,Water?FVector2f((P.X+9000)/18000,(P.Y+7200)/14400):FVector2f(P.X/300,P.Y/300));
            Color[Instance]=Water?FVector4f(1,1,1,1):VoidField::Color(P.X,P.Y,Soul);
        }
        Mesh->CreatePolygon(PG,VI);
    };
    for(int32 Y=0;Y<N-1;Y++) for(int32 X=0;X<N-1;X++) {
        // UE's left-handed front-face convention and its physics normal flip
        // require this order; explicit shading normals still point Z-up.
        int32 V=Y*N+X; Triangle(V,V+N,V+1); Triangle(V+1,V+N,V+N+1);
    }
    M->GetSourceModel(0).BuildSettings.bRecomputeNormals=false;
    M->GetSourceModel(0).BuildSettings.bRecomputeTangents=false;
    M->GetSourceModel(0).BuildSettings.bGenerateLightmapUVs=false;
    M->NaniteSettings.bEnabled=!Water;
    // Retain an exact fallback for CPU collision; Nanite reduction is visual only.
    M->NaniteSettings.FallbackTarget=ENaniteFallbackTarget::PercentTriangles;
    M->NaniteSettings.FallbackPercentTriangles=1.0f;
    M->NaniteSettings.FallbackRelativeError=0.0f;
    M->CommitMeshDescription(0);
    TArray<FText> BuildErrors; M->Build(false,&BuildErrors);
    if(!BuildErrors.IsEmpty()) return nullptr;
    FAssetCompilingManager::Get().FinishAllCompilation();
    M->CreateBodySetup(); M->GetBodySetup()->CollisionTraceFlag=CTF_UseComplexAsSimple;
    M->GetBodySetup()->bDoubleSidedGeometry=false;
    M->GetBodySetup()->InvalidatePhysicsData();
    M->GetBodySetup()->CreatePhysicsMeshes();
    UE_LOG(LogTemp,Display,TEXT("VOID_COLLISION_READY %s bodies=%d has_triangles=%d"),*Package,M->GetBodySetup()->TriMeshGeometries.Num(),M->ContainsPhysicsTriMeshData(true));
    FAssetRegistryModule::AssetCreated(M);
    if(!Save(M)) return nullptr;
    UE_LOG(LogTemp,Display,TEXT("VOID_MESH_READY %s vertices=%d"),*Package,N*N); return M;
}
bool Models(const TSharedPtr<FJsonObject>& Manifest) {
    TSharedRef<FJsonObject> Result=MakeShared<FJsonObject>();
    auto& Registry=FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();
    auto& Tools=FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools")).Get();
    for(const auto& V:Manifest->GetArrayField(TEXT("models"))) {
        auto O=V->AsObject(); const FString ID=O->GetStringField(TEXT("id"));
        UMaterial* Fern=ID==TEXT("fern_02")?FernMaterial(O):nullptr;
        if(ID==TEXT("fern_02") && !Fern) return false;
        const FString Folder=TEXT("/Game/VOID/Models/")+ID;
        TArray<FAssetData> Assets; Registry.GetAssetsByPath(*Folder,Assets,true);
        if(Assets.IsEmpty()) {
            UAssetImportTask* T=NewObject<UAssetImportTask>();
            T->Filename=FPaths::ProjectDir()/TEXT("ArtSource")/O->GetStringField(TEXT("file"));
            T->DestinationPath=Folder; T->bAutomated=true; T->bAsync=false; T->bReplaceExisting=false; T->bSave=true;
            Tools.ImportAssetTasks({T});
            FAssetCompilingManager::Get().FinishAllCompilation();
            Registry.GetAssetsByPath(*Folder,Assets,true);
        }
        Assets.Sort([](const FAssetData& A,const FAssetData& B){return A.GetObjectPathString()<B.GetObjectPathString();});
        TArray<TSharedPtr<FJsonValue>> Meshes;
        for(const FAssetData& A:Assets) if(auto* M=Cast<UStaticMesh>(A.GetAsset())) {
            M->NaniteSettings.bEnabled=true;
            M->NaniteSettings.FallbackRelativeError=1.0f;
            M->GetSourceModel(0).BuildSettings.bGenerateLightmapUVs=false;
            for(auto& S:M->GetStaticMaterials()) if(S.MaterialInterface) {
                if(Fern) S.MaterialInterface=Fern;
                if(auto* MI=Cast<UMaterialInstanceConstant>(S.MaterialInterface)) {
                    // Imported glTF instances can inherit a plugin material.
                    // Create a project-owned parent before enabling Nanite and
                    // instancing; never change the engine/plugin source asset.
                    UMaterial* Parent=MI->GetMaterial();
                    if(Parent && !Parent->GetOutermost()->GetName().StartsWith(TEXT("/Game/VOID/"))) {
                        const FString ParentName=TEXT("M_Native_")+Parent->GetName();
                        const FString ParentPackage=TEXT("/Game/VOID/Materials/")+ParentName;
                        UMaterial* Own=LoadObject<UMaterial>(nullptr,*(ParentPackage+TEXT(".")+ParentName));
                        if(!Own) { Own=DuplicateObject<UMaterial>(Parent,CreatePackage(*ParentPackage),*ParentName); Own->SetFlags(RF_Public|RF_Standalone); FAssetRegistryModule::AssetCreated(Own); }
                        MI->SetParentEditorOnly(Own);
                    }
                    if(ID==TEXT("tree_small_02") && MI->GetOutermost()->GetName().StartsWith(TEXT("/Game/VOID/"))) {
                        MI->BasePropertyOverrides.bOverride_BlendMode=true;
                        MI->BasePropertyOverrides.BlendMode=BLEND_Opaque;
                        MI->BasePropertyOverrides.bOverride_TwoSided=true;
                        MI->BasePropertyOverrides.TwoSided=true;
                        MI->PostEditChange(); Save(MI);
                    }
                }
                UMaterial* Mat=S.MaterialInterface->GetMaterial();
                if(Mat && Mat->GetOutermost()->GetName().StartsWith(TEXT("/Game/VOID/"))) {
                    bool Changed=false;
                    Mat->SetMaterialUsage(Changed,MATUSAGE_Nanite);
                    Mat->SetMaterialUsage(Changed,MATUSAGE_InstancedStaticMeshes);
                    Mat->PostEditChange(); Save(Mat); S.MaterialInterface->PostEditChange(); Save(S.MaterialInterface);
                }
                if(Mat && Mat->GetOutermost()->GetName().StartsWith(TEXT("/Game/VOID/")) && ID==TEXT("tree_small_02")) {
                    Mat->BlendMode=BLEND_Opaque; Mat->TwoSided=true; Mat->PostEditChange(); Save(Mat);
                }
            }
            M->Build(false); M->CreateBodySetup(); M->GetBodySetup()->CollisionTraceFlag=CTF_UseComplexAsSimple;
            M->GetBodySetup()->CreatePhysicsMeshes();
            if(!Save(M)) return false;
            Meshes.Add(MakeShared<FJsonValueString>(M->GetPathName()));
        }
        if(Meshes.IsEmpty()) { UE_LOG(LogTemp,Error,TEXT("VOID_MODEL_IMPORT_FAILED %s"),*ID); return false; }
        Result->SetArrayField(ID,Meshes);
        UE_LOG(LogTemp,Display,TEXT("VOID_MODELS_READY %s count=%d"),*ID,Meshes.Num());
    }
    FString Raw; FJsonSerializer::Serialize(Result,TJsonWriterFactory<>::Create(&Raw));
    return FFileHelper::SaveStringToFile(Raw,*(FPaths::ProjectSavedDir()/TEXT("VOID-imports.json")),FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
}
}
UVoidPrepareCommandlet::UVoidPrepareCommandlet() { IsClient=false; IsServer=false; IsEditor=true; LogToConsole=true; }
int32 UVoidPrepareCommandlet::Main(const FString& Params) {
    FString SoulFile=FPaths::ProjectSavedDir()/TEXT("VOID-active-soul.json");
    FParse::Value(*Params,TEXT("VoidSoul="),SoulFile);
    FVoidSoul Soul; if(!Soul.Load(SoulFile)) return 2;
    FString Raw; TSharedPtr<FJsonObject> Manifest;
    if(!FFileHelper::LoadFileToString(Raw,*(FPaths::ProjectDir()/TEXT("ArtSource/ASSETS.json"))) || !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Raw),Manifest)) return 3;
    IFileManager::Get().MakeDirectory(*FPaths::ProjectSavedDir(),true);
    UMaterial* Ground=TerrainMaterial(Manifest);
    UMaterial* Lake=Ground?WaterMaterial():nullptr;
    const bool Rebuild=FParse::Param(*Params,TEXT("RebuildTerrain"));
    if(!Ground||!Lake||!FieldMesh(Soul,Ground,false,Rebuild)||!FieldMesh(Soul,Lake,true,Rebuild)||!Models(Manifest)) return 4;
    FAssetCompilingManager::Get().FinishAllCompilation();
    const FString Receipt=FString::Printf(TEXT("{\"soul_id\":\"%s\",\"terrain_metres\":[600,600],\"grid\":[257,257],\"source\":\"own_soul_height_field\",\"state\":\"ASSETS_PREPARED\"}"),*Soul.ID);
    if(!FFileHelper::SaveStringToFile(Receipt,*(FPaths::ProjectSavedDir()/TEXT("VOID-prepared.json")),FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM)) return 5;
    UE_LOG(LogTemp,Display,TEXT("VOID_PREPARED soul=%s"),*Soul.ID); return 0;
}
