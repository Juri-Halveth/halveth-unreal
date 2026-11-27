#include "HALVETHGameMode.h"
#include "HALVETHCharacter.h"
#include "HALVETHTrainingTarget.h"
#include "HALVETHRealmWorld.h"
#include "HALVETHAdventureComponent.h"
#include "HALVETHKnowledgeComponent.h"
#include "HALVETHGuideCharacter.h"
#include "HALVETHGuideAnimInstance.h"
#include "Components/SkeletalMeshComponent.h"
#include "GardenField.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "HighResScreenshot.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "HAL/PlatformMisc.h"

// Explicit development capture mode. Real game rendering and real spell impacts;
// every save is routed to the isolated test slot by HalvethVisual.
void AHALVETHGameMode::RunVisualStep() {
    auto* P=Cast<AHALVETHCharacter>(UGameplayStatics::GetPlayerPawn(this,0));
    auto* PC=GetWorld()->GetFirstPlayerController(); if(!P||!PC||!RealmWorld) return;
    const float Times[]={12,18,21,25,27,28.4f,29.8f,31.2f,32.6f,39,42,47,49,55,57,63,65,71,78,81,85,88,92,95,99,102,105};
    if(VisualStep>=UE_ARRAY_COUNT(Times)||VisualElapsed<Times[VisualStep]) return;
    auto View=[P,PC](FVector Position,FVector Target) {
        P->GetCharacterMovement()->SetMovementMode(MOVE_Flying);
        P->SetActorLocation(Position,false,nullptr,ETeleportType::TeleportPhysics);
        PC->SetControlRotation((Target-(Position+FVector(0,0,66))).Rotation());
    };
    auto Survey=[this,P,&View](double X,double Y,int32 Realm,FVector Target) {
        FHitResult Support;
        FCollisionQueryParams Params(SCENE_QUERY_STAT(GardenCaptureSupport),true,P);
        double Z=GardenField::Height(X,Y,Realm);
        if(GetWorld()->LineTraceSingleByChannel(Support,FVector(X,Y,12000),FVector(X,Y,-12000),ECC_Visibility,Params))
            Z=FMath::Max(Z,double(Support.ImpactPoint.Z));
        View(FVector(X,Y,Z+200),Target);
    };
    auto GuideView=[this,&View](int32 Identity,FVector CameraOffset,float TargetHeight) {
        const FVector Position=RealmWorld->GetGuidePosition(Identity);
        View(Position+CameraOffset,Position+FVector(0,0,TargetHeight));
    };
    auto Shot=[this](const FString& Name) {
        for(int Identity=1;Identity<=3;Identity++)if(auto* Guide=RealmWorld->GetGuideCharacter(Identity))
            if(auto* Motion=Cast<UHALVETHGuideAnimInstance>(Guide->GetMesh()->GetAnimInstance()))Motion->LogPoseAudit(*Name);
        FScreenshotRequest::RequestScreenshot(FPaths::ProjectDir()/TEXT("Preview")/(Name+TEXT(".png")),true,false);
        UE_LOG(LogTemp,Display,TEXT("GARDEN_NATIVE_CAPTURE %s"),*Name);
    };
    auto* T=Cast<AHALVETHTrainingTarget>(UGameplayStatics::GetActorOfClass(this,AHALVETHTrainingTarget::StaticClass()));
    switch(VisualStep) {
        case 0:
            // Only this explicit automated capture owns its camera/input.
            // Ordinary play retains all controls and ordinary movement physics.
            P->DisableInput(PC); PC->SetIgnoreLookInput(true); PC->SetIgnoreMoveInput(true);
            P->GetCharacterMovement()->SetMovementMode(MOVE_Flying);
            ApplyQuality(2,false); View(FVector(-1050,-1800,110),FVector(0,550,210)); break;
        case 1: Shot(TEXT("GARDEN_HUB")); break;
        case 2: View(FVector(430,-940,100),FVector(430,-590,150)); if(T) T->ResetTarget(); P->GetAdventure()->CycleAbility(); break;
        case 3: Shot(TEXT("GARDEN_CRYSTAL_READY")); break;
        case 4: case 5: case 6: case 7:
            if(T) PC->SetControlRotation((T->GetActorLocation()+FVector(0,0,10)-(P->GetActorLocation()+FVector(0,0,66))).Rotation());
            if(!P->GetAdventure()->ActivateAbility()) bVisualFailed=true; break;
        case 8: if(!T||!T->IsBroken()||T->GetHealth()!=0) bVisualFailed=true; Shot(TEXT("GARDEN_CRYSTAL_BROKEN")); break;
        case 9: if(!T||T->IsBroken()||T->GetHealth()!=100) bVisualFailed=true; Shot(TEXT("GARDEN_CRYSTAL_REFORMED")); break;
        case 10: Survey(-9000,-11500,0,FVector(0,10000,1500)); break;
        case 11: Shot(TEXT("GARDEN_LANDSCAPE")); break;
        case 12: if(!Travel(1)) bVisualFailed=true; Survey(-5000,-4500,1,FVector(1000,17000,1300)); break;
        case 13: Shot(TEXT("GARDEN_SCARLET")); break;
        case 14:
            TravelCooldown=0; if(!Travel(0)) bVisualFailed=true;
            TravelCooldown=0; if(!Travel(2)) bVisualFailed=true;
            Survey(-4500,4500,2,FVector(3000,15000,450)); break;
        case 15: Shot(TEXT("GARDEN_TIDELIGHT")); break;
        case 16:
            TravelCooldown=0; if(!Travel(0)) bVisualFailed=true;
            TravelCooldown=0; if(!Travel(3)) bVisualFailed=true;
            Survey(-7000,-5500,3,FVector(1500,18000,1500)); break;
        case 17: Shot(TEXT("GARDEN_MEMORY")); break;
        case 18:
            TravelCooldown=0; if(!Travel(0)) bVisualFailed=true;
            GuideView(1,FVector(0,-310,55),104); break;
        case 19: Shot(TEXT("GARDEN_SCARLET_CHARACTER")); break;
        case 20: GuideView(1,FVector(0,-140,92),164); break;
        case 21: Shot(TEXT("GARDEN_SCARLET_FACE")); break;
        case 22: GuideView(2,FVector(0,-310,55),104); break;
        case 23: Shot(TEXT("GARDEN_LUCINET_CHARACTER")); break;
        case 24: GuideView(3,FVector(0,-310,55),104); break;
        case 25: Shot(TEXT("GARDEN_RACHEL_CHARACTER")); break;
        case 26:
            bVisualTest=false;
            FFileHelper::SaveStringToFile(bVisualFailed?TEXT("{\"spell_cycle_passed\":false,\"capture_mode\":\"native_rendering\"}"):TEXT("{\"spell_cycle_passed\":true,\"capture_mode\":\"native_rendering\",\"sampled_views\":12}"),*(FPaths::ProjectDir()/TEXT("QA/visual-runtime.json")));
            FPlatformMisc::RequestExitWithStatus(false,bVisualFailed?3:0); break;
    }
    VisualStep++;
}
