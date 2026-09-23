#include "HALVETHHUD.h"
#include "HALVETHCharacter.h"
#include "HALVETHAdventureComponent.h"
#include "HALVETHGameMode.h"
#include "HALVETHRealmWorld.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Kismet/GameplayStatics.h"

void AHALVETHHUD::DrawHUD()
{
    Super::DrawHUD();
    if (!Canvas) return;
    auto* Mode = Cast<AHALVETHGameMode>(UGameplayStatics::GetGameMode(this));
    auto* Pawn = Cast<AHALVETHCharacter>(GetOwningPawn());
    if (!Mode || !Pawn || !Mode->GetRealmWorld()) return;
    auto* Realm = Mode->GetRealmWorld();
    const float Width = Canvas->ClipX;
    const float Height = Canvas->ClipY;
    const float Scale = FMath::Clamp(Width / 1600, 0.75f, 1.5f);
    const FLinearColor Gold(0.94f, 0.79f, 0.52f);
    const FLinearColor White(0.91f, 0.91f, 0.96f);
    UFont* Font = GEngine->GetMediumFont();
    auto Wrapped = [&](const FString& Text, float X, float Y, float MaxWidth, FLinearColor Color, float TextScale)
    {
        TArray<FString> Words; Text.ParseIntoArrayWS(Words);
        FString Line;
        for (const FString& Word : Words)
        {
            const FString Trial = Line.IsEmpty() ? Word : Line + TEXT(" ") + Word;
            float W = 0, H = 0; GetTextSize(Trial, W, H, Font, TextScale);
            if (W > MaxWidth && !Line.IsEmpty()) { DrawText(Line, Color, X, Y, Font, TextScale); Y += 23 * TextScale; Line = Word; }
            else Line = Trial;
        }
        if (!Line.IsEmpty()) DrawText(Line, Color, X, Y, Font, TextScale);
        return Y + 24 * TextScale;
    };
    DrawRect(FLinearColor(0.018f, 0.014f, 0.032f, 0.8f), 0, 0, Width, 112 * Scale);
    DrawText(TEXT("HALVETH  /  PORTAL GARDEN"), Gold, 30, 18, Font, Scale);
    DrawText(AHALVETHRealmWorld::RealmName(Realm->GetCurrentRealm()), White, 30, 48 * Scale, Font, 1.5f * Scale);
    DrawText(AHALVETHRealmWorld::RealmDescription(Realm->GetCurrentRealm()), Gold, 30, 85 * Scale, Font, 0.8f * Scale);
    const FString Quality = Mode->GetQuality() == 0 ? TEXT("Performance") : Mode->GetQuality() == 1 ? TEXT("Balanced") : TEXT("Epic");
    DrawText(FString::Printf(TEXT("Seed %d  |  %s  |  Local prototype"), Mode->WorldSeed, *Quality), Gold,
        FMath::Max(30.0f, Width - 410 * Scale), 20, Font, 0.85f * Scale);
    DrawRect(FLinearColor(1, 0.82f, 0.6f, 0.9f), Width / 2 - 2, Height / 2 - 2, 4, 4);
    if (Pawn->IsHelpVisible())
    {
        DrawRect(FLinearColor(0.018f, 0.014f, 0.032f, 0.88f), 0, Height - 100 * Scale, Width, 100 * Scale);
        Wrapped(TEXT("WASD move | Mouse look | Space jump | Shift run | Ctrl dodge | E talk / portal"), 30, Height - 88 * Scale, Width - 60, White, 0.9f * Scale);
        Wrapped(TEXT("Q choose spell | Left click cast | L LOVE | I choose item | F use item"), 30, Height - 57 * Scale, Width - 60, White, 0.9f * Scale);
        Wrapped(TEXT("R home | 1 / 2 / 3 graphics | H help | F10 credits | Esc quit"), 30, Height - 27 * Scale, Width - 60, Gold, 0.8f * Scale);
    }
    if (auto* Adventure = Pawn->GetAdventure())
    {
        const float PanelY = Height - (Pawn->IsHelpVisible() ? 232 : 132) * Scale;
        DrawRect(FLinearColor(0.018f, 0.014f, 0.032f, 0.88f), 20, PanelY, Width - 40, 122 * Scale);
        const float BarWidth = FMath::Min(230.0f * Scale, (Width - 100) / 3);
        const float Values[] = {Adventure->GetHealth(), Adventure->GetMana(), Adventure->GetStamina()};
        const TCHAR* Names[] = {TEXT("HEALTH"), TEXT("MANA"), TEXT("STAMINA")};
        const FLinearColor Colors[] = {FLinearColor(0.86f, 0.18f, 0.34f), FLinearColor(0.14f, 0.62f, 0.91f), FLinearColor(0.92f, 0.65f, 0.23f)};
        for (int32 Index = 0; Index < 3; ++Index)
        {
            const float X = 34 + Index * (BarWidth + 20);
            DrawText(FString::Printf(TEXT("%s %.0f"), Names[Index], Values[Index]), White, X, PanelY + 9 * Scale, Font, 0.8f * Scale);
            DrawRect(FLinearColor(0.12f, 0.11f, 0.17f), X, PanelY + 33 * Scale, BarWidth, 5 * Scale);
            DrawRect(Colors[Index], X, PanelY + 33 * Scale, BarWidth * Values[Index] / 100, 5 * Scale);
        }
        Wrapped(TEXT("[Q / CLICK]  ") + Adventure->GetAbilityText(), 34, PanelY + 46 * Scale, Width - 70, Gold, 0.8f * Scale);
        Wrapped(TEXT("[I / F]  ") + Adventure->GetInventoryText(), 34, PanelY + 76 * Scale, Width - 70, White, 0.75f * Scale);
        DrawRect(FLinearColor(0.018f, 0.014f, 0.032f, 0.78f), 20, 157 * Scale, FMath::Min(780.0f * Scale, Width - 40), 92 * Scale);
        Wrapped(Adventure->GetInteractionText(), 34, 172 * Scale, FMath::Min(746.0f * Scale, Width - 70), White, 0.85f * Scale);
    }
    const int32 Destination = Realm->FindPortal(Pawn->GetActorLocation());
    if (Destination != INDEX_NONE)
    {
        const FString Prompt = TEXT("E  -  ENTER ") + AHALVETHRealmWorld::RealmName(Destination);
        DrawRect(FLinearColor(0.09f, 0.02f, 0.08f, 0.9f), Width / 2 - 275 * Scale, Height * 0.65f, 550 * Scale, 48 * Scale);
        DrawText(Prompt, White, Width / 2 - 250 * Scale, Height * 0.65f + 12 * Scale, Font, Scale);
    }
    if (Mode->GetMessageRemaining() > 0)
        DrawText(Mode->GetMessage(), White, 30, 128 * Scale, Font, 0.95f * Scale);
    if (Realm->GetLoveStrength() > 0)
    {
        DrawRect(FLinearColor(1, 0.08f, 0.25f, Realm->GetLoveStrength() * 0.08f), 0, 0, Width, Height);
        DrawText(TEXT("LOVE"), FLinearColor(1, 0.45f, 0.55f), Width / 2 - 35 * Scale, Height * 0.72f, Font, 2 * Scale);
    }
    if (Pawn->IsCreditsVisible())
    {
        const float X = Width * 0.1f, Y = Height * 0.30f;
        DrawRect(FLinearColor(0.015f, 0.012f, 0.027f, 0.98f), X, Y, Width * 0.8f, 265 * Scale);
        float Line = Wrapped(TEXT("HALVETH PORTAL GARDEN  /  CREDITS"), X + 22, Y + 20, Width * 0.8f - 44, Gold, Scale);
        Line = Wrapped(TEXT("Original project code: MIT. Original Scarlet Heart icon: HALVETH Realms contributors. PBR textures: Poly Haven, CC0."), X + 22, Line + 12, Width * 0.8f - 44, White, 0.85f * Scale);
        Line = Wrapped(TEXT("HALVETH Portal Garden uses Unreal\u00ae Engine. Unreal\u00ae is a trademark or registered trademark of Epic Games, Inc. in the United States of America and elsewhere."), X + 22, Line + 12, Width * 0.8f - 44, White, 0.85f * Scale);
        Line = Wrapped(TEXT("Unreal\u00ae Engine, Copyright 1998 - 2026, Epic Games, Inc. All rights reserved."), X + 22, Line + 12, Width * 0.8f - 44, White, 0.85f * Scale);
        Wrapped(TEXT("F10 closes credits. Full licenses accompany the build."), X + 22, Line + 12, Width * 0.8f - 44, Gold, 0.8f * Scale);
    }
}
