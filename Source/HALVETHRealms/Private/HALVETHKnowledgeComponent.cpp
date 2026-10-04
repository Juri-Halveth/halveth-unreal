#include "HALVETHKnowledgeComponent.h"
#include "HALVETHAdventureComponent.h"
#include "KnowledgeSystem.h"

#include "CollisionQueryParams.h"
#include "CollisionShape.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Dom/JsonObject.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformFileManager.h"
#include "HAL/PlatformProcess.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/App.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Guid.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include <cstdio>
#if PLATFORM_WINDOWS
#include "Windows/WindowsHWrapper.h"
#endif

namespace HK = halveth::knowledge;

struct FHALVETHConstruction
{
    int32 Kind = 0;
    int32 Realm = 0;
    FVector Position = FVector::ZeroVector;
    float Yaw = 0;
};

struct FHALVETHKnowledgeRuntime
{
    HK::KnowledgeSystem Knowledge;
    TArray<FHALVETHConstruction> Constructions;
    int32 LastAdventureItems[3] = {3, 2, 2};
};

void FHALVETHKnowledgeRuntimeDeleter::operator()(FHALVETHKnowledgeRuntime* Pointer) const
{
    delete Pointer;
}

namespace
{
    FString Text(std::string_view Value)
    {
        const FUTF8ToTCHAR Converted(Value.data(), static_cast<int32>(Value.size()));
        return FString(Converted.Length(), Converted.Get());
    }

    bool ValidRealm(int32 Realm) { return Realm >= 0 && Realm < 4; }

    FVector NodePosition(int32 Node)
    {
        const FVector Points[] = {FVector(-320,-870,35), FVector(350,-520,35),
            FVector(-340,150,35), FVector(330,620,35)};
        return Points[Node % 4];
    }

    UStaticMeshComponent* Piece(AActor* Actor, const TCHAR* Asset, FVector Position, FVector Scale,
        FLinearColor Color, float Glow = 0, bool Collision = true, FRotator Rotation = FRotator::ZeroRotator)
    {
        auto* Mesh = NewObject<UStaticMeshComponent>(Actor);
        Actor->AddInstanceComponent(Mesh);
        Mesh->SetupAttachment(Actor->GetRootComponent());
        Mesh->SetMobility(EComponentMobility::Movable);
        Mesh->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, Asset));
        Mesh->SetRelativeLocation(Position);
        Mesh->SetRelativeRotation(Rotation);
        Mesh->SetRelativeScale3D(Scale);
        Mesh->SetCollisionProfileName(Collision ? TEXT("BlockAll") : TEXT("NoCollision"));
        Mesh->SetGenerateOverlapEvents(false);
        UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr,
            TEXT("/Game/Materials/M_HalvethSurface.M_HalvethSurface"));
        if (!Base) Base = UMaterial::GetDefaultMaterial(MD_Surface);
        auto* Material = UMaterialInstanceDynamic::Create(Base, Mesh);
        Material->SetVectorParameterValue(TEXT("Tint"), Color);
        Material->SetScalarParameterValue(TEXT("Glow"), Glow);
        Mesh->SetMaterial(0, Material);
        Mesh->RegisterComponent();
        return Mesh;
    }

    void Lamp(AActor* Actor, FVector Position, FLinearColor Color, float Intensity, float Radius)
    {
        auto* Light = NewObject<UPointLightComponent>(Actor);
        Actor->AddInstanceComponent(Light);
        Light->SetupAttachment(Actor->GetRootComponent());
        Light->SetMobility(EComponentMobility::Movable);
        Light->SetRelativeLocation(Position);
        Light->SetLightColor(Color);
        Light->SetIntensityUnits(ELightUnits::Lumens);
        Light->SetIntensity(Intensity);
        Light->SetAttenuationRadius(Radius);
        Light->SetCastShadows(false);
        Light->RegisterComponent();
    }

    void Label(AActor* Actor, const FString& Value, float Height)
    {
        auto* Label = NewObject<UTextRenderComponent>(Actor);
        Actor->AddInstanceComponent(Label);
        Label->SetupAttachment(Actor->GetRootComponent());
        Label->SetRelativeLocation(FVector(0,-75,Height));
        Label->SetRelativeRotation(FRotator(0,-90,0));
        Label->SetWorldSize(17);
        Label->SetHorizontalAlignment(EHTA_Center);
        Label->SetText(FText::FromString(Value));
        Label->SetTextRenderColor(FColor(180,240,225));
        Label->RegisterComponent();
    }

    AActor* EmptyActor(UWorld* World, FVector Position, float Yaw)
    {
        if (!World) return nullptr;
        FActorSpawnParameters Parameters;
        Parameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        AActor* Actor = World->SpawnActor<AActor>(Position, FRotator(0,Yaw,0), Parameters);
        if (!Actor) return nullptr;
        auto* Scene = NewObject<USceneComponent>(Actor);
        Actor->AddInstanceComponent(Scene);
        Scene->SetMobility(EComponentMobility::Movable);
        Actor->SetRootComponent(Scene);
        Scene->RegisterComponent();
        Actor->SetActorLocationAndRotation(Position, FRotator(0,Yaw,0));
        return Actor;
    }

    template <typename Values>
    TArray<TSharedPtr<FJsonValue>> NumberArray(const Values& Input)
    {
        TArray<TSharedPtr<FJsonValue>> Output;
        for (auto Value : Input) Output.Add(MakeShared<FJsonValueNumber>(static_cast<double>(Value)));
        return Output;
    }

    bool Integer(const TSharedPtr<FJsonObject>& Object, const TCHAR* Key, double Low, double High, double& Out)
    {
        return Object.IsValid() && Object->TryGetNumberField(Key, Out) && FMath::IsFinite(Out)
            && Out >= Low && Out <= High && Out == FMath::FloorToDouble(Out);
    }

    template <typename Values>
    bool ReadIntegers(const TSharedPtr<FJsonObject>& Object, const TCHAR* Key, Values& Out, int Low, int High)
    {
        const TArray<TSharedPtr<FJsonValue>>* ValuesIn = nullptr;
        if (!Object->TryGetArrayField(Key, ValuesIn) || ValuesIn->Num() != static_cast<int32>(Out.size())) return false;
        for (int32 I = 0; I < ValuesIn->Num(); ++I)
        {
            double Number = 0;
            if (!(*ValuesIn)[I].IsValid() || !(*ValuesIn)[I]->TryGetNumber(Number) || !FMath::IsFinite(Number)
                || Number < Low || Number > High || Number != FMath::FloorToDouble(Number)) return false;
            Out[I] = static_cast<int>(Number);
        }
        return true;
    }

    bool AtomicReplace(const FString& Temp, const FString& Destination)
    {
        const FString From = FPaths::ConvertRelativePathToFull(Temp);
        const FString To = FPaths::ConvertRelativePathToFull(Destination);
#if PLATFORM_WINDOWS
        // Generic IFileManager::Move deletes the destination first. Keep the prior save
        // present until the operating system replaces it in one same-volume operation.
        return ::MoveFileExW(*From, *To, MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != 0;
#elif PLATFORM_UNIX || PLATFORM_MAC
        return std::rename(TCHAR_TO_UTF8(*From), TCHAR_TO_UTF8(*To)) == 0;
#else
        return false;
#endif
    }

    template <typename Definition>
    FString CostLine(const Definition& Value, const HK::KnowledgeSystem& Knowledge)
    {
        FString Result = TEXT("Cost: ");
        for (std::size_t I = 0; I < Value.costCount; ++I)
        {
            if (I) Result += TEXT(" | ");
            const auto& Cost = Value.costs[I];
            Result += FString::Printf(TEXT("%d %s (%d)"), Cost.quantity, *Text(HK::ItemName(Cost.item)),
                Knowledge.GetItemCount(Cost.item));
        }
        return Result;
    }

    template <typename Definition>
    FString RequiredPages(const Definition& Value, const HK::KnowledgeSystem& Knowledge)
    {
        FString Result = TEXT("Recall: ");
        for (std::size_t I = 0; I < Value.requirementCount; ++I)
        {
            if (I) Result += TEXT("; ");
            const auto& Page = Value.requirements[I];
            Result += FString::Printf(TEXT("%s p.%d %s"), *Text(HK::Books()[Page.document].title),
                static_cast<int32>(Page.page) + 1,
                Knowledge.GetPageProgress(Page.document, Page.page).recalled ? TEXT("[learned]") : TEXT("[needed]"));
        }
        return Result;
    }
}

