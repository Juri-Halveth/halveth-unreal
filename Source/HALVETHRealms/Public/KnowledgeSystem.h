#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <string_view>

namespace halveth::knowledge
{
    inline constexpr std::size_t BookCount = 6, MaxPages = 3, ItemCount = 10, SkillCount = 3;
    inline constexpr std::size_t RecipeCount = 3, EnchantmentCount = 2, ModuleCount = 3, QuestCount = 3;
    inline constexpr std::size_t GatherNodeCount = 16;
    inline constexpr int MaxBuilds = 32, MaxInventory = 999, MaxXP = 10000;
    inline constexpr std::uint32_t MaxPageMilliseconds = 600000;
    enum class Skill : std::uint8_t { Alchemy, Enchantment, Architecture, Count };
    enum class ItemId : std::uint8_t {
        Glassleaf, LumenDew, MoonSalt, PrismDust, EmberMoss, Timber, Stone,
        LumenDraught, WardInk, BindingResin, Count
    };
    struct Result { bool ok = false; std::string_view message; int xp = 0; bool discovered = false; };
    struct Page {
        std::string_view text, prompt;
        std::array<std::string_view, 3> answers{};
        std::size_t correct = 0;
    };
    struct Book {
        std::string_view id, title, author, kind;
        Skill topic = Skill::Alchemy;
        std::array<Page, MaxPages> pages{};
        std::size_t pageCount = 0;
    };
    struct PageRef { std::size_t document = 0, page = 0; };
    struct Cost { ItemId item = ItemId::Count; int quantity = 0; };
    struct Recipe {
        std::string_view id, name;
        ItemId output = ItemId::Count;
        int quantity = 1;
        Skill skill = Skill::Alchemy;
        std::array<PageRef, 2> requirements{};
        std::size_t requirementCount = 0;
        std::array<Cost, 3> costs{};
        std::size_t costCount = 0;
        int practiceXP = 0;
    };
    struct Enchantment {
        std::string_view id, name;
        std::array<PageRef, 2> requirements{};
        std::size_t requirementCount = 0;
        std::array<Cost, 3> costs{};
        std::size_t costCount = 0;
        int practiceXP = 0, maximumRank = 3;
    };
    struct BuildModule {
        std::string_view id, name;
        std::array<PageRef, 2> requirements{};
        std::size_t requirementCount = 0;
        std::array<Cost, 3> costs{};
        std::size_t costCount = 0;
        int practiceXP = 0;
    };
    struct QuestChoice {
        std::string_view name, outcome;
        std::array<Cost, 3> rewards{};
        std::size_t rewardCount = 0;
        int kindness = 0, balance = 0, insight = 0;
    };
    struct Quest {
        std::string_view id, title, summary;
        std::array<QuestChoice, 3> choices{};
        int prerequisiteQuest = -1;
        bool requiresGround = false;
    };
    struct PageProgress {
        std::uint32_t activeMilliseconds = 0;
        std::uint16_t attempts = 0;
        bool recalled = false;
        constexpr bool operator==(const PageProgress&) const = default;
    };
    struct State {
        // The gathering mask records first visits; renewable resource nodes are never locked by it.
        std::uint32_t version = 1, worldSeed = 127, gatheredMask = 0;
        std::array<int, ItemCount> inventory{};
        std::array<int, SkillCount> xp{};
        std::array<std::array<PageProgress, MaxPages>, BookCount> reading{};
        std::array<bool, RecipeCount> discoveredRecipes{};
        std::array<int, RecipeCount> craftedCounts{};
        std::array<int, EnchantmentCount> enchantmentRanks{};
        std::array<int, ModuleCount> builtCounts{};
        std::array<int, QuestCount> questChoices{-1, -1, -1};
        int kindness = 0, balance = 0, insight = 0;
        constexpr bool operator==(const State&) const = default;
    };

