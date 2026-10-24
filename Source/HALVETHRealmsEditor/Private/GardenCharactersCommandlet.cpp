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
#include "TissueField.h"
#include "Materials/MaterialExpressionCustom.h"
#include "Materials/MaterialExpressionTextureObjectParameter.h"
#include "Materials/MaterialExpressionScalarParameter.h"
#include "Materials/MaterialExpressionWorldPosition.h"
#include "Materials/MaterialExpressionVectorParameter.h"
#include "Materials/MaterialExpressionAdd.h"
#include "Materials/MaterialExpressionTextureCoordinate.h"
#include "Materials/MaterialExpressionComponentMask.h"
#include "Materials/MaterialExpressionAppendVector.h"
#include "Materials/MaterialExpressionNormalize.h"
#include "Engine/SubsurfaceProfile.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Misc/FileHelper.h"

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
UMaterial* Material(const FString& Name,FLinearColor Color,float Metallic,float Rough,UTexture2D* Tex=nullptr,bool Tint=false,UTexture2D* Normal=nullptr,bool Hair=false,bool Skin=false,FLinearColor SkinAlbedo=FLinearColor::Black) {
    FString Path=TEXT("/Game/Characters/")+Name;
    auto* M=LoadObject<UMaterial>(nullptr,*(Path+TEXT(".")+Name));
    const bool Created=M==nullptr;
    if(!M)M=NewObject<UMaterial>(CreatePackage(*Path),*Name,RF_Public|RF_Standalone);
    M->GetExpressionCollection().Empty();
    M->BlendMode=Hair?BLEND_Masked:BLEND_Opaque;
    M->OpacityMaskClipValue=.35f;
    M->SetShadingModel(Skin?MSM_SubsurfaceProfile:MSM_DefaultLit);
    M->TwoSided=Hair; M->bUsedWithSkeletalMesh=true; M->bUsedWithMorphTargets=true;
    if(Tex) { auto* T=E<UMaterialExpressionTextureSample>(M);T->Texture=Tex;T->SamplerType=SAMPLERTYPE_Color;
        if(Tint){auto* C=E<UMaterialExpressionConstant3Vector>(M);C->Constant=Color;auto* Mix=E<UMaterialExpressionMultiply>(M);Mix->A.Expression=T;Mix->B.Expression=C;M->GetEditorOnlyData()->BaseColor.Expression=Mix;}
        else M->GetEditorOnlyData()->BaseColor.Expression=T;
    }
    else {auto* C=E<UMaterialExpressionConstant3Vector>(M);C->Constant=Color;M->GetEditorOnlyData()->BaseColor.Expression=C;}
    auto* R=E<UMaterialExpressionConstant>(M);R->R=Rough;M->GetEditorOnlyData()->Roughness.Expression=R;
    auto* B=E<UMaterialExpressionConstant>(M);B->R=Metallic;M->GetEditorOnlyData()->Metallic.Expression=B;
    if(Normal){auto* T=E<UMaterialExpressionTextureSample>(M);T->Texture=Normal;T->SamplerType=SAMPLERTYPE_Normal;M->GetEditorOnlyData()->Normal.Expression=T;}
    if(Hair&&Tex){auto* T=E<UMaterialExpressionTextureSample>(M);T->Texture=Tex;T->SamplerType=SAMPLERTYPE_Color;M->GetEditorOnlyData()->OpacityMask.Expression=T;M->GetEditorOnlyData()->OpacityMask.OutputIndex=4;}
    if(Skin){
        const FLinearColor Balance(.72f,.87f,.98f);
        // Actual skin shading, rather than the old unused SubsurfaceColor on a
        // DefaultLit material. Distances here are an art calibration in cm.
        const FString ProfileName=Name+TEXT("Profile"),ProfilePath=TEXT("/Game/Characters/")+ProfileName;
        auto* Profile=LoadObject<USubsurfaceProfile>(nullptr,*(ProfilePath+TEXT(".")+ProfileName));
        const bool NewProfile=Profile==nullptr;
        if(!Profile)Profile=NewObject<USubsurfaceProfile>(CreatePackage(*ProfilePath),*ProfileName,RF_Public|RF_Standalone);
        Profile->Settings.bEnableBurley=true;Profile->Settings.bEnableMeanFreePath=true;
        Profile->Settings.SurfaceAlbedo=SkinAlbedo*Balance;
        Profile->Settings.MeanFreePathColor=FLinearColor(1.f,.42f,.22f);
        Profile->Settings.MeanFreePathDistance=.15f;Profile->Settings.WorldUnitScale=1.f;
        Profile->Settings.Tint=FLinearColor(.3f,.3f,.3f);
        Profile->Settings.Roughness0=.85f;Profile->Settings.Roughness1=1.3f;Profile->Settings.LobeMix=.75f;
        if(NewProfile)FAssetRegistryModule::AssetCreated(Profile);Save(Profile);M->SubsurfaceProfile=Profile;
        auto* Tone=E<UMaterialExpressionVectorParameter>(M);Tone->ParameterName=TEXT("SkinTone");Tone->DefaultValue=Color*Balance;
        auto* Mix=E<UMaterialExpressionMultiply>(M);Mix->A.Expression=M->GetEditorOnlyData()->BaseColor.Expression;Mix->B.Expression=Tone;
        M->GetEditorOnlyData()->BaseColor.Expression=Mix;
        auto* Amount=E<UMaterialExpressionScalarParameter>(M);Amount->ParameterName=TEXT("SkinScatter");Amount->DefaultValue=.08f;M->GetEditorOnlyData()->Opacity.Expression=Amount;
        auto* Spec=E<UMaterialExpressionScalarParameter>(M);Spec->ParameterName=TEXT("SkinSpecular");Spec->DefaultValue=.35f;M->GetEditorOnlyData()->Specular.Expression=Spec;
        auto* UV=E<UMaterialExpressionTextureCoordinate>(M);
        // Authored, antialiased microdetail in tangent space. The frequency is
        // faded with pixel footprint: distant people do not sparkle or crawl.
        auto* Detail=E<UMaterialExpressionCustom>(M);Detail->OutputType=CMOT_Float4;
        Detail->Code=TEXT(R"HLSL(
float2 p=UV*480.0,cell=floor(p),f=frac(p);float nearest=10.;float2 delta=0.;
[unroll]for(int y=-1;y<=1;y++)[unroll]for(int x=-1;x<=1;x++){
 float2 o=float2(x,y),id=cell+o;
 float2 h=frac(sin(float2(dot(id,float2(127.1,311.7)),dot(id,float2(269.5,183.3))))*43758.5453);
 float2 d=f-o-(.18+.64*h);float r=dot(d,d);if(r<nearest){nearest=r;delta=d;}
}
float footprint=max(length(ddx(p)),length(ddy(p)));
float visible=1-smoothstep(.35,1.4,footprint);
float2 n=-delta*.18*exp(-nearest*24.)*visible;
float oil=.5+.5*sin(UV.x*37.+sin(UV.y*29.));
return float4(n,.43+.12*oil-.035*exp(-nearest*18.)*visible,.2);
)HLSL");
        FCustomInput In;In.InputName=TEXT("UV");In.Input.Expression=UV;Detail->Inputs.Add(In);
        auto* XY=E<UMaterialExpressionComponentMask>(M);XY->Input.Expression=Detail;XY->R=true;XY->G=true;XY->B=false;XY->A=false;
        auto* N=E<UMaterialExpressionAppendVector>(M);N->A.Expression=XY;auto* One=E<UMaterialExpressionConstant>(M);One->R=1;N->B.Expression=One;
        auto* Unit=E<UMaterialExpressionNormalize>(M);Unit->VectorInput.Expression=N;M->GetEditorOnlyData()->Normal.Expression=Unit;
        auto* Roughness=E<UMaterialExpressionComponentMask>(M);Roughness->Input.Expression=Detail;Roughness->R=false;Roughness->G=false;Roughness->B=true;Roughness->A=false;
        auto* Bias=E<UMaterialExpressionScalarParameter>(M);Bias->ParameterName=TEXT("SkinRoughnessBias");Bias->DefaultValue=0;
        auto* Biased=E<UMaterialExpressionAdd>(M);Biased->A.Expression=Roughness;Biased->B.Expression=Bias;
        M->GetEditorOnlyData()->Roughness.Expression=Biased;
    }
    if(Name.EndsWith(TEXT("_Iris"))){
        M->SetShadingModel(MSM_ClearCoat);
        auto* Coat=E<UMaterialExpressionConstant>(M);Coat->R=.7f;M->GetEditorOnlyData()->ClearCoat.Expression=Coat;
        auto* Wet=E<UMaterialExpressionConstant>(M);Wet->R=.08f;M->GetEditorOnlyData()->ClearCoatRoughness.Expression=Wet;
        auto* UV=E<UMaterialExpressionTextureCoordinate>(M);
        auto* Pattern=E<UMaterialExpressionCustom>(M);Pattern->OutputType=CMOT_Float3;
        Pattern->Code=TEXT(R"HLSL(
float2 p=(UV-.5)*2.;float r=length(p),a=atan2(p.y,p.x);
float coarse=1-smoothstep(.8,2.5,fwidth(a)*7.);
float fine=1-smoothstep(.6,2.,fwidth(a)*23.);
float fibers=.48+.32*sin(a*7.+sin(a*3.)+r*9.)*coarse+.16*sin(a*23.-r*17.)*fine;
float limbus=1.-.65*smoothstep(.74,1.,r);
float inner=lerp(.32,1.,smoothstep(.18,.5,r));
return Tone*(.62+1.1*fibers)*limbus*inner;
)HLSL");
        FCustomInput In;In.InputName=TEXT("UV");In.Input.Expression=UV;Pattern->Inputs.Add(In);
        auto* Tone=E<UMaterialExpressionConstant3Vector>(M);Tone->Constant=Color;
        FCustomInput ColorInput;ColorInput.InputName=TEXT("Tone");ColorInput.Input.Expression=Tone;Pattern->Inputs.Add(ColorInput);
        M->GetEditorOnlyData()->BaseColor.Expression=Pattern;
        auto* Spec=E<UMaterialExpressionConstant>(M);Spec->R=.4f;M->GetEditorOnlyData()->Specular.Expression=Spec;
    }
    const bool Probe=Name==TEXT("M_TissueFieldProbe");
    const bool Facial=Name.EndsWith(TEXT("_Hair"))||Name.EndsWith(TEXT("_Sclera"))||Name.EndsWith(TEXT("_Iris"))||Name.EndsWith(TEXT("_Pupil"));
    if(Skin||Facial||Name.EndsWith(TEXT("_Cloth"))||Name.EndsWith(TEXT("_Leather"))||Probe){
        auto* Field=E<UMaterialExpressionTextureObjectParameter>(M);Field->ParameterName=TEXT("TissueField");Field->Texture=LoadObject<UTexture2D>(nullptr,TEXT("/Engine/EngineResources/DefaultTexture.DefaultTexture"));Field->SamplerType=SAMPLERTYPE_Color;
        auto* Enabled=E<UMaterialExpressionScalarParameter>(M);Enabled->ParameterName=TEXT("TissueEnabled");Enabled->DefaultValue=0;
        // Adjacent eyelid, globe, iris, pupil and brow surfaces share one field.
        // Moving skin alone makes otherwise rigid eyes/brows drift through it.
        auto* Softness=E<UMaterialExpressionScalarParameter>(M);Softness->ParameterName=TEXT("TissueSoftness");Softness->DefaultValue=(Skin||Facial)?1.f:.65f;
        UMaterialExpression* Position=nullptr;
        if(Probe){auto* P=E<UMaterialExpressionVectorParameter>(M);P->ParameterName=TEXT("TissueProbePosition");Position=P;}
        else {auto* P=E<UMaterialExpressionWorldPosition>(M);P->WorldPositionShaderOffset=WPT_ExcludeAllShaderOffsets;Position=P;}
        auto* Deform=E<UMaterialExpressionCustom>(M);Deform->Code=UTF8_TO_TCHAR(HalvethTissueField::Shader);Deform->OutputType=CMOT_Float3;
        for(auto Pair:{TPair<FName,UMaterialExpression*>(TEXT("Field"),Field),TPair<FName,UMaterialExpression*>(TEXT("Enabled"),Enabled),TPair<FName,UMaterialExpression*>(TEXT("Softness"),Softness),TPair<FName,UMaterialExpression*>(TEXT("Position"),Position)}){FCustomInput Input;Input.InputName=Pair.Key;Input.Input.Expression=Pair.Value;Deform->Inputs.Add(Input);}
        if(Probe){
            M->SetShadingModel(MSM_Unlit);
            auto* Scale=E<UMaterialExpressionMultiply>(M);Scale->A.Expression=Deform;Scale->ConstB=.05f;
            auto* Encode=E<UMaterialExpressionAdd>(M);Encode->A.Expression=Scale;Encode->ConstB=.5f;M->GetEditorOnlyData()->EmissiveColor.Expression=Encode;
        }else M->GetEditorOnlyData()->WorldPositionOffset.Expression=Deform;
    }
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
    FString ManifestText;TSharedPtr<FJsonObject> Manifest;
    if(!FFileHelper::LoadFileToString(ManifestText,*(FPaths::ProjectDir()/TEXT("ArtSource/Characters/characters.json")))
       ||!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(ManifestText),Manifest)||!Manifest.IsValid())return 8;
    const auto& Identities=Manifest->GetArrayField(TEXT("characters"));if(Identities.Num()!=8)return 8;
    for(int32 I=0;I<Identities.Num();I++) {
        const auto Definition=Identities[I]->AsObject();if(!Definition.IsValid())return 8;
        const FString N=Definition->GetStringField(TEXT("name"));
        const bool MaleSkin=Definition->GetStringField(TEXT("skinSource"))==TEXT("skin_male");
        const auto& Tint=Definition->GetArrayField(TEXT("skinTintLinear"));if(Tint.Num()!=3)return 8;
        const FLinearColor SkinTone(Tint[0]->AsNumber(),Tint[1]->AsNumber(),Tint[2]->AsNumber());
        auto ReadColor=[&](const TCHAR* Key,FLinearColor& Out){
            const TArray<TSharedPtr<FJsonValue>>* Values=nullptr;
            if(!Definition->TryGetArrayField(Key,Values)||!Values||Values->Num()!=3)return false;
            double V[3];for(int32 Channel=0;Channel<3;Channel++){
                if(!(*Values)[Channel]->TryGetNumber(V[Channel])||!FMath::IsFinite(V[Channel])||V[Channel]<0||V[Channel]>1)return false;
            }
            Out=FLinearColor(V[0],V[1],V[2]);return true;
        };
        FLinearColor SkinAlbedo,BrowColor,IrisColor;
        if(!ReadColor(TEXT("skinAlbedoLinear"),SkinAlbedo)||!ReadColor(TEXT("browColorLinear"),BrowColor)||!ReadColor(TEXT("irisColorLinear"),IrisColor))return 8;
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
        UI->SkeletalMeshImportData->NormalImportMethod=FBXNIM_ImportNormals;
        UI->SkeletalMeshImportData->NormalGenerationMethod=EFBXNormalGenerationMethod::MikkTSpace;
        UI->SkeletalMeshImportData->bPreserveSmoothingGroups=true;
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
        for(const FString& S:TArray<FString>{TEXT("Skin"),TEXT("Hair"),TEXT("Sclera"),TEXT("Iris"),TEXT("Pupil")}) {
            FLinearColor C=FLinearColor(.1,.1,.1);float Metal=0,Rough=.6;UTexture2D* Tex=nullptr;
            if(S==TEXT("Skin")){Tex=MaleSkin?Male:Female;C=SkinTone;}
            else if(S==TEXT("Hair")){C=BrowColor;Rough=.72;}
            else if(S==TEXT("Sclera")){C={.52,.49,.45};Rough=.22;}
            else if(S==TEXT("Iris")){C=IrisColor;Rough=.16;}
            else if(S==TEXT("Pupil")){C={.002,.002,.002};Rough=.2;}
            else {UE_LOG(LogTemp,Error,TEXT("GARDEN_CHARACTER_UNKNOWN_MATERIAL_SLOT %s %s"),*N,*S);return 5;}
            Materials.Add(S,Material(N+TEXT("_")+S,C,Metal,Rough,Tex,false,nullptr,false,S==TEXT("Skin"),SkinAlbedo));
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
    if(!Material(TEXT("M_TissueFieldProbe"),FLinearColor::Black,0,1))return 7;
    if(!Material(TEXT("M_AppearanceBackdrop"),FLinearColor(.18f,.18f,.18f),0,.9f))return 7;
    FAssetCompilingManager::Get().FinishAllCompilation();return 0;
}