UHALVETHKnowledgeComponent::UHALVETHKnowledgeComponent()
    : Runtime(new FHALVETHKnowledgeRuntime())
{
    PrimaryComponentTick.bCanEverTick = true;
}

UHALVETHKnowledgeComponent::~UHALVETHKnowledgeComponent() = default;

void UHALVETHKnowledgeComponent::Feedback(const FString& Value)
{
    StatusText = Value;
    UE_LOG(LogTemp, Display, TEXT("HALVETH_KNOWLEDGE %s"), *Value);
}

void UHALVETHKnowledgeComponent::InitializeWorld(uint32 Seed, int32 Realm)
{
    if (!ValidRealm(Realm)) return;
    if (bInitialized && WorldSeed == Seed) { OnRealmChanged(Realm); return; }
    if (bInitialized && bDirty) SaveProgress();
    WorldSeed = Seed;
    CurrentRealm = Realm;
    Runtime.Reset(new FHALVETHKnowledgeRuntime());
    Runtime->Knowledge = HK::KnowledgeSystem(Seed);
    ActiveBook = ActivePage = SelectedRecipe = SelectedStructure = SelectedQuest = 0;
    bInitialized = true;
    bReading = bDirty = bPreserveInvalidSave = false;
    bSmokeSlot = FParse::Param(FCommandLine::Get(), TEXT("HalvethSmokeTest"))
        || FParse::Param(FCommandLine::Get(), TEXT("HalvethKnowledgeSmokeTest"))
        || FParse::Param(FCommandLine::Get(), TEXT("HalvethVisual"));
    // Smoke runs begin from deterministic fresh state and never load the player's slot.
    if (!bSmokeSlot) ReloadProgress();
    RefreshBuiltActors();
}

void UHALVETHKnowledgeComponent::OnRealmChanged(int32 Realm)
{
    if (!bInitialized || !ValidRealm(Realm)) return;
    if (bDirty) SaveProgress();
    CurrentRealm = Realm;
    bReading = false;
    RefreshBuiltActors();
}

void UHALVETHKnowledgeComponent::TickComponent(float DeltaTime, ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    if (!bInitialized || !GetWorld() || GetWorld()->IsPaused() || !FMath::IsFinite(DeltaTime) || DeltaTime <= 0) return;
    const float Step = FMath::Min(DeltaTime, 0.1f);
    const auto* Pawn = Cast<APawn>(GetOwner());
    if (bReading && ValidRealm(CurrentRealm) && Pawn && Pawn->IsLocallyControlled() && FApp::HasFocus())
    {
        const auto Result = Runtime->Knowledge.RecordActiveReading(ActiveBook, ActivePage, Step);
        if (Result.ok) bDirty = true;
    }
    if (auto* Adventure = GetOwner()->FindComponentByClass<UHALVETHAdventureComponent>())
    {
        for (int32 I = 0; I < 3; ++I)
            if (Runtime->LastAdventureItems[I] != Adventure->GetItemCount(I)) bDirty = true;
    }
    AutosaveElapsed += Step;
    if (AutosaveElapsed >= 5)
    {
        AutosaveElapsed = 0;
        if (bDirty && !bPreserveInvalidSave) SaveProgress();
    }
}