    inline constexpr std::array<Book, BookCount> BookCatalog{{
        {"glassleaf-primer", "The Glassleaf Primer", "Iria of the Quiet Kiln", "Book", Skill::Alchemy, {{
            {"When the blue moon lowers over the Confluence, Glassleaf turns clear enough to show its veins of silver. Iria gathered two leaves and folded them around one bead of Lumen Dew. The first Lumen Draught glowed like a hearth seen through rain, carrying the garden's warmth into a traveler's weary limbs.",
             "What does one Lumen Draught require?", {"Two Glassleaf and one Lumen Dew", "One Stone and one Timber", "Three Moon Salt"}, 0},
            {"The old gatekeepers wrote their wards on slate. One Moon Salt held the line steady, while one Prism Dust taught it to catch the stars. Together they became Ward Ink. Salt gives the mark its boundary; dust gives it a voice that light can answer. Without both, the gate's inscriptions fade before dawn.",
             "Which pair makes Ward Ink?", {"Timber and Ember Moss", "Moon Salt and Prism Dust", "Glassleaf and Stone"}, 1},
            {"Binding Resin was born during the red storm, when the council's shelter split along its ribs. Iria joined one Timber, one Glassleaf and one Ember Moss into a warm amber thread. The moss lent its patient ember, the leaf its silver veins, and the wood its grain. By morning the broken frame stood whole.",
             "Which third ingredient completes Timber and Glassleaf for Binding Resin?", {"Stone", "Moon Salt", "Ember Moss"}, 2}
        }}, 3},
        {"ink-between-stars", "Ink Between Stars", "Sera, Keeper of Small Lights", "Book", Skill::Enchantment, {{
            {"Sera found a lantern that shone only when nobody carried it. Its star had no home inside the glass. She drew a closed ring of Ward Ink around the wick, and the wandering light settled into the circle. The first lesson of Lantern Weave is shelter: a clear boundary lets a small radiance remain.",
             "What gave the wandering lantern-light a place to remain?", {"An open crack in the glass", "A closed ring of Ward Ink", "A pile of cold ash"}, 1},
            {"To strengthen Lantern Weave, Sera braided one Ward Ink with one Prism Dust. The first knot remembered evening, the second midnight, and the third dawn. Each knot gives its bearer room for ten more measures of mana. Travelers call those three knots the Nightkeeper's Promise: a greater inner reservoir for the road ahead.",
             "Which pair strengthens a knot of Lantern Weave?", {"Timber and Moon Salt", "Glassleaf and Stone", "Ward Ink and Prism Dust"}, 2},
            {"Quiet Step began with a messenger crossing the sleeping crystal beds. One Binding Resin and one Lumen Draught taught her magic to move with less waste. Each of its three ranks reduces a spell's mana cost by five parts in a hundred. The crystals still woke at her final footfall, until a traveler's pencil found the missing rest in the pattern.",
             "Which resources does one Quiet Step rank consume?", {"Binding Resin and Lumen Draught", "Moon Salt and Timber", "Two Stone"}, 0}
        }}, 3},
        {"three-supports", "A House Needs Three Supports", "Rachel's Workshop Collective", "Book", Skill::Architecture, {{
            {"Rachel's first workbench tilted toward the tide until four Stone were set beneath its corners. Each corner carried its share of the weight, while the earth between them remained open to roots and rain. From that patient square grew the Ground foundations on which the Confluence raised its camps.",
             "What is the cost of one Ground module?", {"One Prism Dust", "Four Stone", "Three Lumen Dew"}, 1},
            {"Once a Ground workbench anchors the settlement, its people can raise a Camp wherever the paths need shelter. Three Timber form its ribs and one Binding Resin binds the crossings. Rachel left the eastern side open to morning warmth, and planted no post where a Glassleaf root already held the soil.",
             "What must exist before a Camp can be built?", {"A completed Ground module", "A sealed crystal lantern", "A boat on the outer tide"}, 0},
            {"A settlement with a Ground workbench can raise Beacons along its distant paths. Two Stone carry each bowl; one Ward Ink inscribes its rim; one Prism Dust wakes its light. The glow points inward toward shelter, never outward like a challenge. On storm nights the lost follow the gentle rim back to the Confluence.",
             "What wakes the Beacon's inscribed bowl with light?", {"Three Timber alone", "A sealed layer of cold mud", "Prism Dust meeting the Ward Ink"}, 2}
        }}, 3},
        {"moonwater-scroll", "A Scroll of Moonwater Accounts", "Unknown Garden Apprentice", "Scroll", Skill::Alchemy, {{
            {"Before sunrise, the underside of Glassleaf gathers Lumen Dew. The blue moon's reflection stays in each bead long after the sky turns gold. When a leaf releases its dew, the glow returns through the roots to the moonwater channels. That is why the oldest gardens shine beneath the ground as well as above it.",
             "Where does a Glassleaf send the glow after releasing its dew?", {"Into the council's locked archive", "Through its roots to the moonwater channels", "Into the smoke above a kiln"}, 1},
            {"Ember Moss grows where a fallen lantern once warmed the soil. It holds a low crimson glow through winter and releases it slowly when the channels run cold. The apprentices call it the garden's banked hearth. Its warmth gives Binding Resin the patience to bend with living wood.",
             "Which plant is called the garden's banked hearth?", {"Ember Moss", "Glassleaf", "The white reeds of the outer tide"}, 0}, {}
        }}, 2},
        {"quiet-scribble", "A Margin Full of Quiet Signals", "A Traveler's Pencil", "Scribble", Skill::Enchantment, {{
            {"In the margin: Sera, your last curl answers the crystal too quickly. Leave a breath between the second mark and the closing stroke. I tried it beside the blue beds tonight. My boots crossed the stones, and the sleeping crystals kept their song. The silence belongs inside the pattern. - a hurried pencil, rain at the edge",
             "What completes the Quiet Step pattern in this margin note?", {"A louder final stroke", "A second pair of boots", "A pause before the closing stroke"}, 2},
            {"Another note, drawn around a moth: Turn the Beacon's bright rim toward the path, not into a traveler's eyes. A moth follows a soft edge better than a blinding center. We want our light to say that a place remains beside the fire. A welcome can be seen from far away.",
             "How is the beacon's signal described here?", {"An invitation to return", "A challenge to the outer tide", "A warning to keep away"}, 0}, {}
        }}, 2},
        {"council-journal", "Minutes of an Unfinished Garden", "The Confluence Council", "Journal", Skill::Architecture, {{
            {"Scarlet asked that the dew be shared before the common lanterns dimmed. Lucinet wanted the channels measured, so the next dry season would find the gardeners prepared. Rachel set aside a small reserve and listened to both. The council wrote every proposal into its minutes, then placed the decision in the traveler's hands.",
             "Who proposed measuring the moonwater channels?", {"Scarlet", "Lucinet", "The gate's silent lantern"}, 1},
            {"After the archive was settled, the council returned to Rachel's first foundation. A common camp would welcome the tired; a repair reserve would care for weathered paths; a line of beacons would guide the explorers home. Rachel laid three small stones on the bench, one for each future, and waited for the traveler to choose.",
             "What would a line of beacons offer the explorers?", {"A closed archive", "A new moonwater source", "A visible route home"}, 2}, {}
        }}, 2}
    }};

