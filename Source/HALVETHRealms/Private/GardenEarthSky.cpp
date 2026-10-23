#include "HALVETHRealmWorld.h"
#include "HALVETHAstralSanctum.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/VolumetricCloudComponent.h"
#include "Components/PostProcessComponent.h"
#include "Materials/MaterialInterface.h"
#include "Misc/DateTime.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Serialization/JsonSerializer.h"
#include "ProceduralMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "ViewSkyMath.h"

void AHALVETHRealmWorld::BuildNightStars(){
    StarField=NewObject<UProceduralMeshComponent>(this,TEXT("AngularNightStars"));AddInstanceComponent(StarField);StarField->SetupAttachment(RootComponent);
    StarField->SetMobility(EComponentMobility::Movable);StarField->SetCollisionEnabled(ECollisionEnabled::NoCollision);StarField->SetCastShadow(false);
    TArray<FVector> Vertices,Normals;TArray<int32> Indices;TArray<FVector2D> UV;TArray<FLinearColor> Colors;TArray<FProcMeshTangent> Tangents;
    const auto Field=HalvethView::stars(0x68414c56,1800);constexpr double Distance=1000000.;
    for(const auto& Star:Field){
        const FVector D(Star.direction.x,Star.direction.y,Star.direction.z);
        const FVector R=FVector::CrossProduct(FMath::Abs(D.Z)>.9?FVector::RightVector:FVector::UpVector,D).GetSafeNormal();
        const FVector U=FVector::CrossProduct(D,R);const double Size=Distance*Star.radius;const int32 Base=Vertices.Num();
        const FLinearColor Color=FMath::Lerp(FLinearColor(.63f,.76f,1),FLinearColor(1,.83f,.64f),float(Star.color));
        for(const FVector2D Corner:{FVector2D(-1,-1),FVector2D(1,-1),FVector2D(1,1),FVector2D(-1,1)}){
            Vertices.Add(D*Distance+(R*Corner.X+U*Corner.Y)*Size);Normals.Add(-D);UV.Add((Corner+FVector2D(1,1))*.5);
            Colors.Add(FLinearColor(Color.R,Color.G,Color.B,float(Star.brightness)));Tangents.Add(FProcMeshTangent(R,false));
        }
        Indices.Append({Base,Base+2,Base+1,Base,Base+3,Base+2});
    }
    StarField->CreateMeshSection_LinearColor(0,Vertices,Indices,Normals,UV,Colors,Tangents,false);
    if(auto* Material=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Materials/M_AngularStars.M_AngularStars"))){
        StarMaterial=UMaterialInstanceDynamic::Create(Material,StarField);StarField->SetMaterial(0,StarMaterial);
    }else UE_LOG(LogTemp,Error,TEXT("GARDEN_ANGULAR_STAR_MATERIAL_MISSING"));
    StarField->RegisterComponent();Generated.Add(StarField);StarCount=int32(Field.size());
    UE_LOG(LogTemp,Display,TEXT("GARDEN_ANGULAR_STARS points=%d meshes=1 collision=NONE angular_radius_min=.00065 angular_radius_max=.00155 distribution=seeded_uniform_sphere"),StarCount);
}

void AHALVETHRealmWorld::InitializeEarthSky(){
    SunDirections.Empty();SkyClockUnix=FDateTime::UtcNow().ToUnixTimestamp();SkyTimeScale=1;
    SkyMinHeight=1;SkyMaxHeight=-1;SkyObservedSamples=0;SkyNightSamples=0;SkyDaySamples=0;
    FParse::Value(FCommandLine::Get(),TEXT("HalvethSkyUnix="),SkyClockUnix);
    FParse::Value(FCommandLine::Get(),TEXT("HalvethSkyTimeScale="),SkyTimeScale);
    FString Raw;TSharedPtr<FJsonObject> Doc;
    if(FFileHelper::LoadFileToString(Raw,*(FPaths::ProjectContentDir()/TEXT("sky/ephemeris.json")))&&FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Raw),Doc)&&Doc.IsValid()){
        const TArray<TSharedPtr<FJsonValue>>* Samples=nullptr;
        if(Doc->TryGetArrayField(TEXT("samples"),Samples)&&Samples->Num()==25){
            SkyFirstUnix=(*Samples)[0]->AsObject()->GetNumberField(TEXT("unixSeconds"));SkyLastUnix=(*Samples)[24]->AsObject()->GetNumberField(TEXT("unixSeconds"));
            for(auto& Item:*Samples){auto S=Item->AsObject();double Az=FMath::DegreesToRadians(S->GetNumberField(TEXT("azimuthDegrees"))),Alt=FMath::DegreesToRadians(S->GetNumberField(TEXT("altitudeDegrees")));
                SunDirections.Add(FVector(FMath::Cos(Alt)*FMath::Sin(Az),FMath::Cos(Alt)*FMath::Cos(Az),FMath::Sin(Alt)));
            }
            UE_LOG(LogTemp,Display,TEXT("GARDEN_NASA_SKY_BOUND samples=25 coverage_hours=24 observer_virtual_lat=45 lon=0 current_clock_in_coverage=%d weather_source=authored"),SkyClockUnix>=SkyFirstUnix&&SkyClockUnix<=SkyLastUnix);
        }
    }
    if(SunDirections.Num()!=25)UE_LOG(LogTemp,Warning,TEXT("GARDEN_NASA_SKY_MISSING cached_horizons_data_required"));
    BuildNightStars();
    TickEarthSky(0);
}