void UHALVETHKnowledgeComponent::EndPlay(const EEndPlayReason::Type Reason)
{
    if (bInitialized && bDirty && !bPreserveInvalidSave) SaveProgress();
    for (AActor* Actor : BuiltActors) if (IsValid(Actor)) Actor->Destroy();
    BuiltActors.Empty();
    Super::EndPlay(Reason);
}

void UHALVETHKnowledgeComponent::ToggleReading()
{
    if (!bInitialized) return;
    bReading = !bReading;
    if (!bReading && bDirty) SaveProgress();
    if (bReading) Feedback(TEXT("Read at your pace. 1 / 2 / 3 answers the page; each first recall earns knowledge once."));
}

void UHALVETHKnowledgeComponent::OpenBook(int32 Index)
{
    if (!bInitialized || Index < 0 || Index >= static_cast<int32>(HK::BookCount)) return;
    ActiveBook = Index;
    ActivePage = 0;
    bReading = true;
    Feedback(Text(HK::Books()[Index].title));
}

void UHALVETHKnowledgeComponent::NextBook()
{
    if (!bInitialized || !bReading) return;
    ActiveBook = (ActiveBook + 1) % HK::BookCount;
    ActivePage = 0;
}

void UHALVETHKnowledgeComponent::NextPage()
{
    if (bInitialized && bReading) ActivePage = (ActivePage + 1) % HK::Books()[ActiveBook].pageCount;
}

void UHALVETHKnowledgeComponent::PreviousPage()
{
    if (!bInitialized || !bReading) return;
    const int32 Count = static_cast<int32>(HK::Books()[ActiveBook].pageCount);
    ActivePage = (ActivePage + Count - 1) % Count;
}

bool UHALVETHKnowledgeComponent::RecallPage(int32 Choice)
{
    if (!bInitialized || !bReading || Choice < 0 || Choice > 2) return false;
    if (Runtime->Knowledge.GetPageProgress(ActiveBook, ActivePage).recalled)
    { Feedback(TEXT("This page is already learned. Its knowledge reward is recorded once.")); return false; }
    const auto Result = Runtime->Knowledge.Recall(ActiveBook, ActivePage, Choice);
    Feedback(Text(Result.message));
    bDirty = true; // Failed recall attempts are also a journal entry.
    SaveProgress();
    return Result.ok;
}

void UHALVETHKnowledgeComponent::CycleRecipe()
{
    if (bInitialized && bReading) SelectedRecipe = (SelectedRecipe + 1) % (HK::RecipeCount + HK::EnchantmentCount);
}

bool UHALVETHKnowledgeComponent::CraftSelected()
{
    if (!bInitialized || !bReading) return false;
    const auto Result = SelectedRecipe < static_cast<int32>(HK::RecipeCount)
        ? Runtime->Knowledge.Craft(SelectedRecipe)
        : Runtime->Knowledge.Enchant(SelectedRecipe - HK::RecipeCount);
    Feedback(Text(Result.message));
    if (Result.ok) { bDirty = true; SaveProgress(); }
    return Result.ok;
}

bool UHALVETHKnowledgeComponent::PackDraught()
{
    if (!bInitialized || !bReading || !GetOwner()) return false;
    auto* Adventure = GetOwner()->FindComponentByClass<UHALVETHAdventureComponent>();
    if (!Adventure || Adventure->GetItemCount(0) >= 99)
    { Feedback(TEXT("The healing-item pocket is full. Use an item before packing another draught.")); return false; }
    const HK::State Before = Runtime->Knowledge.ExportState();
    const auto Result = Runtime->Knowledge.ConsumeItem(HK::ItemId::LumenDraught, 1);
    if (!Result.ok) { Feedback(Text(Result.message)); return false; }
    if (!Adventure->GrantConsumable(0, 1))
    {
        Runtime->Knowledge.ImportState(Before);
        Feedback(TEXT("The draught stayed in the workshop; no inventory space was available."));
        return false;
    }
    Feedback(TEXT("Lumen Draught packed: close the book, choose it with I and use it with F to restore 40 health."));
    bDirty = true;
    SaveProgress();
    return true;
}

void UHALVETHKnowledgeComponent::CycleStructure()
{
    if (bInitialized) SelectedStructure = (SelectedStructure + 1) % HK::ModuleCount;
}

