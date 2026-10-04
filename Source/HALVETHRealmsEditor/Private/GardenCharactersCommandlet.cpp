#include "GardenCharactersCommandlet.h"
#include "AssetImportTask.h"
#include "AssetToolsModule.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetCompilingManager.h"
#include "IAssetTools.h"
#include "Factories/FbxFactory.h"
#include "Factories/FbxImportUI.h"
#include "Factories/FbxSkeletalMeshImportData.h"
#include "Factories/FbxAnimSequenceImportData.h"
#include "Factories/TextureFactory.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/Texture2D.h"
#include "Animation/AnimSequence.h"
#include "Animation/MorphTarget.h"
#include "Animation/Skeleton.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionConstant.h"
#include "Materials/MaterialExpressionConstant3Vector.h"
#include "Materials/MaterialExpressionTextureSample.h"
#include "Materials/MaterialExpressionMultiply.h"
#include "Misc/Paths.h"
#include "Misc/PackageName.h"
#include "UObject/SavePackage.h"
#include "HAL/IConsoleManager.h"

namespace {
bool Save(UObject* O) {
    UPackage* P=O->GetOutermost(); P->MarkPackageDirty();
    FSavePackageArgs A; A.TopLevelFlags=RF_Public|RF_Standalone; A.SaveFlags=SAVE_NoError;
    return UPackage::SavePackage(P,O,*FPackageName::LongPackageNameToFilename(P->GetName(),FPackageName::GetAssetPackageExtension()),A);
}
template<class T> T* E(UMaterial* M) { auto* R=NewObject<T>(M);M->GetExpressionCollection().AddExpression(R);return R; }
UTexture2D* Texture(const FString& Name,bool Normal=false,const FString& AssetName=FString()) {
    const FString Destination=AssetName.IsEmpty()?Name:AssetName;
    auto* T=NewObject<UAssetImportTask>();T->Filename=FPaths::ProjectDir()/TEXT("ArtSource/Characters")/(Name+TEXT(".png"));
    T->DestinationPath=TEXT("/Game/Characters");T->DestinationName=Destination;T->bAutomated=true;T->bSave=true;T->bReplaceExisting=true;
    auto* F=NewObject<UTextureFactory>();F->SuppressImportOverwriteDialog();T->Factory=F;
    FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools")).Get().ImportAssetTasks({T});
    for(auto* O:T->GetObjects()) if(auto* R=Cast<UTexture2D>(O)) {R->LODGroup=TEXTUREGROUP_Character;if(Normal){R->CompressionSettings=TC_Normalmap;R->SRGB=false;}R->PostEditChange();Save(R);return R;}
    return nullptr;
}
UMaterial* Material(const FString& Name,FLinearColor Color,float Metallic,float Rough,UTexture2D* Tex=nullptr,bool Tint=false,UTexture2D* Normal=nullptr,bool Hair=false,bool Skin=false) {
    FString Path=TEXT("/Game/Characters/")+Name;
    auto* M=LoadObject<UMaterial>(nullptr,*(Path+TEXT(".")+Name));
    const bool Created=M==nullptr;
    if(!M)M=NewObject<UMaterial>(CreatePackage(*Path),*Name,RF_Public|RF_Standalone);
    M->GetExpressionCollection().Empty();
    M->BlendMode=Hair?BLEND_Masked:BLEND_Opaque;
    M->OpacityMaskClipValue=.35f;
    M->SetShadingModel(MSM_DefaultLit);
    M->TwoSided=true; M->bUsedWithSkeletalMesh=true; M->bUsedWithMorphTargets=true;
    if(Tex) { auto* T=E<UMaterialExpressionTextureSample>(M);T->Texture=Tex;T->SamplerType=SAMPLERTYPE_Color;
        if(Tint){auto* C=E<UMaterialExpressionConstant3Vector>(M);C->Constant=Color;auto* Mix=E<UMaterialExpressionMultiply>(M);Mix->A.Expression=T;Mix->B.Expression=C;M->GetEditorOnlyData()->BaseColor.Expression=Mix;}
        else M->GetEditorOnlyData()->BaseColor.Expression=T;
    }
    else {auto* C=E<UMaterialExpressionConstant3Vector>(M);C->Constant=Color;M->GetEditorOnlyData()->BaseColor.Expression=C;}
    auto* R=E<UMaterialExpressionConstant>(M);R->R=Rough;M->GetEditorOnlyData()->Roughness.Expression=R;
    auto* B=E<UMaterialExpressionConstant>(M);B->R=Metallic;M->GetEditorOnlyData()->Metallic.Expression=B;
    if(Normal){auto* T=E<UMaterialExpressionTextureSample>(M);T->Texture=Normal;T->SamplerType=SAMPLERTYPE_Normal;M->GetEditorOnlyData()->Normal.Expression=T;}
    if(Hair&&Tex){auto* T=E<UMaterialExpressionTextureSample>(M);T->Texture=Tex;T->SamplerType=SAMPLERTYPE_Color;M->GetEditorOnlyData()->OpacityMask.Expression=T;M->GetEditorOnlyData()->OpacityMask.OutputIndex=4;}
    if(Skin){auto* C=E<UMaterialExpressionConstant3Vector>(M);C->Constant=FLinearColor(.6f,.22f,.12f);M->GetEditorOnlyData()->SubsurfaceColor.Expression=C;auto* A=E<UMaterialExpressionConstant>(M);A->R=.24f;M->GetEditorOnlyData()->Opacity.Expression=A;}
    M->PreEditChange(nullptr);M->PostEditChange();if(Created)FAssetRegistryModule::AssetCreated(M);Save(M);return M;
}
}
UGardenCharactersCommandlet::UGardenCharactersCommandlet() {IsClient=false;IsServer=false;IsEditor=true;LogToConsole=true;}
int32 UGardenCharactersCommandlet::Main(const FString& Params) {
    // The import contract below is the legacy FBX skeletal importer, including
    // its explicit morph/animation settings. Do not silently select Interchange.
    if(auto* Flag=IConsoleManager::Get().FindConsoleVariable(TEXT("Interchange.FeatureFlags.Import.FBX")))
        Flag->Set(0,ECVF_SetByCode);
    auto* Female=Texture(TEXT("skin_female"));auto* Male=Texture(TEXT("skin_male"));if(!Female||!Male)return 2;
    auto* Fabric=Texture(TEXT("cloth_weave"));auto* FabricNormal=Texture(TEXT("cloth_normal"),true);if(!Fabric||!FabricNormal)return 2;
    const FString Names[]={TEXT("Scarlet"),TEXT("Lucinet"),TEXT("Rachel")};
    const FLinearColor ClothColors[]={{.19,.017,.039},{.026,.082,.105},{.095,.038,.021}};
    const FLinearColor HairColors[]={{.055,.011,.014},{.044,.025,.016},{.22,.078,.025}};
    for(int32 I=0;I<3;I++) {
        const FString N=Names[I];
        auto* Wardrobe=Texture(N+TEXT("_wardrobe"));auto* WardrobeNormal=Texture(N+TEXT("_wardrobe_normal"),true);auto* HairTexture=Texture(N+TEXT("_hair"),false,TEXT("T_")+N+TEXT("Hair"));auto* FootTexture=Texture(TEXT("footwear_diffuse"));
        if(!Wardrobe||!WardrobeNormal||!HairTexture||!FootTexture)return 2;
        // Texture imports can load Interchange after the commandlet starts.
        // Bind the skeletal importer after that module registration, explicitly.
        auto* FbxRoute=IConsoleManager::Get().FindConsoleVariable(TEXT("Interchange.FeatureFlags.Import.FBX"));
        if(!FbxRoute){UE_LOG(LogTemp,Error,TEXT("GARDEN_CHARACTER_IMPORT_ROUTE_UNAVAILABLE"));return 6;}
        FbxRoute->Set(0,ECVF_SetByCode);
        UE_LOG(LogTemp,Display,TEXT("GARDEN_CHARACTER_IMPORT_ROUTE legacy_fbx=%d"),FbxRoute->GetInt()==0);
        auto* Task=NewObject<UAssetImportTask>();
        Task->Filename=FPaths::ProjectDir()/TEXT("ArtSource/Characters")/(N+TEXT(".fbx"));
        Task->DestinationPath=TEXT("/Game/Characters");Task->DestinationName=N;
        Task->bAutomated=true;Task->bReplaceExisting=true;Task->bSave=false;
        auto* Factory=NewObject<UFbxFactory>();Factory->SetDetectImportTypeOnImport(false);
        UFbxImportUI* UI=Factory->ImportUI.Get();UI->MeshTypeToImport=FBXIT_SkeletalMesh;UI->OriginalImportType=FBXIT_SkeletalMesh;
        UI->bImportAsSkeletal=true;UI->bImportMesh=true;UI->bImportAnimations=true;UI->bCreatePhysicsAsset=false;
        UI->bImportMaterials=false;UI->bImportTextures=false;UI->bOverrideFullName=true;
        UI->OverrideAnimationName=N+TEXT("Idle");UI->SkeletalMeshImportData->bImportMorphTargets=true;
        UI->SkeletalMeshImportData->bImportMeshesInBoneHierarchy=true;
        UI->AnimSequenceImportData->bUseDefaultSampleRate=true;UI->AnimSequenceImportData->bImportBoneTracks=true;
        Task->Factory=Factory;Task->Options=UI;
        FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools")).Get().ImportAssetTasks({Task});
        USkeletalMesh* Mesh=nullptr;UAnimSequence* Idle=nullptr;
        for(UObject* O:Task->GetObjects()) {
            if(auto* S=Cast<USkeletalMesh>(O)) Mesh=S;
            if(auto* A=Cast<UAnimSequence>(O)) Idle=A;
            Save(O);UE_LOG(LogTemp,Display,TEXT("GARDEN_CHARACTER_IMPORTED %s"),*O->GetPathName());
        }
        if(!Mesh) Mesh=LoadObject<USkeletalMesh>(nullptr,*(TEXT("/Game/Characters/")+N+TEXT(".")+N));
        if(!Idle) Idle=LoadObject<UAnimSequence>(nullptr,*(TEXT("/Game/Characters/")+N+TEXT("Idle.")+N+TEXT("Idle")));
        if(!Mesh||!Idle) {UE_LOG(LogTemp,Error,TEXT("GARDEN_CHARACTER_REQUIRED_MESH_ANIMATION_MISSING %s"),*N);return 3;}
        TMap<FString,UMaterial*> Materials;
        for(const FString& S:TArray<FString>{TEXT("Skin"),TEXT("Cloth"),TEXT("Leather"),TEXT("Footwear"),TEXT("Metal"),TEXT("Hair"),TEXT("Sclera"),TEXT("Iris"),TEXT("Pupil")}) {
            FLinearColor C=FLinearColor(.1,.1,.1);float Metal=0,Rough=.6;UTexture2D* Tex=nullptr;
            if(S==TEXT("Skin"))Tex=I==1?Male:Female;
            else if(S==TEXT("Cloth")){C=ClothColors[I]*2.2f;Rough=.82;Tex=Wardrobe;}
            else if(S==TEXT("Leather")){C={.13,.08,.045};Rough=.64;Tex=Wardrobe;}
            else if(S==TEXT("Footwear")){Rough=.68;Tex=FootTexture;}
            else if(S==TEXT("Metal")){C={.49,.29,.091};Metal=.83;Rough=.3;}
            else if(S==TEXT("Hair")){C=HairColors[I]*10.f;Rough=.53;Tex=HairTexture;}
            else if(S==TEXT("Sclera")){C={.8,.77,.72};Rough=.25;}
            else if(S==TEXT("Iris")){C=I==1?FLinearColor(.057,.095,.16):FLinearColor(.047,.14,.105);Rough=.25;}
            else if(S==TEXT("Pupil")){C={.005,.006,.008};Rough=.18;}
            else {UE_LOG(LogTemp,Error,TEXT("GARDEN_CHARACTER_UNKNOWN_MATERIAL_SLOT %s %s"),*N,*S);return 5;}
            Materials.Add(S,Material(N+TEXT("_")+S,C,Metal,Rough,Tex,S==TEXT("Cloth")||S==TEXT("Leather")||S==TEXT("Hair"),S==TEXT("Cloth")||S==TEXT("Leather")?WardrobeNormal:nullptr,S==TEXT("Hair"),S==TEXT("Skin")));
        }
        for(FSkeletalMaterial& Slot:Mesh->GetMaterials()) {
            FString S=Slot.MaterialSlotName.ToString();int32 Dot;if(S.FindChar('.',Dot))S=S.Left(Dot);
            int32 Underscore;if(S.FindLastChar('_',Underscore)&&S.Mid(Underscore+1).IsNumeric())S=S.Left(Underscore);
            if(!Materials.Contains(S)){UE_LOG(LogTemp,Error,TEXT("GARDEN_CHARACTER_UNKNOWN_MATERIAL_SLOT %s %s"),*N,*S);return 5;}
            Slot.MaterialInterface=Materials[S];
        }
        Mesh->PostEditChange();
        if(!Mesh->GetSkeleton()||!Save(Mesh->GetSkeleton())||!Save(Mesh)||!Save(Idle)) return 4;
        UE_LOG(LogTemp,Display,TEXT("GARDEN_CHARACTER_READY %s bones=%d morphs=%d duration=%f"),*N,Mesh->GetRefSkeleton().GetNum(),Mesh->GetMorphTargets().Num(),Idle->GetPlayLength());
        for(const auto& Morph:Mesh->GetMorphTargets())UE_LOG(LogTemp,Display,TEXT("GARDEN_CHARACTER_MORPH %s"),*Morph->GetName());
    }
    FAssetCompilingManager::Get().FinishAllCompilation();return 0;
}
