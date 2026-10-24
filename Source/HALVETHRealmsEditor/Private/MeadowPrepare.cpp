// Copyright (c) 2026 Juri Janovski. SPDX-License-Identifier: LicenseRef-HALVETH-PIRL-2.0
// Existing MIT asset-persistence and MeshDescription setup patterns retain
// their earlier MIT permissions; only original new Meadow additions are PIRL.
// See LICENSES.md and the unchanged LICENSE for the overlapping portions.
#include "MeadowPrepare.h"
#include "MeadowModel.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetCompilingManager.h"
#include "AssetImportTask.h"
#include "AssetToolsModule.h"
#include "IAssetTools.h"
#include "Factories/TextureFactory.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "MeshDescription.h"
#include "StaticMeshAttributes.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionCustom.h"
#include "Materials/MaterialExpressionVertexColor.h"
#include "Materials/MaterialExpressionWorldPosition.h"
#include "Materials/MaterialExpressionCameraPositionWS.h"
#include "Materials/MaterialExpressionLocalPosition.h"
#include "Materials/MaterialExpressionPerInstanceCustomData.h"
#include "Materials/MaterialExpressionTime.h"
#include "Materials/MaterialExpressionTextureSample.h"
#include "Materials/MaterialExpressionTextureCoordinate.h"
#include "Materials/MaterialExpressionConstant.h"
#include "Materials/MaterialExpressionMultiply.h"
#include "HAL/FileManager.h"
#include "Misc/PackageName.h"
#include "UObject/SavePackage.h"