bool UHALVETHKnowledgeComponent::BuildSelected()
{
    if (!bInitialized || !GetOwner() || !GetWorld()) return false;
    const auto Eligibility = Runtime->Knowledge.CanBuild(SelectedStructure);
    if (!Eligibility.ok) { Feedback(Text(Eligibility.message)); return false; }
    if (Runtime->Constructions.Num() >= HK::MaxBuilds) return false;
    FVector Forward = GetOwner()->GetActorForwardVector();
    Forward.Z = 0;
    const FVector Desired = GetOwner()->GetActorLocation() + Forward.GetSafeNormal() * 380;
    if (Desired.SizeSquared2D() > FMath::Square(2000.0))
    { Feedback(TEXT("Build within the garden's solid ground, closer to the central path.")); return false; }
    FCollisionQueryParams Query(SCENE_QUERY_STAT(HalvethConstruction), false, GetOwner());
    FHitResult Ground;
    if (!GetWorld()->LineTraceSingleByChannel(Ground, Desired + FVector(0,0,450), Desired - FVector(0,0,700), ECC_Visibility, Query)
        || Ground.ImpactNormal.Z < 0.9f || Ground.ImpactPoint.Z < -20 || Ground.ImpactPoint.Z > 120)
    { Feedback(TEXT("Aim toward a flat patch of ground. Nothing was spent.")); return false; }
    const FVector Location = Ground.ImpactPoint;
    for (const auto& Existing : Runtime->Constructions)
        if (Existing.Realm == CurrentRealm && FVector::DistSquared2D(Existing.Position, Location) < FMath::Square(320.0))
        { Feedback(TEXT("Leave room between constructions. Move to another patch of ground.")); return false; }
    if (GetWorld()->OverlapBlockingTestByChannel(Location + FVector(0,0,130), FQuat::Identity, ECC_Pawn,
        FCollisionShape::MakeBox(FVector(125,125,105)), Query))
    { Feedback(TEXT("That place is occupied. Move or turn before building; no materials were used.")); return false; }
    const float Yaw = FRotator::ClampAxis(GetOwner()->GetActorRotation().Yaw);
    AActor* Built = SpawnStructure(SelectedStructure, Location, Yaw);
    if (!Built) { Feedback(TEXT("The structure could not be placed. Materials are unchanged.")); return false; }
    const auto Result = Runtime->Knowledge.Build(SelectedStructure);
    if (!Result.ok) { Built->Destroy(); Feedback(Text(Result.message)); return false; }
    Runtime->Constructions.Add({SelectedStructure, CurrentRealm, Location, Yaw});
    BuiltActors.Add(Built);
    bDirty = true;
    Feedback(Text(Result.message));
    SaveProgress();
    return true;
}

void UHALVETHKnowledgeComponent::CycleQuest()
{
    if (bInitialized && bReading) SelectedQuest = (SelectedQuest + 1) % HK::QuestCount;
}

bool UHALVETHKnowledgeComponent::ChooseQuest(int32 Choice)
{
    if (!bInitialized || !bReading || Choice < 0 || Choice > 2) return false;
    const auto Result = Runtime->Knowledge.ChooseQuest(SelectedQuest, Choice);
    Feedback(Text(Result.message));
    if (Result.ok) { bDirty = true; SaveProgress(); }
    return Result.ok;
}

bool UHALVETHKnowledgeComponent::GatherNearby()
{
    if (!bInitialized || bReading || !GetOwner()) return false;
    const auto State = Runtime->Knowledge.ExportState();
    for (int32 Local = 0; Local < 4; ++Local)
    {
        const int32 Node = CurrentRealm * 4 + Local;
        if (FVector::DistSquared2D(GetOwner()->GetActorLocation(), NodePosition(Node)) > FMath::Square(180.0)) continue;
        const auto Result = Runtime->Knowledge.Gather(Node);
        Feedback(Text(Result.message));
        if (Result.ok)
        {
            const auto After = Runtime->Knowledge.ExportState();
            for (std::size_t I = 0; I < HK::ItemCount; ++I)
                if (After.inventory[I] > State.inventory[I])
                    Feedback(FString::Printf(TEXT("Gathered %d %s. This resource renews; press E again to gather more."),
                        After.inventory[I] - State.inventory[I], *Text(HK::ItemName(static_cast<HK::ItemId>(I)))));
            bDirty = true; SaveProgress();
        }
        return true;
    }
    return false;
}

float UHALVETHKnowledgeComponent::GetSpellCostMultiplier() const
{
    return bInitialized ? 1.0f - 0.05f * Runtime->Knowledge.ExportState().enchantmentRanks[1] : 1.0f;
}

float UHALVETHKnowledgeComponent::GetMaxManaBonus() const
{
    return bInitialized ? 10.0f * Runtime->Knowledge.ExportState().enchantmentRanks[0] : 0.0f;
}

int32 UHALVETHKnowledgeComponent::GetBuiltCount() const { return Runtime->Constructions.Num(); }

int32 UHALVETHKnowledgeComponent::GetVisibleBuiltCount() const
{
    int32 Count = 0;
    for (const AActor* Actor : BuiltActors)
        if (IsValid(Actor) && Actor->ActorHasTag(TEXT("HALVETH_PLAYER_CONSTRUCTION"))) ++Count;
    return Count;
}

int32 UHALVETHKnowledgeComponent::GetKnowledgeRank() const
{
    if (!bInitialized) return 0;
    return Runtime->Knowledge.GetLevel(HK::Skill::Alchemy) + Runtime->Knowledge.GetLevel(HK::Skill::Enchantment)
        + Runtime->Knowledge.GetLevel(HK::Skill::Architecture);
}

int32 UHALVETHKnowledgeComponent::GetMaterialCount(int32 Index) const
{
    if (!bInitialized || Index < 0 || Index >= static_cast<int32>(HK::ItemCount)) return 0;
    return Runtime->Knowledge.GetItemCount(static_cast<HK::ItemId>(Index));
}

int32 UHALVETHKnowledgeComponent::GetActiveReadingMilliseconds() const
{
    return bInitialized ? static_cast<int32>(Runtime->Knowledge.GetPageProgress(ActiveBook, ActivePage).activeMilliseconds) : 0;
}

TArray<FString> UHALVETHKnowledgeComponent::GetBookLines() const
{
    TArray<FString> Lines;
    if (!bInitialized) { Lines.Add(TEXT("The library is preparing.")); return Lines; }
    const auto& Book = HK::Books()[ActiveBook];
    const auto& Page = Book.pages[ActivePage];
    const auto Progress = Runtime->Knowledge.GetPageProgress(ActiveBook, ActivePage);
    Lines.Add(FString::Printf(TEXT("%s  |  %d / %d"), *Text(Book.kind), ActiveBook + 1, static_cast<int32>(HK::BookCount)));
    Lines.Add(Text(Book.title));
    Lines.Add(Text(Book.author));
    Lines.Add(FString::Printf(TEXT("Page %d / %d  |  %.1fs active reading  |  %s"), ActivePage + 1,
        static_cast<int32>(Book.pageCount), Progress.activeMilliseconds / 1000.0,
        Progress.recalled ? TEXT("Learned") : TEXT("Ready to recall")));
    Lines.Add(Text(Page.text));
    Lines.Add(Text(Page.prompt));
    for (int32 I = 0; I < 3; ++I) Lines.Add(FString::Printf(TEXT("[%d] %s"), I + 1, *Text(Page.answers[I])));
    Lines.Add(TEXT("Tab: next book    PgUp / PgDn: pages    1 / 2 / 3: answer"));
    return Lines;
}

