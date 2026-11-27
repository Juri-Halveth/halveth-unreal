#include "HALVETHRealmWorld.h"
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

void AHALVETHRealmWorld::InitializeEarthSky(){
    SunDirections.Empty();SkyClockUnix=FDateTime::UtcNow().ToUnixTimestamp();SkyTimeScale=1;
    SkyMinHeight=1;SkyMaxHeight=-1;SkyObservedSamples=0;SkyNightSamples=0;SkyDaySamples=0;
    FParse::Value(FCommandLine::Get(),TEXT("HalvethSkyUnix="),SkyClockUnix);
    FParse::Value(FCommandLine::Get(),TEXT("HalvethSkyTimeScale="),SkyTimeScale);
    FString Raw;TSharedPtr<FJsonObject> Doc;
    if(FFileHelper::LoadFileToString(Raw,*(FPaths::ProjectDir()/TEXT("assets/sky/ephemeris.json")))&&FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Raw),Doc)&&Doc.IsValid()){
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
    for(int I=0;I<240;I++){
        double Z=.1+.9*(I+.5)/240,A=I*2.399963229728653;FVector Direction(FMath::Sqrt(1-Z*Z)*FMath::Cos(A),FMath::Sqrt(1-Z*Z)*FMath::Sin(A),Z);
        auto* Star=Shape(SphereMesh,Direction*90000,FVector(.6f+.25f*(I%3)),FLinearColor(.7,.82,1),18,false);
        Star->SetCastShadow(false);Star->SetVisibility(false);Stars.Add(Star);
    }
    TickEarthSky(0);
}

void AHALVETHRealmWorld::TickEarthSky(float Dt){
    SkyClockUnix+=Dt*SkyTimeScale;SkyUpdateTime+=Dt;if(Dt>0&&SkyUpdateTime<.20)return;SkyUpdateTime=0;
    if(SunDirections.Num()!=25)return;
    double Time=SkyClockUnix-SkyFirstUnix;
    // Offline days repeat the explicitly dated trajectory; play refreshes it.
    Time=FMath::Fmod(Time,86400.);if(Time<0)Time+=86400.;double Fraction=Time/3600;int I=FMath::Clamp(FMath::FloorToInt(Fraction),0,23);
    FVector Direction=FMath::Lerp(SunDirections[I],SunDirections[I+1],Fraction-I).GetSafeNormal();
    double Height=Direction.Z;Sun->SetRelativeRotation((-Direction).Rotation());
    Sun->SetIntensity(Height>-.02?48000*FMath::Clamp((Height+.02)/.12,0.,1.):0);
    const double Warm=FMath::Clamp((.25-Height)/.25,0.,1.);
    Sun->SetLightColor(FMath::Lerp(FLinearColor(1,.98,.92),FLinearColor(1,.32,.12),Warm));
    Sky->SetIntensity(FMath::Lerp(.18,1.4,FMath::Clamp((Height+.10)/.25,0.,1.)));
    const double Day=FMath::Clamp((Height+.08)/.18,0.,1.);
    NightFill->SetIntensity(.30*(1-Day));
    // A bounded histogram exposure can compensate retained indirect lighting
    // during an accelerated sunset, instead of forcing instant night gain.
    PostProcess->Settings.AutoExposureMinBrightness=FMath::Lerp(2.,13.,Day);
    PostProcess->Settings.AutoExposureMaxBrightness=13;
    PostProcess->Settings.bOverride_AutoExposureSpeedUp=true;
    PostProcess->Settings.bOverride_AutoExposureSpeedDown=true;
    PostProcess->Settings.AutoExposureSpeedUp=8;
    PostProcess->Settings.AutoExposureSpeedDown=2;
    for(auto Star:Stars)if(Star)Star->SetVisibility(Height<-.08);
    SkyMinHeight=FMath::Min(SkyMinHeight,Height);SkyMaxHeight=FMath::Max(SkyMaxHeight,Height);
    SkyObservedSamples++;if(Height<-.08)SkyNightSamples++;if(Height>.1)SkyDaySamples++;
}

bool AHALVETHRealmWorld::VerifySkyDynamics() const{
    const bool Timelapse=FMath::Abs(SkyTimeScale)>1;
    const bool Passed=SunDirections.Num()==25&&Stars.Num()==240&&Clouds&&SkyObservedSamples>1
        &&(!Timelapse||(SkyNightSamples>0&&SkyDaySamples>0&&SkyMinHeight<-.1&&SkyMaxHeight>.1));
    UE_LOG(LogTemp,Display,TEXT("GARDEN_SKY_AUDIT passed=%d source_samples=%d render_observations=%d day_observations=%d night_observations=%d min_sun_z=%.5f max_sun_z=%.5f time_scale=%.3f cloud_component=%d decorative_stars=%d"),Passed,SunDirections.Num(),SkyObservedSamples,SkyDaySamples,SkyNightSamples,SkyMinHeight,SkyMaxHeight,SkyTimeScale,Clouds!=nullptr,Stars.Num());
    return Passed;
}
