#include "HALVETHPrepareCommandlet.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetImportTask.h"
#include "AssetToolsModule.h"
#include "IAssetTools.h"
#include "Engine/Texture2D.h"
#include "Factories/TextureFactory.h"
#include "HAL/FileManager.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionConstant.h"
#include "Materials/MaterialExpressionMultiply.h"
#include "Materials/MaterialExpressionScalarParameter.h"
#include "Materials/MaterialExpressionVectorParameter.h"
#include "Materials/MaterialExpressionTextureSample.h"
#include "Materials/MaterialExpressionTextureCoordinate.h"
#include "Modules/ModuleManager.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"

namespace
{
    bool SaveAsset(UObject* Asset)
    {
        UPackage* Package = Asset->GetOutermost();
        const FString Filename = FPackageName::LongPackageNameToFilename(Package->GetName(), FPackageName::GetAssetPackageExtension());
        IFileManager::Get().MakeDirectory(*FPaths::GetPath(Filename), true);
        Package->MarkPackageDirty();
        FSavePackageArgs SaveArgs;
        SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
        SaveArgs.SaveFlags = SAVE_NoError;
        return UPackage::SavePackage(Package, Asset, *Filename, SaveArgs);
    }

    UTexture2D* ImportTexture(const FString& Filename, const FString& AssetName, TextureCompressionSettings Compression, bool SRGB)
    {
        const FString ObjectPath = TEXT("/Game/Materials/PolyHaven/") + AssetName + TEXT(".") + AssetName;
        if (UTexture2D* Existing = LoadObject<UTexture2D>(nullptr, *ObjectPath)) return Existing;
        if (!FPaths::FileExists(Filename))
        {
            UE_LOG(LogTemp, Error, TEXT("PBR source image is missing: %s"), *Filename);
            return nullptr;
        }
        UAssetImportTask* Task = NewObject<UAssetImportTask>();
        Task->Filename = Filename;
        Task->DestinationPath = TEXT("/Game/Materials/PolyHaven");
        Task->DestinationName = AssetName;
        Task->bAutomated = true;
        Task->bAsync = false;
        Task->bReplaceExisting = false;
        Task->bSave = false;
        UTextureFactory* Factory = NewObject<UTextureFactory>();
        Factory->SuppressImportOverwriteDialog();
        Task->Factory = Factory;
        auto& Tools = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools")).Get();
        TArray<UAssetImportTask*> Tasks{Task};
        Tools.ImportAssetTasks(Tasks);
        for (UObject* Object : Task->GetObjects())
        {
            if (UTexture2D* Texture = Cast<UTexture2D>(Object))
            {
                Texture->SRGB = SRGB;
                Texture->CompressionSettings = Compression;
                Texture->LODGroup = TEXTUREGROUP_World;
                Texture->PostEditChange();
                if (!SaveAsset(Texture)) return nullptr;
                return Texture;
            }
        }
        return nullptr;
    }

    bool PreparePBR(const FString& SourceId, const FString& MaterialName, const FString& Prefix)
    {
        const FString ObjectPath = TEXT("/Game/Materials/") + MaterialName + TEXT(".") + MaterialName;
        if (LoadObject<UMaterial>(nullptr, *ObjectPath)) return true;
        const FString Directory = FPaths::ProjectDir() / TEXT("ArtSource/PolyHaven") / SourceId;
        UTexture2D* Color = ImportTexture(Directory / (SourceId + TEXT("_diff_2k.png")), Prefix + TEXT("_Color"), TC_Default, true);
        UTexture2D* Normal = ImportTexture(Directory / (SourceId + TEXT("_nor_dx_2k.png")), Prefix + TEXT("_Normal"), TC_Normalmap, false);
        UTexture2D* Rough = ImportTexture(Directory / (SourceId + TEXT("_rough_2k.png")), Prefix + TEXT("_Roughness"), TC_Masks, false);
        if (!Color || !Normal || !Rough) return false;
        UPackage* Package = CreatePackage(*(TEXT("/Game/Materials/") + MaterialName));
        UMaterial* Material = NewObject<UMaterial>(Package, *MaterialName, RF_Public | RF_Standalone);
        auto* UV = NewObject<UMaterialExpressionTextureCoordinate>(Material);
        UV->UTiling = 12; UV->VTiling = 12;
        Material->GetExpressionCollection().AddExpression(UV);
        auto Sample = [Material, UV](UTexture2D* Texture, EMaterialSamplerType Type)
        {
            auto* Expression = NewObject<UMaterialExpressionTextureSample>(Material);
            Expression->Texture = Texture;
            Expression->SamplerType = Type;
            Expression->Coordinates.Expression = UV;
            Material->GetExpressionCollection().AddExpression(Expression);
            return Expression;
        };
        Material->GetEditorOnlyData()->BaseColor.Expression = Sample(Color, SAMPLERTYPE_Color);
        Material->GetEditorOnlyData()->Normal.Expression = Sample(Normal, SAMPLERTYPE_Normal);
        Material->GetEditorOnlyData()->Roughness.Expression = Sample(Rough, SAMPLERTYPE_Masks);
        Material->PreEditChange(nullptr);
        Material->PostEditChange();
        FAssetRegistryModule::AssetCreated(Material);
        const bool Saved = SaveAsset(Material);
        if (Saved) UE_LOG(LogTemp, Display, TEXT("HALVETH_PBR_READY %s (2K, DX normal, UV12)"), *MaterialName);
        return Saved;
    }
}