FString UHALVETHKnowledgeComponent::GetConstructionHint() const
{
    if (!bInitialized) return TEXT("The workshop is preparing.");
    const auto& Module = HK::Modules()[SelectedStructure];
    const auto Ready = Runtime->Knowledge.CanBuild(SelectedStructure);
    return FString::Printf(TEXT("Plan: %s | %s | %s | T: choose  G: build ahead"),
        *Text(Module.name), *CostLine(Module, Runtime->Knowledge),
        Ready.ok ? TEXT("Ready") : *Text(Ready.message));
}

TArray<FString> UHALVETHKnowledgeComponent::GetWorkshopLines(int32 Panel) const
{
    TArray<FString> Lines;
    if (!bInitialized) return Lines;
    const auto& Knowledge = Runtime->Knowledge;
    const auto State = Knowledge.ExportState();
    Lines.Add(FString::Printf(TEXT("Alchemy %d  |  Enchantment %d  |  Architecture %d"), Knowledge.GetLevel(HK::Skill::Alchemy),
        Knowledge.GetLevel(HK::Skill::Enchantment), Knowledge.GetLevel(HK::Skill::Architecture)));
    if (Panel == 0)
    {
        Lines.Add(TEXT("WORKSHOP | O: next panel (1 / 4)"));
        if (SelectedRecipe < static_cast<int32>(HK::RecipeCount))
        {
            const auto& Recipe = HK::Recipes()[SelectedRecipe];
            Lines.Add(FString::Printf(TEXT("CRAFT: %s  %s"), *Text(Recipe.name), Knowledge.IsRecipeUnlocked(SelectedRecipe) ? TEXT("[known]") : TEXT("[learn pages]")));
            Lines.Add(CostLine(Recipe, Knowledge));
            Lines.Add(RequiredPages(Recipe, Knowledge));
        }
        else
        {
            const int32 Index = SelectedRecipe - HK::RecipeCount;
            const auto& Enchantment = HK::Enchantments()[Index];
            Lines.Add(FString::Printf(TEXT("ENCHANT: %s  rank %d / 3"), *Text(Enchantment.name), State.enchantmentRanks[Index]));
            Lines.Add(Index == 0 ? TEXT("Each rank adds 10 maximum mana.") : TEXT("Each rank reduces spell costs by 5%."));
            Lines.Add(CostLine(Enchantment, Knowledge));
            Lines.Add(RequiredPages(Enchantment, Knowledge));
        }
        Lines.Add(FString::Printf(TEXT("Ready: Draught %d  |  Ink %d  |  Resin %d"), Knowledge.GetItemCount(HK::ItemId::LumenDraught),
            Knowledge.GetItemCount(HK::ItemId::WardInk), Knowledge.GetItemCount(HK::ItemId::BindingResin)));
        Lines.Add(TEXT("C: recipe / enchantment    V: make"));
        Lines.Add(TEXT("P: move a draught to the healing-item pocket (+40 HP when used)"));
        Lines.Add(TEXT("Keep draughts here when making Quiet Step. Field crafting works anywhere."));
    }
    else if (Panel == 1)
    {
        const auto& Module = HK::Modules()[SelectedStructure];
        Lines.Add(TEXT("CONSTRUCTION | O: next panel (2 / 4)"));
        Lines.Add(FString::Printf(TEXT("BUILD: %s  |  %d / 32 placed"), *Text(Module.name), Runtime->Constructions.Num()));
        Lines.Add(CostLine(Module, Knowledge));
        Lines.Add(RequiredPages(Module, Knowledge));
        Lines.Add(Text(Knowledge.CanBuild(SelectedStructure).message));
        Lines.Add(TEXT("T: choose structure. G: close this book; press G again in the world to build."));
        Lines.Add(TEXT("Structures are placed 3.8 metres ahead on clear, flat ground. Leave room between them."));
        Lines.Add(TEXT("A Ground workbench is the settlement milestone for Camp and Beacon; later structures can stand in any realm."));
    }
    else if (Panel == 2)
    {
        const auto& Quest = HK::Quests()[SelectedQuest];
        Lines.Add(TEXT("COUNCIL | O: next panel (3 / 4)"));
        Lines.Add(FString::Printf(TEXT("%s"), *Text(Quest.title)));
        if (State.questChoices[SelectedQuest] >= 0)
            Lines.Add(Text(Quest.choices[State.questChoices[SelectedQuest]].outcome));
        else
        {
            Lines.Add(Text(Quest.summary));
            const TCHAR* Keys[] = {TEXT("Z"), TEXT("X"), TEXT("Y")};
            for (int32 I = 0; I < 3; ++I) Lines.Add(FString::Printf(TEXT("[%s] %s"), Keys[I], *Text(Quest.choices[I].name)));
            if (Quest.prerequisiteQuest >= 0)
                Lines.Add(FString::Printf(TEXT("Prior decision: %s"), *Text(HK::Quests()[Quest.prerequisiteQuest].title)));
            if (Quest.requiresGround) Lines.Add(TEXT("A completed Ground workbench is required."));
        }
        Lines.Add(FString::Printf(TEXT("Kindness %d  |  Balance %d  |  Insight %d"), State.kindness, State.balance, State.insight));
        Lines.Add(TEXT("N: next decision. Each outcome and its reward are recorded once."));
    }
    else
    {
        Lines.Add(TEXT("MATERIALS | O: next panel (4 / 4)"));
        for (std::size_t I = 0; I < HK::ItemCount; ++I)
            Lines.Add(FString::Printf(TEXT("%s: %d"), *Text(HK::ItemName(static_cast<HK::ItemId>(I))), State.inventory[I]));
        Lines.Add(TEXT("Close the library and press E near garden resources. Every deliberate harvest renews its material; gathering grants no XP."));
    }
    Lines.Add(TEXT("F5: save    F9: reload    B: close"));
    return Lines;
}