namespace {
template<class T> T* Node(UMaterial* M){auto* E=NewObject<T>(M);M->GetExpressionCollection().AddExpression(E);return E;}
bool Save(UObject* O){
    auto* P=O->GetOutermost();P->MarkPackageDirty();
    const FString File=FPackageName::LongPackageNameToFilename(P->GetName(),FPackageName::GetAssetPackageExtension());
    IFileManager::Get().MakeDirectory(*FPaths::GetPath(File),true);
    FSavePackageArgs Args;Args.TopLevelFlags=RF_Public|RF_Standalone;Args.SaveFlags=SAVE_NoError;
    return UPackage::SavePackage(P,O,*File,Args);
}
UMaterial* Material(const TCHAR* Name){
    const FString Path=FString(TEXT("/Game/Garden/"))+Name;
    auto* M=LoadObject<UMaterial>(nullptr,*(Path+TEXT(".")+Name));
    if(!M)M=NewObject<UMaterial>(CreatePackage(*Path),Name,RF_Public|RF_Standalone);
    M->PreEditChange(nullptr);M->GetExpressionCollection().Empty();return M;
}
void Input(UMaterialExpressionCustom* E,const TCHAR* Name,UMaterialExpression* Value,int32 Output=0){
    FCustomInput I;I.InputName=Name;I.Input.Expression=Value;I.Input.OutputIndex=Output;E->Inputs.Add(I);
}
UMaterialExpressionTextureSample* Sample(UMaterial* M,const TCHAR* Name,EMaterialSamplerType Type,UMaterialExpression* UV){
    auto* E=Node<UMaterialExpressionTextureSample>(M);
    const FString P=FString(TEXT("/Game/VOID/Textures/"))+Name;
    E->Texture=LoadObject<UTexture2D>(nullptr,*(P+TEXT(".")+Name));E->SamplerType=Type;E->Coordinates.Expression=UV;return E;
}
UTexture2D* ForestTexture(const TCHAR* Name,const TCHAR* Suffix,TextureCompressionSettings Compression,bool SRGB){
    const FString Path=FString(TEXT("/Game/Garden/"))+Name;
    if(auto* Existing=LoadObject<UTexture2D>(nullptr,*(Path+TEXT(".")+Name)))return Existing;
    auto* Task=NewObject<UAssetImportTask>();
    Task->Filename=FPaths::ProjectDir()/TEXT("ArtSource/PolyHaven/forest_ground_04")/(FString(TEXT("forest_ground_04_"))+Suffix+TEXT("_2k.png"));
    Task->DestinationPath=TEXT("/Game/Garden");Task->DestinationName=Name;Task->bAutomated=true;Task->bSave=false;
    auto* Factory=NewObject<UTextureFactory>();Factory->SuppressImportOverwriteDialog();Factory->CompressionSettings=Compression;
    Factory->ColorSpaceMode=SRGB?ETextureSourceColorSpace::SRGB:ETextureSourceColorSpace::Linear;Task->Factory=Factory;
    FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools")).Get().ImportAssetTasks({Task});
    FAssetCompilingManager::Get().FinishAllCompilation();
    for(auto* Object:Task->GetObjects())if(auto* T=Cast<UTexture2D>(Object)){
        T->SRGB=SRGB;T->CompressionSettings=Compression;T->LODGroup=TEXTUREGROUP_World;T->PostEditChange();return Save(T)?T:nullptr;
    }return nullptr;
}
UMaterial* Ground(){
    auto* M=Material(TEXT("M_LivingGround"));
    auto* W=Node<UMaterialExpressionVertexColor>(M);
    auto* World=Node<UMaterialExpressionWorldPosition>(M);
    auto* UV=Node<UMaterialExpressionTextureCoordinate>(M);UV->UTiling=1.7;UV->VTiling=1.7;
    auto* Grass=Sample(M,TEXT("T_grass_path_2_D"),SAMPLERTYPE_Color,UV);
    auto* Soil=Node<UMaterialExpressionTextureSample>(M);Soil->Coordinates.Expression=UV;Soil->SamplerType=SAMPLERTYPE_Color;
    Soil->Texture=ForestTexture(TEXT("T_MeadowSoil_D"),TEXT("diff"),TC_Default,true);
    auto* Rock=Sample(M,TEXT("T_rock_boulder_dry_D"),SAMPLERTYPE_Color,UV);
    if(!Grass->Texture||!Soil->Texture||!Rock->Texture)return nullptr;
    auto* C=Node<UMaterialExpressionCustom>(M);C->OutputType=CMOT_Float3;C->Inputs.Empty();
    Input(C,TEXT("Grass"),Grass);Input(C,TEXT("Soil"),Soil);Input(C,TEXT("Rock"),Rock);Input(C,TEXT("Weights"),W);Input(C,TEXT("World"),World);
    // Photographic detail remains in the texture, while dry grass_path colour
    // no longer turns a moist biome into a yellow sandy surface. These are
    // authored PBR albedos, not a soil chemistry or plant-growth simulation.
    C->Code=TEXT("float g=dot(Grass.rgb,float3(.2126,.7152,.0722)); float2 p=World.xy*.0067; float2 c=floor(p),f=frac(p); f=f*f*(3-2*f); float4 h=frac(sin(float4(dot(c,float2(127.1,311.7)),dot(c+float2(1,0),float2(127.1,311.7)),dot(c+float2(0,1),float2(127.1,311.7)),dot(c+1,float2(127.1,311.7))))*43758.5453); float n=lerp(lerp(h.x,h.y,f.x),lerp(h.z,h.w,f.x),f.y); float macro=.88+.07*sin(World.x*.0017+sin(World.y*.0011))+.05*sin(World.y*.0031+World.x*.0007); float3 meadow=float3(.038,.087,.015)*(.55+1.65*g)*macro; float3 earth=Soil.rgb*lerp(.96,.65,saturate(Weights.b)); float cover=saturate(Weights.g*(.66+.48*n)); return lerp(lerp(earth,meadow,cover),Rock.rgb*.72,saturate(Weights.r));");
    M->GetEditorOnlyData()->BaseColor.Expression=C;
    auto* N=Node<UMaterialExpressionTextureSample>(M);N->Coordinates.Expression=UV;N->SamplerType=SAMPLERTYPE_Normal;
    N->Texture=ForestTexture(TEXT("T_MeadowSoil_N"),TEXT("nor_dx"),TC_Normalmap,false);
    auto* Rough=Node<UMaterialExpressionTextureSample>(M);Rough->Coordinates.Expression=UV;Rough->SamplerType=SAMPLERTYPE_Masks;
    Rough->Texture=ForestTexture(TEXT("T_MeadowSoil_R"),TEXT("rough"),TC_Masks,false);
    if(!N->Texture||!Rough->Texture)return nullptr;
    M->GetEditorOnlyData()->Normal.Expression=N;M->GetEditorOnlyData()->Roughness.Expression=Rough;
    M->PostEditChange();bool Changed=false;M->SetMaterialUsage(Changed,MATUSAGE_Nanite);M->PostEditChange();
    FAssetRegistryModule::AssetCreated(M);return Save(M)?M:nullptr;
}
UMaterial* Leaf(){
    auto* M=Material(TEXT("M_MeadowLeaf"));M->BlendMode=BLEND_Opaque;M->TwoSided=true;M->SetShadingModel(MSM_TwoSidedFoliage);
    M->MaxWorldPositionOffsetDisplacement=90;
    auto* Color=Node<UMaterialExpressionVertexColor>(M);
    auto* Rough=Node<UMaterialExpressionConstant>(M);Rough->R=.82;
    auto* Subsurface=Node<UMaterialExpressionMultiply>(M);Subsurface->A.Expression=Color;Subsurface->ConstB=.42;
    M->GetEditorOnlyData()->BaseColor.Expression=Color;M->GetEditorOnlyData()->Roughness.Expression=Rough;
    M->GetEditorOnlyData()->SubsurfaceColor.Expression=Subsurface;
    auto* World=Node<UMaterialExpressionWorldPosition>(M);
    auto* Eye=Node<UMaterialExpressionCameraPositionWS>(M);
    auto* Local=Node<UMaterialExpressionLocalPosition>(M);Local->IncludedOffsets=EPositionIncludedOffsets::ExcludeOffsets;
    auto* Time=Node<UMaterialExpressionTime>(M);
    auto* Random=Node<UMaterialExpressionPerInstanceCustomData>(M);Random->DataIndex=0;
    auto* UV=Node<UMaterialExpressionTextureCoordinate>(M);
    auto* Wind=Node<UMaterialExpressionCustom>(M);Wind->OutputType=CMOT_Float3;Wind->Inputs.Empty();
    Input(Wind,TEXT("World"),World);Input(Wind,TEXT("Eye"),Eye);Input(Wind,TEXT("Local"),Local);
    Input(Wind,TEXT("Time"),Time);Input(Wind,TEXT("Phase"),Random);Input(Wind,TEXT("Height"),Color,4);
    Input(Wind,TEXT("UV"),UV);
    Wind->Code=TEXT("struct RootedWind {\n")+FString(UTF8_TO_TCHAR(HalvethMeadow::WindHLSL))+TEXT("\n}; RootedWind W; float3 sway=W.HalvethMeadowWind(World,Height,Time,UV.x+Phase*6.2831853); float f=saturate((distance(World.xy,Eye.xy)-1900)/1500); f=f*f*(3-2*f); return sway*(1-f)+float3(0,0,-max(0,Local.z)*f);");
    M->GetEditorOnlyData()->WorldPositionOffset.Expression=Wind;
    M->PostEditChange();bool Changed=false;M->SetMaterialUsage(Changed,MATUSAGE_InstancedStaticMeshes);M->PostEditChange();
    FAssetRegistryModule::AssetCreated(M);return Save(M)?M:nullptr;
}
struct MeshData {
    TArray<FVector3f> Points;TArray<FVector4f> Colors;TArray<int32> Indices;TArray<float> Phases;
    int32 Point(FVector P,FVector4f C,float Phase=0){Colors.Add(C);Phases.Add(Phase);return Points.Add(FVector3f(P));}
    void Tri(int32 A,int32 B,int32 C){Indices.Append({A,B,C});}
};
FVector V(HalvethMeadow::Vec P){return FVector(P.x,P.y,P.z);}
bool Clump(int32 Variant,UMaterial* Material){
    MeshData G;
    constexpr uint32 BladeCount=128;
    for(uint32 I=0;I<BladeCount;I++){
        const auto Blade=HalvethMeadow::MakeBlade(Variant,I);
        const float Hue=.78f+.3f*float(I%7)/6.f;
        const int32 Start=G.Points.Num();
        for(int32 J=0;J<=6;J++){
            const double T=J/6.;
            const FVector4f Color(.025f*Hue+.018f*T,.061f*Hue+.055f*T,.007f+.009f*T,float(T));
            for(int32 Side=-1;Side<=1;Side++){
                FVector P=V(HalvethMeadow::BladePoint(Blade,T,Side));
                G.Point(P,Color,float(Blade.phase));
            }
        }
        for(int32 J=0;J<6;J++)for(int32 S=0;S<2;S++){
            const int32 A=Start+J*3+S,B=A+1,C=A+3,D=C+1;
            if(J<5){G.Tri(A,C,B);G.Tri(B,C,D);}else G.Tri(A,C,B);
        }
        if(Variant>=2&&(I==7||I==29)){
            const FVector Head=V(HalvethMeadow::BladePoint(Blade,1,0));
            const FVector4f Bloom=Variant==2?FVector4f(.66f,.37f,.018f,1):FVector4f(.32f,.045f,.19f,1);
            // Petals are cupped triangles around a real stem, not a camera-facing sprite.
            const int32 Centre=G.Point(Head,FVector4f(Bloom.X*.8f,Bloom.Y*.8f,Bloom.Z*.8f,1),float(Blade.phase));
            for(int32 K=0;K<8;K++){
                const double A=K*2*PI/8,B=(K+.5)*2*PI/8,C=(K+1)*2*PI/8;
                const int32 P=G.Point(Head+FVector(2.2*FMath::Cos(A),2.2*FMath::Sin(A),.4),Bloom,float(Blade.phase));
                const int32 Q=G.Point(Head+FVector(4*FMath::Cos(B),4*FMath::Sin(B),1.3),Bloom,float(Blade.phase));
                const int32 R=G.Point(Head+FVector(2.2*FMath::Cos(C),2.2*FMath::Sin(C),.4),Bloom,float(Blade.phase));
                G.Tri(Centre,P,Q);G.Tri(Centre,Q,R);
            }
        }
    }
    const FString Name=FString::Printf(TEXT("Meadow_%d"),Variant),Path=TEXT("/Game/Garden/")+Name;
    auto* M=LoadObject<UStaticMesh>(nullptr,*(Path+TEXT(".")+Name));
    if(!M)M=NewObject<UStaticMesh>(CreatePackage(*Path),*Name,RF_Public|RF_Standalone);
    M->SetNumSourceModels(1);M->GetStaticMaterials().Reset();M->GetStaticMaterials().Add(FStaticMaterial(Material));
    FMeshDescription D;FStaticMeshAttributes A(D);A.Register();A.GetVertexInstanceUVs().SetNumChannels(1);
    const auto Group=D.CreatePolygonGroup();TArray<FVertexID> Points;
    for(auto P:G.Points){auto ID=D.CreateVertex();A.GetVertexPositions()[ID]=P;Points.Add(ID);}
    for(int32 I=0;I<G.Indices.Num();I+=3){
        const FVector3f P=G.Points[G.Indices[I]],Q=G.Points[G.Indices[I+1]],R=G.Points[G.Indices[I+2]];
        FVector3f Normal=FVector3f::CrossProduct(Q-P,R-P).GetSafeNormal();
        if(Normal.IsNearlyZero())continue;
        TArray<FVertexInstanceID> Corners;
        for(int32 J=0;J<3;J++){
            const int32 Index=G.Indices[I+J];const auto ID=D.CreateVertexInstance(Points[Index]);Corners.Add(ID);
            A.GetVertexInstanceNormals()[ID]=Normal;A.GetVertexInstanceColors()[ID]=G.Colors[Index];
            A.GetVertexInstanceUVs().Set(ID,0,FVector2f(G.Phases[Index],G.Points[Index].Z/80));
        }
        D.CreatePolygon(Group,Corners);
    }
    auto& S=M->GetSourceModel(0).BuildSettings;S.bRecomputeNormals=false;S.bRecomputeTangents=true;S.bGenerateLightmapUVs=false;
    M->NaniteSettings.bEnabled=false;M->CreateMeshDescription(0,MoveTemp(D));M->CommitMeshDescription(0);
    TArray<FText> Errors;M->Build(false,&Errors);FAssetCompilingManager::Get().FinishAllCompilation();
    if(!Errors.IsEmpty())return false;FAssetRegistryModule::AssetCreated(M);
    UE_LOG(LogTemp,Display,TEXT("MEADOW_MESH_READY variant=%d blades=%u vertices=%d triangles=%d opacity=opaque"),Variant,BladeCount,G.Points.Num(),G.Indices.Num()/3);
    return Save(M);
}
}
UMaterialInterface* PrepareMeadowAssets(){
    auto* GroundMaterial=Ground();auto* LeafMaterial=Leaf();if(!GroundMaterial||!LeafMaterial)return nullptr;
    for(int32 I=0;I<4;I++)if(!Clump(I,LeafMaterial))return nullptr;
    FAssetCompilingManager::Get().FinishAllCompilation();return GroundMaterial;
}
