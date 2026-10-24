// SPDX-License-Identifier: MIT
#include "HumanAppearanceRuntime.h"
#include "HumanAppearance.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Materials/MaterialInstanceDynamic.h"

void HalvethAppearanceRuntime::Apply(USkeletalMeshComponent* Mesh,uint32 Seed){
    if(!Mesh||!Mesh->GetSkeletalMeshAsset())return;
    const auto Traits=HalvethAppearance::traits(Seed);
    for(std::size_t I=0;I<Traits.morph.size();I++)
        Mesh->SetMorphTarget(FName(UTF8_TO_TCHAR(HalvethAppearance::Morphs[I])),Traits.morph[I]);
    for(int32 I=0;I<Mesh->GetNumMaterials();I++){
        auto* Parent=Mesh->GetMaterial(I);if(!Parent)continue;
        const auto& Slots=Mesh->GetSkeletalMeshAsset()->GetMaterials();
        if(!Slots.IsValidIndex(I)||!Slots[I].MaterialSlotName.ToString().StartsWith(TEXT("Skin")))continue;
        auto* Material=UMaterialInstanceDynamic::Create(Parent,Mesh);FLinearColor Source=FLinearColor::White;
        Parent->GetVectorParameterValue(FMaterialParameterInfo(TEXT("SkinTone")),Source);
        Source.R*=Traits.skinMultiplier*(1.+Traits.warmth);
        Source.G*=Traits.skinMultiplier;Source.B*=Traits.skinMultiplier*(1.-Traits.warmth);
        Material->SetVectorParameterValue(TEXT("SkinTone"),Source);Mesh->SetMaterial(I,Material);
    }
    UE_LOG(LogTemp,Display,TEXT("HUMAN_APPEARANCE mesh=%s seed=%u morphs=4 skin=profile_microdetail wardrobe=removed adult=true"),*Mesh->GetSkeletalMeshAsset()->GetName(),Seed);
}