UHALVETHPrepareCommandlet::UHALVETHPrepareCommandlet()
{
    IsClient = false; IsServer = false; IsEditor = true; LogToConsole = true;
}

int32 UHALVETHPrepareCommandlet::Main(const FString& Params)
{
    const FString PackageName = TEXT("/Game/Materials/M_HalvethSurface");
    const FString Filename = FPackageName::LongPackageNameToFilename(PackageName, FPackageName::GetAssetPackageExtension());
    if (FPaths::FileExists(Filename))
    {
        if (LoadObject<UMaterial>(nullptr, TEXT("/Game/Materials/M_HalvethSurface.M_HalvethSurface")))
        {
            UE_LOG(LogTemp, Display, TEXT("HALVETH_PREPARE_READY: existing material preserved."));
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("Existing material cannot be read. It was left untouched: %s"), *Filename);
            return 1;
        }
    }
    else
    {
    IFileManager::Get().MakeDirectory(*FPaths::GetPath(Filename), true);
    UPackage* Package = CreatePackage(*PackageName);
    UMaterial* Material = NewObject<UMaterial>(Package, TEXT("M_HalvethSurface"), RF_Public | RF_Standalone);
    Material->TwoSided = false;
    auto* Tint = NewObject<UMaterialExpressionVectorParameter>(Material);
    Tint->ParameterName = TEXT("Tint");
    Tint->DefaultValue = FLinearColor(0.5f, 0.15f, 0.3f);
    auto* Glow = NewObject<UMaterialExpressionScalarParameter>(Material);
    Glow->ParameterName = TEXT("Glow"); Glow->DefaultValue = 0;
    auto* Emission = NewObject<UMaterialExpressionMultiply>(Material);
    Emission->A.Expression = Tint; Emission->B.Expression = Glow;
    auto* Roughness = NewObject<UMaterialExpressionConstant>(Material);
    Roughness->R = 0.68f;
    Material->GetExpressionCollection().AddExpression(Tint);
    Material->GetExpressionCollection().AddExpression(Glow);
    Material->GetExpressionCollection().AddExpression(Emission);
    Material->GetExpressionCollection().AddExpression(Roughness);
    Material->GetEditorOnlyData()->BaseColor.Expression = Tint;
    Material->GetEditorOnlyData()->EmissiveColor.Expression = Emission;
    Material->GetEditorOnlyData()->Roughness.Expression = Roughness;
    Material->PreEditChange(nullptr);
    Material->PostEditChange();
    FAssetRegistryModule::AssetCreated(Material);
    Package->MarkPackageDirty();
    FSavePackageArgs SaveArgs;
    SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
    SaveArgs.SaveFlags = SAVE_NoError;
    if (!UPackage::SavePackage(Package, Material, *Filename, SaveArgs)) return 1;
    UE_LOG(LogTemp, Display, TEXT("HALVETH_PREPARE_READY: created %s"), *Filename);
    }
    const bool Cobblestone = PreparePBR(TEXT("cobblestone_floor_03"), TEXT("M_PH_CobblestoneFloor03"), TEXT("T_PH_CobblestoneFloor03"));
    const bool Forest = PreparePBR(TEXT("forest_ground_04"), TEXT("M_PH_ForestGround04"), TEXT("T_PH_ForestGround04"));
    return Cobblestone && Forest ? 0 : 1;
}