    inline constexpr std::array<Recipe, RecipeCount> RecipeCatalog{{
        {"lumen-draught", "Lumen Draught", ItemId::LumenDraught, 1, Skill::Alchemy, {{{0,0},{}}}, 1,
         {{{ItemId::Glassleaf,2},{ItemId::LumenDew,1},{}}}, 2, 12},
        {"ward-ink", "Ward Ink", ItemId::WardInk, 1, Skill::Alchemy, {{{0,1},{}}}, 1,
         {{{ItemId::MoonSalt,1},{ItemId::PrismDust,1},{}}}, 2, 14},
        {"binding-resin", "Binding Resin", ItemId::BindingResin, 1, Skill::Alchemy, {{{0,2},{}}}, 1,
         {{{ItemId::Timber,1},{ItemId::Glassleaf,1},{ItemId::EmberMoss,1}}}, 3, 16}
    }};
    inline constexpr std::array<Enchantment, EnchantmentCount> EnchantmentCatalog{{
        {"lantern-weave", "Lantern Weave", {{{1,0},{1,1}}}, 2,
         {{{ItemId::WardInk,1},{ItemId::PrismDust,1},{}}}, 2, 18, 3},
        {"quiet-step", "Quiet Step", {{{1,2},{4,0}}}, 2,
         {{{ItemId::BindingResin,1},{ItemId::LumenDraught,1},{}}}, 2, 18, 3}
    }};
    inline constexpr std::array<BuildModule, ModuleCount> ModuleCatalog{{
        {"ground", "Ground / Workbench", {{{2,0},{}}}, 1, {{{ItemId::Stone,4},{},{}}}, 1, 10},
        {"camp", "Camp", {{{2,1},{}}}, 1, {{{ItemId::Timber,3},{ItemId::BindingResin,1},{}}}, 2, 15},
        {"beacon", "Beacon", {{{2,2},{4,1}}}, 2, {{{ItemId::Stone,2},{ItemId::WardInk,1},{ItemId::PrismDust,1}}}, 3, 18}
    }};
    inline constexpr std::array<Quest, QuestCount> QuestCatalog{{
        {"thirsting-lanterns", "The Thirsting Lanterns", "The council must allocate a small store of garden resources.", {{
            {"Share the dew", "The common garden receives water; kindness rises and you receive three Lumen Dew.", {{{ItemId::LumenDew,3},{},{}}}, 1, 2, 0, 0},
            {"Keep a reserve", "A reserve is recorded; balance rises and you receive two Moon Salt.", {{{ItemId::MoonSalt,2},{},{}}}, 1, 0, 2, 0},
            {"Measure the channels", "The channels gain a measured plan; insight rises and you receive two Prism Dust.", {{{ItemId::PrismDust,2},{},{}}}, 1, 0, 0, 2}
        }}, -1, false},
        {"open-archive", "The Open Archive", "With the lantern decision recorded, choose how to care for the archive.", {{
            {"Offer public copies", "Copies circulate through the garden; kindness rises and a vial of Ward Ink is granted.", {{{ItemId::WardInk,1},{},{}}}, 1, 2, 0, 0},
            {"Protect fragile originals", "The originals gain a careful enclosure; balance rises and Binding Resin is granted.", {{{ItemId::BindingResin,1},{},{}}}, 1, 0, 2, 0},
            {"Share annotated records", "Readers can inspect the margins; insight rises and three Prism Dust are granted.", {{{ItemId::PrismDust,3},{},{}}}, 1, 0, 0, 2}
        }}, 0, false},
        {"place-to-return", "A Place to Return", "After the archive decision and a built Ground module, choose the settlement's next emphasis.", {{
            {"Support the common camp", "The council supports a shared resting place; kindness rises and a Lumen Draught is granted.", {{{ItemId::LumenDraught,1},{},{}}}, 1, 2, 0, 0},
            {"Reserve repair supplies", "A repair reserve is recorded; balance rises and four Timber are granted.", {{{ItemId::Timber,4},{},{}}}, 1, 0, 2, 0},
            {"Mark the explorers' route", "A path for returning explorers is recorded; insight rises and four Prism Dust are granted.", {{{ItemId::PrismDust,4},{},{}}}, 1, 0, 0, 2}
        }}, 1, true}
    }};

