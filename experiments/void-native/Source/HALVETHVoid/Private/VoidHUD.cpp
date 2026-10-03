#include "VoidHUD.h"
#include "VoidCharacter.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "GameFramework/PlayerController.h"
void AVoidHUD::DrawHUD() {
    Super::DrawHUD(); if (!Canvas) return;
    AVoidCharacter* Player = Cast<AVoidCharacter>(GetOwningPawn());
    const float W = Canvas->SizeX, H = Canvas->SizeY;
    DrawRect(FLinearColor(.02f,.03f,.04f,.7f),26,H-55,FMath::Min(W-52,760.0f),36);
    DrawText(TEXT("HALVETH  /  VOID"), FLinearColor(.91f,.88f,.76f), 38, 30, GEngine->GetMediumFont(), 1.7f);
    DrawText(TEXT("WASD  Wandern   |   Maus  Blick   |   Shift  Laufen   |   I  Gepaeck   |   Esc  Schliessen"), FLinearColor(.85f,.87f,.83f), 38, H-44, GEngine->GetSmallFont(), 1);
    if (!Player || !Player->bInventory) {
        DrawLine(W*.5f-4,H*.5f,W*.5f+4,H*.5f,FLinearColor(1,1,1,.65f));
        DrawLine(W*.5f,H*.5f-4,W*.5f,H*.5f+4,FLinearColor(1,1,1,.65f)); return;
    }
    const float X=W*.13f, Y=H*.18f, PW=W*.74f, PH=H*.62f;
    DrawRect(FLinearColor(.025f,.035f,.04f,.94f),X,Y,PW,PH);
    DrawText(TEXT("REISEGEPAECK"),FLinearColor(.93f,.87f,.67f),X+32,Y+25,GEngine->GetMediumFont(),2.0f);
    const TArray<FString> Names{TEXT("QUELLENSTEIN"),TEXT("OASENSAMEN"),TEXT("PORTALFRAGMENT")};
    const TArray<FString> Details{
        TEXT("Ein mineralischer Fund vom Seeufer.\nGewicht 0.3 kg  |  Staerke 2  |  Wert 12\nResonanz: Wasser +4"),
        TEXT("Samen einer Pflanze dieser neuen Welt.\nGewicht 0.1 kg  |  Wachstum 7  |  Wert 8\nLebenskraft +3"),
        TEXT("Ein Fragment mit einer offenen Geschichte.\nGewicht 0.8 kg  |  Ladung 0/10  |  Wert 40\nZauber: Verbindung (noch ungeladen)")};
    float MX=0,MY=0; GetOwningPlayerController()->GetMousePosition(MX,MY);
    for(int32 I=0;I<3;I++) {
        const float CY=Y+96+I*87;
        const bool Hover=MX>=X+26 && MX<=X+PW*.4f && MY>=CY && MY<=CY+67;
        DrawRect(Hover?FLinearColor(.24f,.28f,.25f,1):FLinearColor(.09f,.12f,.13f,1),X+26,CY,PW*.36f,67);
        DrawText(Names[I],FLinearColor(.92f,.91f,.84f),X+45,CY+22,GEngine->GetMediumFont(),1.3f);
        if(Hover) DrawText(Details[I],FLinearColor(.87f,.9f,.84f),X+PW*.45f,Y+126,GEngine->GetMediumFont(),1.3f);
    }
    DrawText(TEXT("Bewege den Mauszeiger ueber einen Fund. Die Welt laeuft weiter."),FLinearColor(.64f,.73f,.7f),X+32,Y+PH-43,GEngine->GetSmallFont(),1);
}