TArray<FString> UHALVETHKnowledgeComponent::GetPanelLines() const
{
    TArray<FString> Lines = GetBookLines();
    Lines.Append(GetWorkshopLines());
    return Lines;
}

AActor* UHALVETHKnowledgeComponent::SpawnStructure(int32 Kind, FVector Location, float Yaw)
{
    if (Kind < 0 || Kind >= static_cast<int32>(HK::ModuleCount)) return nullptr;
    AActor* Actor = EmptyActor(GetWorld(), Location, Yaw);
    if (!Actor) return nullptr;
    Actor->Tags.Add(TEXT("HALVETH_PLAYER_CONSTRUCTION"));
    const TCHAR* Cube = TEXT("/Engine/BasicShapes/Cube.Cube");
    const TCHAR* Sphere = TEXT("/Engine/BasicShapes/Sphere.Sphere");
    const TCHAR* Cylinder = TEXT("/Engine/BasicShapes/Cylinder.Cylinder");
    const TCHAR* Cone = TEXT("/Engine/BasicShapes/Cone.Cone");
    const FLinearColor Stone(0.19f,0.24f,0.28f), Wood(0.22f,0.085f,0.035f), Mint(0.12f,0.85f,0.63f);
    if (Kind == 0)
    {
        Piece(Actor,Cube,FVector(0,0,10),FVector(2.4f,2.4f,0.2f),Stone);
        for (int32 X : {-1,1}) for (int32 Y : {-1,1})
            Piece(Actor,Cube,FVector(X*70,Y*40,55),FVector(0.13f,0.13f,0.8f),Wood);
        Piece(Actor,Cube,FVector(0,0,100),FVector(1.8f,1.15f,0.15f),Wood);
        Piece(Actor,Sphere,FVector(-40,0,122),FVector(0.25f,0.25f,0.4f),Mint,0.5f,false);
        Piece(Actor,Sphere,FVector(30,15,123),FVector(0.25f,0.25f,0.35f),FLinearColor(0.5f,0.1f,0.8f),0.5f,false);
        Label(Actor,TEXT("Your workbench"),155);
    }
    else if (Kind == 1)
    {
        Piece(Actor,Cylinder,FVector(0,0,12),FVector(1.15f,1.15f,0.18f),Stone);
        for (int32 I = 0; I < 3; ++I)
            Piece(Actor,Cylinder,FVector(0,0,28),FVector(0.17f,0.17f,1.15f),Wood,0,true,FRotator(90,I*60,0));
        Piece(Actor,Sphere,FVector(0,0,57),FVector(0.38f,0.38f,0.7f),FLinearColor(1.0f,0.26f,0.04f),5,false);
        Piece(Actor,Cube,FVector(0,100,26),FVector(1.5f,0.33f,0.5f),Wood);
        Lamp(Actor,FVector(0,0,100),FLinearColor(1.0f,0.35f,0.06f),2400,500);
        Label(Actor,TEXT("Your camp"),145);
    }
    else
    {
        Piece(Actor,Cylinder,FVector(0,0,17),FVector(1.1f,1.1f,0.34f),Stone);
        Piece(Actor,Cylinder,FVector(0,0,125),FVector(0.25f,0.25f,2.2f),Stone);
        Piece(Actor,Cone,FVector(0,0,258),FVector(0.58f,0.58f,0.8f),Mint,4,false);
        Piece(Actor,Cone,FVector(0,0,225),FVector(0.58f,0.58f,0.5f),Mint,4,false,FRotator(180,0,0));
        Lamp(Actor,FVector(0,0,245),Mint,3800,950);
        Label(Actor,TEXT("Your beacon"),305);
    }
    return Actor;
}

void UHALVETHKnowledgeComponent::RefreshBuiltActors()
{
    for (AActor* Actor : BuiltActors) if (IsValid(Actor)) Actor->Destroy();
    BuiltActors.Empty();
    if (!bInitialized || !GetWorld()) return;
    for (const auto& Record : Runtime->Constructions)
        if (Record.Realm == CurrentRealm)
            if (AActor* Actor = SpawnStructure(Record.Kind, Record.Position, Record.Yaw)) BuiltActors.Add(Actor);
    for (int32 Local = 0; Local < 4; ++Local)
    {
        const int32 Node = CurrentRealm * 4 + Local;
        AActor* Actor = EmptyActor(GetWorld(), NodePosition(Node), 0);
        if (!Actor) continue;
        Actor->Tags.Add(TEXT("HALVETH_GATHER_NODE"));
        Piece(Actor,TEXT("/Engine/BasicShapes/Cone.Cone"),FVector::ZeroVector,FVector(0.4f,0.4f,0.6f),FLinearColor(0.16f,0.8f,0.45f),1.4f,false);
        const auto Item = static_cast<HK::ItemId>((Node + WorldSeed % 7u) % 7u);
        Label(Actor,Text(HK::ItemName(Item)) + TEXT(" [E]"),70);
        BuiltActors.Add(Actor);
    }
}