    inline constexpr const auto& Books() { return BookCatalog; }
    inline constexpr const auto& Recipes() { return RecipeCatalog; }
    inline constexpr const auto& Enchantments() { return EnchantmentCatalog; }
    inline constexpr const auto& Modules() { return ModuleCatalog; }
    inline constexpr const auto& Quests() { return QuestCatalog; }
    inline constexpr std::string_view ItemName(ItemId Item) {
        constexpr std::array<std::string_view, ItemCount> Names{"Glassleaf", "Lumen Dew", "Moon Salt", "Prism Dust", "Ember Moss", "Timber", "Stone", "Lumen Draught", "Ward Ink", "Binding Resin"};
        return static_cast<std::size_t>(Item) < ItemCount ? Names[static_cast<std::size_t>(Item)] : "Unknown item";
    }
    inline constexpr std::string_view SkillName(Skill Value) {
        constexpr std::array<std::string_view, SkillCount> Names{"Alchemy", "Enchantment", "Architecture"};
        return static_cast<std::size_t>(Value) < SkillCount ? Names[static_cast<std::size_t>(Value)] : "Unknown skill";
    }

    class KnowledgeSystem
    {
    public:
        explicit KnowledgeSystem(std::uint32_t Seed = 127);
        State ExportState() const { return Data; }
        Result ImportState(const State& Candidate);
        Result RecordActiveReading(std::size_t Document, std::size_t PageIndex, double Seconds);
        Result Recall(std::size_t Document, std::size_t PageIndex, std::size_t Choice);
        Result Craft(std::size_t RecipeIndex);
        Result Enchant(std::size_t EnchantmentIndex);
        Result CanBuild(std::size_t ModuleIndex) const;
        Result Build(std::size_t ModuleIndex);
        Result ChooseQuest(std::size_t QuestIndex, std::size_t Choice);
        Result Gather(std::size_t NodeIndex);
        Result GrantItem(ItemId Item, int Quantity);
        Result ConsumeItem(ItemId Item, int Quantity);
        bool IsRecipeUnlocked(std::size_t Index) const;
        bool IsEnchantmentUnlocked(std::size_t Index) const;
        bool IsModuleUnlocked(std::size_t Index) const;
        int GetItemCount(ItemId Item) const;
        int GetXP(Skill Value) const;
        int GetLevel(Skill Value) const;
        PageProgress GetPageProgress(std::size_t Document, std::size_t PageIndex) const;
        std::uint32_t GetWorldSeed() const { return Data.worldSeed; }
        std::uint32_t LandmarkSeed(std::size_t Landmark) const;
    private:
        State Data{};
        static bool ValidPage(std::size_t Document, std::size_t PageIndex);
        static bool RequirementsMet(const State& Value, const std::array<PageRef,2>& Requirements, std::size_t Count);
        static std::array<int,SkillCount> ExpectedXP(const State& Value);
        bool DiscoverRecipes();
        int AwardXP(Skill Value, int Amount);
        bool CanPay(const std::array<Cost,3>& Costs, std::size_t Count) const;
        void Pay(const std::array<Cost,3>& Costs, std::size_t Count);
    };

