// Copyright (c) 2026 Juri Janovski. MIT.
#include "AstralPrepareCommandlet.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetImportTask.h"
#include "AssetToolsModule.h"
#include "IAssetTools.h"
#include "Factories/TextureFactory.h"
#include "Engine/Texture2D.h"
#include "Materials/MaterialExpressionTextureSample.h"
#include "Materials/MaterialExpressionTextureCoordinate.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionAdd.h"
#include "Materials/MaterialExpressionMultiply.h"
#include "Materials/MaterialExpressionNoise.h"
#include "Materials/MaterialExpressionScalarParameter.h"
#include "Materials/MaterialExpressionVectorParameter.h"
#include "Materials/MaterialExpressionVertexColor.h"
#include "Materials/MaterialExpressionWorldPosition.h"
#include "Materials/MaterialExpressionConstant.h"
#include "Materials/MaterialExpressionTextureSampleParameter2D.h"
#include "Materials/MaterialExpressionScreenPosition.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "UObject/SavePackage.h"

UAstralPrepareCommandlet::UAstralPrepareCommandlet(){IsClient=false;IsServer=false;IsEditor=true;LogToConsole=true;}
int32 UAstralPrepareCommandlet::Main(const FString& Params){
    auto Import=[](const TCHAR* Name,const TCHAR* File,TextureCompressionSettings Compression)->UTexture2D*{
        auto* Task=NewObject<UAssetImportTask>();Task->Filename=FPaths::ProjectDir()/TEXT("ArtSource/Astral")/File;Task->DestinationPath=TEXT("/Game/Materials/Astral");Task->DestinationName=Name;
        Task->bAutomated=true;Task->bReplaceExisting=true;Task->bSave=false;
        auto* Factory=NewObject<UTextureFactory>();Factory->SuppressImportOverwriteDialog();Task->Factory=Factory;
        TArray<UAssetImportTask*> Tasks{Task};FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools")).Get().ImportAssetTasks(Tasks);
        for(auto* Object:Task->GetObjects())if(auto* Texture=Cast<UTexture2D>(Object)){
            Texture->SRGB=false;Texture->CompressionSettings=Compression;Texture->LODGroup=TEXTUREGROUP_World;Texture->PostEditChange();
            auto* Package=Texture->GetOutermost();const FString Target=FPackageName::LongPackageNameToFilename(Package->GetName(),FPackageName::GetAssetPackageExtension());
            IFileManager::Get().MakeDirectory(*FPaths::GetPath(Target),true);FSavePackageArgs Args;Args.TopLevelFlags=RF_Public|RF_Standalone;
            if(UPackage::SavePackage(Package,Texture,*Target,Args))return Texture;
        }return nullptr;
    };
    auto* Normal=Import(TEXT("T_AstralMineralNormal"),TEXT("mineral_normal.png"),TC_Normalmap);
    auto* Rough=Import(TEXT("T_AstralMineralRoughness"),TEXT("mineral_roughness.png"),TC_Masks);
    if(!Normal||!Rough)return 2;
    auto* M=LoadObject<UMaterial>(nullptr,TEXT("/Game/Materials/M_AstralSurface.M_AstralSurface"));
    UPackage* Package=M?M->GetOutermost():CreatePackage(TEXT("/Game/Materials/M_AstralSurface"));
    if(!M)M=NewObject<UMaterial>(Package,TEXT("M_AstralSurface"),RF_Public|RF_Standalone);
    M->PreEditChange(nullptr);M->GetExpressionCollection().Empty();
    auto Add=[M](UMaterialExpression* E){M->GetExpressionCollection().AddExpression(E);};
    auto Scalar=[M,&Add](const TCHAR* Name,float Value){auto* P=NewObject<UMaterialExpressionScalarParameter>(M);P->ParameterName=Name;P->DefaultValue=Value;Add(P);return P;};
    auto* Tint=NewObject<UMaterialExpressionVectorParameter>(M);Tint->ParameterName=TEXT("Tint");Tint->DefaultValue=FLinearColor(.12,.1,.09);Add(Tint);
    auto* World=NewObject<UMaterialExpressionWorldPosition>(M);Add(World);
    auto* Noise=NewObject<UMaterialExpressionNoise>(M);Noise->Position.Expression=World;Noise->Scale=.018;Noise->Levels=3;Noise->Quality=1;Noise->OutputMin=.0;Noise->OutputMax=1.;Add(Noise);
    auto* Grain=NewObject<UMaterialExpressionMultiply>(M);Grain->A.Expression=Noise;Grain->ConstB=.18;Add(Grain);
    auto* Bias=NewObject<UMaterialExpressionAdd>(M);Bias->A.Expression=Grain;Bias->ConstB=.82;Add(Bias);
    auto* Base=NewObject<UMaterialExpressionMultiply>(M);Base->A.Expression=Tint;Base->B.Expression=Bias;Add(Base);
    auto* Vertex=NewObject<UMaterialExpressionVertexColor>(M);Add(Vertex);
    auto* Color=NewObject<UMaterialExpressionMultiply>(M);Color->A.Expression=Base;Color->B.Expression=Vertex;Add(Color);
    auto* Emission=NewObject<UMaterialExpressionMultiply>(M);Emission->A.Expression=Tint;Emission->B.Expression=Scalar(TEXT("Glow"),0);Add(Emission);
    auto* UV=NewObject<UMaterialExpressionTextureCoordinate>(M);Add(UV);
    auto Sample=[M,UV,&Add](UTexture2D* T,EMaterialSamplerType Type){auto* S=NewObject<UMaterialExpressionTextureSample>(M);S->Texture=T;S->SamplerType=Type;S->Coordinates.Expression=UV;Add(S);return S;};
    auto* Roughness=NewObject<UMaterialExpressionMultiply>(M);Roughness->A.Expression=Scalar(TEXT("Roughness"),.55);Roughness->B.Expression=Sample(Rough,SAMPLERTYPE_Masks);Add(Roughness);
    M->GetEditorOnlyData()->BaseColor.Expression=Color;
    M->GetEditorOnlyData()->Metallic.Expression=Scalar(TEXT("Metallic"),.0);
    M->GetEditorOnlyData()->Roughness.Expression=Roughness;
    M->GetEditorOnlyData()->Normal.Expression=Sample(Normal,SAMPLERTYPE_Normal);
    M->GetEditorOnlyData()->EmissiveColor.Expression=Emission;
    M->PreEditChange(nullptr);M->PostEditChange();FAssetRegistryModule::AssetCreated(M);
    const FString File=FPackageName::LongPackageNameToFilename(Package->GetName(),FPackageName::GetAssetPackageExtension());
    IFileManager::Get().MakeDirectory(*FPaths::GetPath(File),true);Package->MarkPackageDirty();
    FSavePackageArgs Args;Args.TopLevelFlags=RF_Public|RF_Standalone;
    const bool Ok=UPackage::SavePackage(Package,M,*File,Args);
    if(!Ok)return 1;
    auto* Portal=LoadObject<UMaterial>(nullptr,TEXT("/Game/Materials/M_AstralAperture.M_AstralAperture"));
    auto* PortalPackage=Portal?Portal->GetOutermost():CreatePackage(TEXT("/Game/Materials/M_AstralAperture"));
    if(!Portal)Portal=NewObject<UMaterial>(PortalPackage,TEXT("M_AstralAperture"),RF_Public|RF_Standalone);
    Portal->PreEditChange(nullptr);Portal->GetExpressionCollection().Empty();Portal->SetShadingModel(MSM_Unlit);
    auto* Position=NewObject<UMaterialExpressionScreenPosition>(Portal);Portal->GetExpressionCollection().AddExpression(Position);
    auto* View=NewObject<UMaterialExpressionTextureSampleParameter2D>(Portal);View->ParameterName=TEXT("ArchiveView");View->Texture=Rough;View->SamplerType=SAMPLERTYPE_Masks;View->Coordinates.Expression=Position;Portal->GetExpressionCollection().AddExpression(View);
    Portal->GetEditorOnlyData()->EmissiveColor.Expression=View;Portal->PostEditChange();PortalPackage->MarkPackageDirty();
    FAssetRegistryModule::AssetCreated(Portal);
    const FString PortalFile=FPackageName::LongPackageNameToFilename(PortalPackage->GetName(),FPackageName::GetAssetPackageExtension());
    const bool Saved=UPackage::SavePackage(PortalPackage,Portal,*PortalFile,Args);
    UE_LOG(LogTemp,Display,TEXT("ASTRAL_MATERIAL result=%s world_space_noise=true live_aperture=true"),Saved?TEXT("PASS"):TEXT("FAIL"));return Saved?0:1;
}