FString UHALVETHKnowledgeComponent::SavePath() const
{
    if (bSmokeSlot)
        return FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Tests"),
            FString::Printf(TEXT("knowledge-%u-smoke.json"), WorldSeed));
    return FPaths::Combine(FPlatformProcess::UserSettingsDir(), TEXT("HALVETH"), TEXT("PortalGarden"),
        TEXT("SaveGames"), FString::Printf(TEXT("knowledge-%u.json"), WorldSeed));
}

bool UHALVETHKnowledgeComponent::SaveProgress()
{
    if (!bInitialized || bPreserveInvalidSave || !GetOwner()) return false;
    auto* Adventure = GetOwner()->FindComponentByClass<UHALVETHAdventureComponent>();
    if (!Adventure) { Feedback(TEXT("Saving awaits the adventure inventory.")); return false; }
    const auto State = Runtime->Knowledge.ExportState();
    TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();
    Root->SetNumberField(TEXT("schema"), 1);
    Root->SetNumberField(TEXT("version"), State.version);
    Root->SetNumberField(TEXT("worldSeed"), State.worldSeed);
    Root->SetNumberField(TEXT("gatheredMask"), State.gatheredMask);
    Root->SetArrayField(TEXT("inventory"), NumberArray(State.inventory));
    Root->SetArrayField(TEXT("xp"), NumberArray(State.xp));
    Root->SetArrayField(TEXT("craftedCounts"), NumberArray(State.craftedCounts));
    Root->SetArrayField(TEXT("enchantmentRanks"), NumberArray(State.enchantmentRanks));
    Root->SetArrayField(TEXT("builtCounts"), NumberArray(State.builtCounts));
    Root->SetArrayField(TEXT("questChoices"), NumberArray(State.questChoices));
    Root->SetNumberField(TEXT("kindness"), State.kindness);
    Root->SetNumberField(TEXT("balance"), State.balance);
    Root->SetNumberField(TEXT("insight"), State.insight);
    TArray<TSharedPtr<FJsonValue>> Discovered;
    for (bool Value : State.discoveredRecipes) Discovered.Add(MakeShared<FJsonValueBoolean>(Value));
    Root->SetArrayField(TEXT("discoveredRecipes"), Discovered);
    TArray<TSharedPtr<FJsonValue>> Reading;
    for (const auto& Book : State.reading)
    {
        TArray<TSharedPtr<FJsonValue>> Pages;
        for (const auto& Page : Book)
        {
            TSharedRef<FJsonObject> Record = MakeShared<FJsonObject>();
            Record->SetNumberField(TEXT("milliseconds"), Page.activeMilliseconds);
            Record->SetNumberField(TEXT("attempts"), Page.attempts);
            Record->SetBoolField(TEXT("recalled"), Page.recalled);
            Pages.Add(MakeShared<FJsonValueObject>(Record));
        }
        Reading.Add(MakeShared<FJsonValueArray>(Pages));
    }
    Root->SetArrayField(TEXT("reading"), Reading);
    TArray<TSharedPtr<FJsonValue>> Builds;
    for (const auto& Record : Runtime->Constructions)
    {
        TSharedRef<FJsonObject> Build = MakeShared<FJsonObject>();
        Build->SetNumberField(TEXT("kind"), Record.Kind);
        Build->SetNumberField(TEXT("realm"), Record.Realm);
        Build->SetNumberField(TEXT("x"), Record.Position.X);
        Build->SetNumberField(TEXT("y"), Record.Position.Y);
        Build->SetNumberField(TEXT("z"), Record.Position.Z);
        Build->SetNumberField(TEXT("yaw"), Record.Yaw);
        Builds.Add(MakeShared<FJsonValueObject>(Build));
    }
    Root->SetArrayField(TEXT("constructions"), Builds);
    const std::array<int,3> AdventureItems{Adventure->GetItemCount(0),Adventure->GetItemCount(1),Adventure->GetItemCount(2)};
    Root->SetArrayField(TEXT("adventureItems"), NumberArray(AdventureItems));
    FString Serialized;
    if (!FJsonSerializer::Serialize(Root, TJsonWriterFactory<>::Create(&Serialized))) return false;
    const FString Destination = SavePath();
    const FString Temp = Destination + TEXT(".") + FGuid::NewGuid().ToString(EGuidFormats::Digits) + TEXT(".tmp");
    IFileManager::Get().MakeDirectory(*FPaths::GetPath(Destination), true);
    if (!FFileHelper::SaveStringToFile(Serialized, *Temp, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM)
        || !AtomicReplace(Temp, Destination))
    {
        IFileManager::Get().Delete(*Temp, false, true, true);
        Feedback(TEXT("Progress is in memory; the local save could not be replaced. Try F5 again."));
        return false;
    }
    for (int32 I = 0; I < 3; ++I) Runtime->LastAdventureItems[I] = AdventureItems[I];
    bDirty = false;
    return true;
}