    inline KnowledgeSystem::KnowledgeSystem(std::uint32_t Seed)
    {
        Data.worldSeed = Seed;
        Data.inventory = {4, 4, 3, 4, 3, 14, 20, 0, 0, 0};
    }
    inline bool KnowledgeSystem::ValidPage(std::size_t Document, std::size_t PageIndex)
    {
        return Document < BookCount && PageIndex < Books()[Document].pageCount;
    }
    inline PageProgress KnowledgeSystem::GetPageProgress(std::size_t Document, std::size_t PageIndex) const
    {
        return ValidPage(Document, PageIndex) ? Data.reading[Document][PageIndex] : PageProgress{};
    }
    inline int KnowledgeSystem::GetItemCount(ItemId Item) const
    {
        const auto Index = static_cast<std::size_t>(Item);
        return Index < ItemCount ? Data.inventory[Index] : 0;
    }
    inline int KnowledgeSystem::GetXP(Skill Value) const
    {
        const auto Index = static_cast<std::size_t>(Value);
        return Index < SkillCount ? Data.xp[Index] : 0;
    }
    inline int KnowledgeSystem::GetLevel(Skill Value) const
    {
        return static_cast<std::size_t>(Value) < SkillCount ? std::min(10, 1 + GetXP(Value) / 100) : 0;
    }
    inline bool KnowledgeSystem::RequirementsMet(const State& Value, const std::array<PageRef,2>& Requirements, std::size_t Count)
    {
        if (Count == 0 || Count > Requirements.size()) return false;
        for (std::size_t Index = 0; Index < Count; ++Index)
        {
            const auto Reference = Requirements[Index];
            if (!ValidPage(Reference.document, Reference.page) || !Value.reading[Reference.document][Reference.page].recalled) return false;
        }
        return true;
    }
    inline bool KnowledgeSystem::IsRecipeUnlocked(std::size_t Index) const
    {
        return Index < RecipeCount && RequirementsMet(Data, Recipes()[Index].requirements, Recipes()[Index].requirementCount);
    }
    inline bool KnowledgeSystem::IsEnchantmentUnlocked(std::size_t Index) const
    {
        return Index < EnchantmentCount && RequirementsMet(Data, Enchantments()[Index].requirements, Enchantments()[Index].requirementCount);
    }
    inline bool KnowledgeSystem::IsModuleUnlocked(std::size_t Index) const
    {
        return Index < ModuleCount && RequirementsMet(Data, Modules()[Index].requirements, Modules()[Index].requirementCount);
    }
    inline bool KnowledgeSystem::DiscoverRecipes()
    {
        bool Found = false;
        for (std::size_t Index = 0; Index < RecipeCount; ++Index)
            if (IsRecipeUnlocked(Index) && !Data.discoveredRecipes[Index])
            { Data.discoveredRecipes[Index] = true; Found = true; }
        return Found;
    }
    inline int KnowledgeSystem::AwardXP(Skill Value, int Amount)
    {
        const auto Index = static_cast<std::size_t>(Value);
        if (Index >= SkillCount || Amount <= 0) return 0;
        const int Award = std::min(Amount, MaxXP - Data.xp[Index]);
        Data.xp[Index] += Award;
        return Award;
    }
    inline Result KnowledgeSystem::RecordActiveReading(std::size_t Document, std::size_t PageIndex, double Seconds)
    {
        if (!ValidPage(Document, PageIndex) || !std::isfinite(Seconds) || Seconds <= 0)
            return {false, "No active page time recorded."};
        const auto Milliseconds = static_cast<std::uint32_t>(std::lround(std::min(2.0, Seconds) * 1000));
        auto& Progress = Data.reading[Document][PageIndex];
        Progress.activeMilliseconds = std::min(MaxPageMilliseconds, Progress.activeMilliseconds + Milliseconds);
        return {true, "Active page time recorded; recall remains a separate choice."};
    }
    inline Result KnowledgeSystem::Recall(std::size_t Document, std::size_t PageIndex, std::size_t Choice)
    {
        if (!ValidPage(Document, PageIndex) || Choice >= 3) return {false, "Choose a valid page and answer."};
        auto& Progress = Data.reading[Document][PageIndex];
        if (Progress.recalled) return {true, "This page is already recalled."};
        if (Progress.attempts < 255) ++Progress.attempts;
        if (Choice != Books()[Document].pages[PageIndex].correct)
            return {false, "That answer does not match the page. Read or try another answer."};
        Progress.recalled = true;
        const int Award = AwardXP(Books()[Document].topic, 8);
        return {true, "Page recalled.", Award, DiscoverRecipes()};
    }
    inline bool KnowledgeSystem::CanPay(const std::array<Cost,3>& Costs, std::size_t Count) const
    {
        if (Count == 0 || Count > Costs.size()) return false;
        std::array<int,ItemCount> Required{};
        for (std::size_t Index = 0; Index < Count; ++Index)
        {
            const Cost Value = Costs[Index];
            const auto ItemIndex = static_cast<std::size_t>(Value.item);
            if (ItemIndex >= ItemCount || Value.quantity <= 0 || Value.quantity > MaxInventory - Required[ItemIndex]) return false;
            Required[ItemIndex] += Value.quantity;
        }
        for (std::size_t Index = 0; Index < ItemCount; ++Index)
            if (Data.inventory[Index] < Required[Index]) return false;
        return true;
    }
    inline void KnowledgeSystem::Pay(const std::array<Cost,3>& Costs, std::size_t Count)
    {
        for (std::size_t Index = 0; Index < Count; ++Index)
            Data.inventory[static_cast<std::size_t>(Costs[Index].item)] -= Costs[Index].quantity;
    }
    inline Result KnowledgeSystem::GrantItem(ItemId Item, int Quantity)
    {
        const auto Index = static_cast<std::size_t>(Item);
        if (Index >= ItemCount || Quantity <= 0 || Quantity > MaxInventory - Data.inventory[Index])
            return {false, "The item does not fit in the pack."};
        Data.inventory[Index] += Quantity;
        return {true, "Item received."};
    }
    inline Result KnowledgeSystem::ConsumeItem(ItemId Item, int Quantity)
    {
        const auto Index = static_cast<std::size_t>(Item);
        if (Index >= ItemCount || Quantity <= 0 || Quantity > Data.inventory[Index])
            return {false, "The pack does not contain that quantity."};
        Data.inventory[Index] -= Quantity;
        return {true, "Item consumed."};
    }
    inline Result KnowledgeSystem::Craft(std::size_t RecipeIndex)
    {
        if (!IsRecipeUnlocked(RecipeIndex)) return {false, "Recall the recipe's source page first."};
        const auto& Recipe = Recipes()[RecipeIndex];
        if (Data.craftedCounts[RecipeIndex] >= 1000) return {false, "This recipe's practice record is full."};
        if (!CanPay(Recipe.costs, Recipe.costCount)) return {false, "The recipe needs more materials."};
        const auto Output = static_cast<std::size_t>(Recipe.output);
        if (Recipe.quantity > MaxInventory - Data.inventory[Output]) return {false, "The crafted item would not fit."};
        Pay(Recipe.costs, Recipe.costCount);
        Data.inventory[Output] += Recipe.quantity;
        ++Data.craftedCounts[RecipeIndex];
        return {true, "Craft complete.", AwardXP(Recipe.skill, Recipe.practiceXP)};
    }
    inline Result KnowledgeSystem::Enchant(std::size_t EnchantmentIndex)
    {
        if (!IsEnchantmentUnlocked(EnchantmentIndex)) return {false, "Recall both parts of the enchantment first."};
        const auto& Pattern = Enchantments()[EnchantmentIndex];
        if (Data.enchantmentRanks[EnchantmentIndex] >= Pattern.maximumRank) return {false, "The enchantment is already complete."};
        if (!CanPay(Pattern.costs, Pattern.costCount)) return {false, "The enchantment needs more materials."};
        Pay(Pattern.costs, Pattern.costCount);
        ++Data.enchantmentRanks[EnchantmentIndex];
        return {true, "Enchantment strengthened.", AwardXP(Skill::Enchantment, Pattern.practiceXP)};
    }
    inline Result KnowledgeSystem::CanBuild(std::size_t ModuleIndex) const
    {
        if (!IsModuleUnlocked(ModuleIndex)) return {false, "Recall the building's source pages first."};
        int Total = 0;
        for (int Count : Data.builtCounts) Total += Count;
        if (Total >= MaxBuilds) return {false, "This settlement has reached its module limit."};
        if (ModuleIndex != 0 && Data.builtCounts[0] == 0) return {false, "Build a Ground foundation first."};
        const auto& Module = Modules()[ModuleIndex];
        if (!CanPay(Module.costs, Module.costCount)) return {false, "The building needs more materials."};
        return {true, "The materials and knowledge are ready."};
    }
    inline Result KnowledgeSystem::Build(std::size_t ModuleIndex)
    {
        const Result Ready = CanBuild(ModuleIndex);
        if (!Ready.ok) return Ready;
        const auto& Module = Modules()[ModuleIndex];
        Pay(Module.costs, Module.costCount);
        ++Data.builtCounts[ModuleIndex];
        return {true, "Building materials committed.", AwardXP(Skill::Architecture, Module.practiceXP)};
    }
    inline Result KnowledgeSystem::ChooseQuest(std::size_t QuestIndex, std::size_t Choice)
    {
        if (QuestIndex >= QuestCount || Choice >= 3) return {false, "Choose a valid council decision."};
        if (Data.questChoices[QuestIndex] != -1) return {false, "This council decision is already recorded."};
        const auto& Quest = Quests()[QuestIndex];
        if (Quest.prerequisiteQuest >= 0 && Data.questChoices[static_cast<std::size_t>(Quest.prerequisiteQuest)] == -1)
            return {false, "Resolve the earlier council decision first."};
        if (Quest.requiresGround && Data.builtCounts[0] == 0) return {false, "The council needs a built Ground foundation."};
        const auto& Decision = Quest.choices[Choice];
        auto Inventory = Data.inventory;
        for (std::size_t Index = 0; Index < Decision.rewardCount; ++Index)
        {
            const auto Reward = Decision.rewards[Index];
            const auto Item = static_cast<std::size_t>(Reward.item);
            if (Reward.quantity > MaxInventory - Inventory[Item]) return {false, "Make room for the council's gift first."};
            Inventory[Item] += Reward.quantity;
        }
        Data.inventory = Inventory;
        Data.questChoices[QuestIndex] = static_cast<int>(Choice);
        Data.kindness += Decision.kindness;
        Data.balance += Decision.balance;
        Data.insight += Decision.insight;
        return {true, Decision.outcome, AwardXP(Skill::Architecture, 10)};
    }
    inline std::uint32_t KnowledgeSystem::LandmarkSeed(std::size_t Landmark) const
    {
        std::uint32_t Value = Data.worldSeed ^ (0x9e3779b9u * (static_cast<std::uint32_t>(Landmark) + 1u));
        Value ^= Value >> 16; Value *= 0x7feb352du; Value ^= Value >> 15;
        Value *= 0x846ca68bu; Value ^= Value >> 16;
        return Value;
    }
    inline Result KnowledgeSystem::Gather(std::size_t NodeIndex)
    {
        if (NodeIndex >= GatherNodeCount) return {false, "That gathering node is outside the garden."};
        const std::uint32_t Flag = std::uint32_t{1} << NodeIndex;
        const auto Seed = LandmarkSeed(NodeIndex);
        // A seed rotates a complete material cycle, guaranteeing access to every raw material.
        // The native caller invokes this only for an explicit nearby gathering action, never a timer.
        const auto Item = static_cast<ItemId>((NodeIndex + Data.worldSeed % 7u) % 7u);
        const int Quantity = 1 + static_cast<int>((Seed >> 8) % 3u);
        const Result Granted = GrantItem(Item, Quantity);
        if (!Granted.ok) return Granted;
        Data.gatheredMask |= Flag;
        return {true, "Garden resource gathered. This living node can be tended again."};
    }
    inline std::array<int,SkillCount> KnowledgeSystem::ExpectedXP(const State& Value)
    {
        std::array<int,SkillCount> XP{};
        for (std::size_t Document = 0; Document < BookCount; ++Document)
            for (std::size_t PageIndex = 0; PageIndex < Books()[Document].pageCount; ++PageIndex)
                if (Value.reading[Document][PageIndex].recalled) XP[static_cast<std::size_t>(Books()[Document].topic)] += 8;
        for (std::size_t Index = 0; Index < RecipeCount; ++Index)
            XP[static_cast<std::size_t>(Recipes()[Index].skill)] += Value.craftedCounts[Index] * Recipes()[Index].practiceXP;
        for (std::size_t Index = 0; Index < EnchantmentCount; ++Index)
            XP[static_cast<std::size_t>(Skill::Enchantment)] += Value.enchantmentRanks[Index] * Enchantments()[Index].practiceXP;
        for (std::size_t Index = 0; Index < ModuleCount; ++Index)
            XP[static_cast<std::size_t>(Skill::Architecture)] += Value.builtCounts[Index] * Modules()[Index].practiceXP;
        for (int Choice : Value.questChoices) if (Choice != -1) XP[static_cast<std::size_t>(Skill::Architecture)] += 10;
        for (int& Points : XP) Points = std::min(MaxXP, Points);
        return XP;
    }
    inline Result KnowledgeSystem::ImportState(const State& Candidate)
    {
        if (Candidate.version != 1 || (Candidate.gatheredMask >> GatherNodeCount) != 0)
            return {false, "Unsupported knowledge state or gathering mask."};
        for (int Count : Candidate.inventory) if (Count < 0 || Count > MaxInventory) return {false, "Invalid inventory quantity."};
        for (std::size_t Document = 0; Document < BookCount; ++Document)
            for (std::size_t PageIndex = 0; PageIndex < MaxPages; ++PageIndex)
            {
                const auto Progress = Candidate.reading[Document][PageIndex];
                if ((!ValidPage(Document, PageIndex) && Progress != PageProgress{}) ||
                    Progress.activeMilliseconds > MaxPageMilliseconds || Progress.attempts > 255 ||
                    (Progress.recalled && Progress.attempts == 0)) return {false, "Invalid page progress."};
            }
        for (std::size_t Index = 0; Index < RecipeCount; ++Index)
        {
            const bool Known = RequirementsMet(Candidate, Recipes()[Index].requirements, Recipes()[Index].requirementCount);
            if (Candidate.discoveredRecipes[Index] != Known || Candidate.craftedCounts[Index] < 0 || Candidate.craftedCounts[Index] > 1000 ||
                (Candidate.craftedCounts[Index] > 0 && !Known)) return {false, "Recipe discovery or practice state is inconsistent."};
        }
        for (std::size_t Index = 0; Index < EnchantmentCount; ++Index)
            if (Candidate.enchantmentRanks[Index] < 0 || Candidate.enchantmentRanks[Index] > Enchantments()[Index].maximumRank ||
                (Candidate.enchantmentRanks[Index] > 0 && !RequirementsMet(Candidate, Enchantments()[Index].requirements, Enchantments()[Index].requirementCount)))
                return {false, "Invalid enchantment progress."};
        int Built = 0;
        for (std::size_t Index = 0; Index < ModuleCount; ++Index)
        {
            if (Candidate.builtCounts[Index] < 0 || Candidate.builtCounts[Index] > MaxBuilds ||
                (Candidate.builtCounts[Index] > 0 && !RequirementsMet(Candidate, Modules()[Index].requirements, Modules()[Index].requirementCount)))
                return {false, "Invalid building progress."};
            Built += Candidate.builtCounts[Index];
        }
        if (Built > MaxBuilds || (Candidate.builtCounts[0] == 0 && Built > 0)) return {false, "Building capacity or foundation state is invalid."};
        int Kindness = 0, Balance = 0, Insight = 0;
        for (std::size_t Index = 0; Index < QuestCount; ++Index)
        {
            const int Choice = Candidate.questChoices[Index];
            if (Choice < -1 || Choice > 2) return {false, "Invalid council choice."};
            if (Choice == -1) continue;
            const auto& Quest = Quests()[Index];
            if ((Quest.prerequisiteQuest >= 0 && Candidate.questChoices[static_cast<std::size_t>(Quest.prerequisiteQuest)] == -1) ||
                (Quest.requiresGround && Candidate.builtCounts[0] == 0)) return {false, "Council prerequisites are missing."};
            const auto& Decision = Quest.choices[static_cast<std::size_t>(Choice)];
            Kindness += Decision.kindness; Balance += Decision.balance; Insight += Decision.insight;
        }
        if (Candidate.kindness != Kindness || Candidate.balance != Balance || Candidate.insight != Insight)
            return {false, "Council consequences do not match the recorded choices."};
        if (Candidate.xp != ExpectedXP(Candidate)) return {false, "Experience does not match recorded recall and practice."};
        Data = Candidate;
        return {true, "Knowledge state restored."};
    }
}