void AHALVETHRealmWorld::TickEarthSky(float Dt){
    // Translation follows the observer, rotation never follows their look.
    // Distant directions therefore have no local parallax or enclosing walls.
    if(StarField)if(auto* PC=GetWorld()->GetFirstPlayerController()){
        FVector Eye;FRotator View;PC->GetPlayerViewPoint(Eye,View);StarField->SetWorldLocation(Eye);
    }
    SkyClockUnix+=Dt*SkyTimeScale;SkyUpdateTime+=Dt;if(Dt>0&&SkyUpdateTime<.20)return;SkyUpdateTime=0;
    if(SunDirections.Num()!=25)return;
    double Time=SkyClockUnix-SkyFirstUnix;
    // Offline days repeat the explicitly dated trajectory; play refreshes it.
    Time=FMath::Fmod(Time,86400.);if(Time<0)Time+=86400.;double Fraction=Time/3600;int I=FMath::Clamp(FMath::FloorToInt(Fraction),0,23);
    FVector Direction=FMath::Lerp(SunDirections[I],SunDirections[I+1],Fraction-I).GetSafeNormal();
    double Height=Direction.Z;Sun->SetRelativeRotation((-Direction).Rotation());
    Sun->SetIntensity(Height>-.02?42000*FMath::Clamp((Height+.02)/.12,0.,1.):0);
    const double Warm=FMath::Clamp((.25-Height)/.25,0.,1.);
    Sun->SetLightColor(FMath::Lerp(FLinearColor(1,.98,.92),FLinearColor(1,.32,.12),Warm));
    Sky->SetIntensity(FMath::Lerp(.18,1.65,FMath::Clamp((Height+.10)/.25,0.,1.)));
    const double Day=FMath::Clamp((Height+.08)/.18,0.,1.);
    NightFill->SetIntensity(100*(1-Day)); // Authored gameplay fill, not a physical Moon model.
    if(auto* Sanctum=GetAstralSanctum())Sanctum->SetDaylight(Day);
    // A bounded histogram exposure can compensate retained indirect lighting
    // during an accelerated sunset, instead of forcing instant night gain.
    PostProcess->Settings.AutoExposureMinBrightness=FMath::Lerp(7.5,13.,Day);
    PostProcess->Settings.AutoExposureMaxBrightness=13;
    PostProcess->Settings.bOverride_AutoExposureSpeedUp=true;
    PostProcess->Settings.bOverride_AutoExposureSpeedDown=true;
    PostProcess->Settings.AutoExposureSpeedUp=8;
    PostProcess->Settings.AutoExposureSpeedDown=2;
    if(StarField){
        const double Night=1-FMath::SmoothStep(-.15,.015,Height);StarField->SetVisibility(Night>.001);
        if(StarMaterial)StarMaterial->SetScalarParameterValue(TEXT("NightVisibility"),float(Night));
        const double Angle=FMath::Fmod(SkyClockUnix-SkyFirstUnix,86164.)*2*PI/86164.;
        StarField->SetWorldRotation(FQuat(FVector(0,.7071067811865475,.7071067811865475),Angle));
    }
    SkyMinHeight=FMath::Min(SkyMinHeight,Height);SkyMaxHeight=FMath::Max(SkyMaxHeight,Height);
    SkyObservedSamples++;if(Height<-.08)SkyNightSamples++;if(Height>.1)SkyDaySamples++;
}

bool AHALVETHRealmWorld::VerifySkyDynamics() const{
    const bool Timelapse=FMath::Abs(SkyTimeScale)>1;
    const bool LightSlots=Sun->GetAtmosphereSunLightIndex()==0&&NightFill->GetAtmosphereSunLightIndex()==1&&NightFill->GetAtmosphereSunDiskColorScale().IsAlmostBlack();
    const bool Passed=LightSlots&&SunDirections.Num()==25&&StarField&&StarMaterial&&StarCount==1800&&Clouds&&SkyObservedSamples>1
        &&(!Timelapse||(SkyNightSamples>0&&SkyDaySamples>0&&SkyMinHeight<-.1&&SkyMaxHeight>.1));
    UE_LOG(LogTemp,Display,TEXT("GARDEN_SKY_AUDIT passed=%d source_samples=%d render_observations=%d day_observations=%d night_observations=%d min_sun_z=%.5f max_sun_z=%.5f time_scale=%.3f cloud_component=%d decorative_stars=%d"),Passed,SunDirections.Num(),SkyObservedSamples,SkyDaySamples,SkyNightSamples,SkyMinHeight,SkyMaxHeight,SkyTimeScale,Clouds!=nullptr,StarCount);
    UE_LOG(LogTemp,Display,TEXT("GARDEN_ATMOSPHERE_ROLES passed=%d sun_slot=%u fill_slot=%u fill_visible_disk=NONE"),LightSlots,Sun->GetAtmosphereSunLightIndex(),NightFill->GetAtmosphereSunLightIndex());
    return Passed;
}