bool UHALVETHKnowledgeComponent::ReloadProgress()
{
    if (!bInitialized || !GetOwner()) return false;
    const FString Path = SavePath();
    IFileManager& Files = IFileManager::Get();
    if (!Files.FileExists(*Path)) return false;
    auto Reject = [this]()
    {
        bPreserveInvalidSave = true;
        Feedback(TEXT("This knowledge save is incompatible or damaged. The current world and the original file are preserved."));
        return false;
    };
    const int64 Size = Files.FileSize(*Path);
    if (Size <= 0 || Size > 65536) return Reject();
    FString Serialized;
    if (!FFileHelper::LoadFileToString(Serialized, *Path)) return Reject();
    TSharedPtr<FJsonObject> Root;
    if (!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Serialized), Root) || !Root.IsValid()) return Reject();
    double Number = 0;
    if (!Integer(Root,TEXT("schema"),1,1,Number)) return Reject();
    HK::State Candidate;
    if (!Integer(Root,TEXT("version"),1,1,Number)) return Reject();
    Candidate.version = static_cast<uint32>(Number);
    if (!Integer(Root,TEXT("worldSeed"),0,4294967295.0,Number) || Number != WorldSeed) return Reject();
    Candidate.worldSeed = static_cast<uint32>(Number);
    if (!Integer(Root,TEXT("gatheredMask"),0,65535,Number)) return Reject();
    Candidate.gatheredMask = static_cast<uint32>(Number);
    if (!ReadIntegers(Root,TEXT("inventory"),Candidate.inventory,0,999)
        || !ReadIntegers(Root,TEXT("xp"),Candidate.xp,0,10000)
        || !ReadIntegers(Root,TEXT("craftedCounts"),Candidate.craftedCounts,0,1000)
        || !ReadIntegers(Root,TEXT("enchantmentRanks"),Candidate.enchantmentRanks,0,3)
        || !ReadIntegers(Root,TEXT("builtCounts"),Candidate.builtCounts,0,32)
        || !ReadIntegers(Root,TEXT("questChoices"),Candidate.questChoices,-1,2)) return Reject();
    if (!Integer(Root,TEXT("kindness"),0,6,Number)) return Reject(); Candidate.kindness = static_cast<int>(Number);
    if (!Integer(Root,TEXT("balance"),0,6,Number)) return Reject(); Candidate.balance = static_cast<int>(Number);
    if (!Integer(Root,TEXT("insight"),0,6,Number)) return Reject(); Candidate.insight = static_cast<int>(Number);
    std::array<int,3> AdventureItems;
    if (!ReadIntegers(Root,TEXT("adventureItems"),AdventureItems,0,99)) return Reject();
    const TArray<TSharedPtr<FJsonValue>>* Values = nullptr;
    if (!Root->TryGetArrayField(TEXT("discoveredRecipes"),Values) || Values->Num() != HK::RecipeCount) return Reject();
    for (int32 I = 0; I < Values->Num(); ++I)
        if (!(*Values)[I]->TryGetBool(Candidate.discoveredRecipes[I])) return Reject();
    if (!Root->TryGetArrayField(TEXT("reading"),Values) || Values->Num() != HK::BookCount) return Reject();
    for (int32 I = 0; I < Values->Num(); ++I)
    {
        const TArray<TSharedPtr<FJsonValue>>* Pages = nullptr;
        if (!(*Values)[I]->TryGetArray(Pages) || Pages->Num() != HK::MaxPages) return Reject();
        for (int32 P = 0; P < Pages->Num(); ++P)
        {
            const TSharedPtr<FJsonObject>* Page = nullptr;
            if (!(*Pages)[P]->TryGetObject(Page) || !Page->IsValid()) return Reject();
            if (!Integer(*Page,TEXT("milliseconds"),0,HK::MaxPageMilliseconds,Number)) return Reject();
            Candidate.reading[I][P].activeMilliseconds = static_cast<uint32>(Number);
            if (!Integer(*Page,TEXT("attempts"),0,65535,Number)) return Reject();
            Candidate.reading[I][P].attempts = static_cast<uint16>(Number);
            if (!(*Page)->TryGetBoolField(TEXT("recalled"),Candidate.reading[I][P].recalled)) return Reject();
        }
    }
    if (!Root->TryGetArrayField(TEXT("constructions"),Values) || Values->Num() > HK::MaxBuilds) return Reject();
    TArray<FHALVETHConstruction> Constructions;
    std::array<int,3> Counts{};
    for (const auto& Value : *Values)
    {
        const TSharedPtr<FJsonObject>* Object = nullptr;
        if (!Value->TryGetObject(Object) || !Object->IsValid()) return Reject();
        FHALVETHConstruction Record;
        if (!Integer(*Object,TEXT("kind"),0,2,Number)) return Reject(); Record.Kind = static_cast<int>(Number);
        if (!Integer(*Object,TEXT("realm"),0,3,Number)) return Reject(); Record.Realm = static_cast<int>(Number);
        double X,Y,Z,Yaw;
        if (!(*Object)->TryGetNumberField(TEXT("x"),X) || !(*Object)->TryGetNumberField(TEXT("y"),Y)
            || !(*Object)->TryGetNumberField(TEXT("z"),Z) || !(*Object)->TryGetNumberField(TEXT("yaw"),Yaw)
            || !FMath::IsFinite(X) || !FMath::IsFinite(Y) || !FMath::IsFinite(Z) || !FMath::IsFinite(Yaw)
            || FMath::Square(X)+FMath::Square(Y) > FMath::Square(2000.0) || Z < -20 || Z > 120 || Yaw < 0 || Yaw >= 360) return Reject();
        Record.Position = FVector(X,Y,Z); Record.Yaw = static_cast<float>(Yaw);
        for (const auto& Prior : Constructions)
            if (Prior.Realm == Record.Realm && FVector::DistSquared2D(Prior.Position,Record.Position) < FMath::Square(320.0)) return Reject();
        Constructions.Add(Record);
        ++Counts[Record.Kind];
    }
    if (Counts != Candidate.builtCounts) return Reject();
    HK::KnowledgeSystem Validated(WorldSeed);
    if (!Validated.ImportState(Candidate).ok) return Reject();
    auto* Adventure = GetOwner()->FindComponentByClass<UHALVETHAdventureComponent>();
    if (!Adventure || !Adventure->RestoreConsumables(AdventureItems[0],AdventureItems[1],AdventureItems[2])) return Reject();
    Runtime->Knowledge = Validated;
    Runtime->Constructions = MoveTemp(Constructions);
    for (int32 I = 0; I < 3; ++I) Runtime->LastAdventureItems[I] = AdventureItems[I];
    bDirty = bPreserveInvalidSave = false;
    RefreshBuiltActors();
    Feedback(TEXT("Knowledge, workshop, inventory and constructions restored for this world seed."));
    return true;
}
