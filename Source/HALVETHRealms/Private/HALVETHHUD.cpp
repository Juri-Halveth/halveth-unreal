#include "HALVETHHUD.h"
#include "HALVETHCharacter.h"
#include "HALVETHAdventureComponent.h"
#include "HALVETHKnowledgeComponent.h"
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
    if (auto* Knowledge = Pawn->GetKnowledge(); Knowledge && Knowledge->IsReading())
    {
        const float ReaderScale = FMath::Clamp(Height / 1000.0f, 0.72f, 1.2f);
        const float Margin = 28 * ReaderScale;
        const float Top = 112 * ReaderScale;
        const float Bottom = Height - 147 * ReaderScale;
        const float Split = Width * 0.54f;
        DrawRect(FLinearColor(0.018f, 0.014f, 0.028f, 0.97f), 0, 0, Width, Height);
        DrawText(TEXT("HALVETH  /  THE LIVING LIBRARY"), Gold, Margin, 20 * ReaderScale, Font, 1.8f * ReaderScale);
        DrawText(TEXT("Read. Remember. Make something that changes the world."), White, Margin, 55 * ReaderScale, Font, ReaderScale);
        const TCHAR* Panels[] = {TEXT("CRAFT"), TEXT("BUILD"), TEXT("COUNCIL"), TEXT("BAG")};
        const float TabWidth = (Width - Split - Margin * 1.5f) / 4;
        for (int32 Index = 0; Index < 4; ++Index)
        {
            const float X = Split + Margin * 0.5f + TabWidth * Index;
            DrawRect(Index == Pawn->GetReaderPanel() ? FLinearColor(0.32f, 0.12f, 0.19f) : FLinearColor(0.055f, 0.075f, 0.09f), X, 76 * ReaderScale, TabWidth - 4, 29 * ReaderScale);
            DrawText(Panels[Index], Index == Pawn->GetReaderPanel() ? Gold : White, X + 10 * ReaderScale, 81 * ReaderScale, Font, ReaderScale);
        }
        DrawText(TEXT("O switches the workshop panel"), Gold, Margin, 84 * ReaderScale, Font, 0.8f * ReaderScale);
        DrawRect(FLinearColor(0.095f, 0.042f, 0.069f, 0.96f), Margin, Top, Split - Margin * 1.5f, Bottom - Top);
        DrawRect(FLinearColor(0.04f, 0.08f, 0.095f, 0.96f), Split + Margin * 0.5f, Top, Width - Split - Margin * 1.5f, Bottom - Top);
        auto Column = [&](const TArray<FString>& Lines, float X, float MaxWidth)
        {
            float Y = Top + 18 * ReaderScale;
            for (int32 Index = 0; Index < Lines.Num(); ++Index)
            {
                if (Y > Bottom - 42 * ReaderScale) break;
                Y = Wrapped(Lines[Index], X, Y, MaxWidth, Index == 0 ? Gold : White,
                    (Index == 0 ? 1.45f : 1.35f) * ReaderScale) + 10 * ReaderScale;
            }
        };
        Column(Knowledge->GetBookLines(), Margin * 1.65f, Split - Margin * 2.8f);
        Column(Knowledge->GetWorkshopLines(Pawn->GetReaderPanel()), Split + Margin * 1.15f, Width - Split - Margin * 2.8f);
        const float StatusY = Bottom + 10 * ReaderScale;
        Wrapped(Knowledge->GetStatusText(), Margin, StatusY, Width - Margin * 2, Gold, 1.05f * ReaderScale);
        Wrapped(TEXT("B / Esc close | Tab book | PgUp / PgDn page | 1 / 2 / 3 answer | C recipe / enchantment | V craft"), Margin, Height - 77 * ReaderScale, Width - Margin * 2, White, ReaderScale);
        Wrapped(TEXT("O panels | P pack draught | T plan | B then G build | N request | Z / X / Y council route | F5 save | F9 reload"), Margin, Height - 49 * ReaderScale, Width - Margin * 2, White, ReaderScale);
        Wrapped(TEXT("Read at your pace. Insight is earned once; practical skill grows when you apply it."), Margin, Height - 23 * ReaderScale, Width - Margin * 2, Gold, 0.94f * ReaderScale);
        return;
    }
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
        Wrapped(TEXT("WASD move | Mouse look | Space jump | Shift run | Ctrl dodge | E gather / talk / portal"), 30, Height - 88 * Scale, Width - 60, White, 0.9f * Scale);
        Wrapped(TEXT("Q choose spell | Left click cast | L LOVE | I choose item | F use item"), 30, Height - 57 * Scale, Width - 60, White, 0.9f * Scale);
        Wrapped(TEXT("B library | T choose structure / G build ahead | R home | 1 / 2 / 3 graphics | H help | F10 credits | Esc save + quit"), 30, Height - 27 * Scale, Width - 60, Gold, 0.8f * Scale);
    }
    if (auto* Adventure = Pawn->GetAdventure())
    {
        const float PanelY = Height - (Pawn->IsHelpVisible() ? 232 : 132) * Scale;
        DrawRect(FLinearColor(0.018f, 0.014f, 0.032f, 0.88f), 20, PanelY, Width - 40, 122 * Scale);
        const float BarWidth = FMath::Min(230.0f * Scale, (Width - 100) / 3);
        const float Values[] = {Adventure->GetHealth(), Adventure->GetMana(), Adventure->GetStamina()};
        const float Maxima[] = {100.0f, Adventure->GetMaxMana(), 100.0f};
        const TCHAR* Names[] = {TEXT("HEALTH"), TEXT("MANA"), TEXT("STAMINA")};
        const FLinearColor Colors[] = {FLinearColor(0.86f, 0.18f, 0.34f), FLinearColor(0.14f, 0.62f, 0.91f), FLinearColor(0.92f, 0.65f, 0.23f)};
        for (int32 Index = 0; Index < 3; ++Index)
        {
            const float X = 34 + Index * (BarWidth + 20);
            DrawText(FString::Printf(TEXT("%s %.0f/%.0f"), Names[Index], Values[Index], Maxima[Index]), White, X, PanelY + 9 * Scale, Font, 0.8f * Scale);
            DrawRect(FLinearColor(0.12f, 0.11f, 0.17f), X, PanelY + 33 * Scale, BarWidth, 5 * Scale);
            DrawRect(Colors[Index], X, PanelY + 33 * Scale, BarWidth * Values[Index] / Maxima[Index], 5 * Scale);
        }
        Wrapped(TEXT("[Q / CLICK]  ") + Adventure->GetAbilityText(), 34, PanelY + 46 * Scale, Width - 70, Gold, 0.8f * Scale);
        Wrapped(TEXT("[I / F]  ") + Adventure->GetInventoryText(), 34, PanelY + 76 * Scale, Width - 70, White, 0.75f * Scale);
        DrawRect(FLinearColor(0.018f, 0.014f, 0.032f, 0.78f), 20, 157 * Scale, FMath::Min(780.0f * Scale, Width - 40), 92 * Scale);
        Wrapped(Adventure->GetInteractionText(), 34, 172 * Scale, FMath::Min(746.0f * Scale, Width - 70), White, 0.85f * Scale);
    }
    const int32 Destination = Realm->FindPortal(Pawn->GetActorLocation());
    if (auto* Knowledge = Pawn->GetKnowledge())
    {
        DrawRect(FLinearColor(0.018f, 0.014f, 0.032f, 0.78f), 20, 255 * Scale, FMath::Min(780.0f * Scale, Width - 40), 88 * Scale);
        const float NextLine = Wrapped(Knowledge->GetConstructionHint(), 34, 265 * Scale, FMath::Min(746.0f * Scale, Width - 70), Gold, 0.75f * Scale);
        Wrapped(Knowledge->GetStatusText(), 34, NextLine + 3 * Scale, FMath::Min(746.0f * Scale, Width - 70), White, 0.72f * Scale);
    }
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
