#include "../Source/HALVETHRealms/Public/KnowledgeSystem.h"

#include <cstdlib>
#include <iostream>
#include <limits>
#include <set>
#include <string_view>

namespace k = halveth::knowledge;
namespace
{
    int Checks = 0;
    void Check(bool Condition, const char* Description)
    {
        ++Checks;
        if (!Condition)
        {
            std::cerr << "FAIL: " << Description << '\n';
            std::exit(1);
        }
    }
    void Learn(k::KnowledgeSystem& World, std::size_t Document, std::size_t Page)
    {
        Check(World.Recall(Document, Page, k::Books()[Document].pages[Page].correct).ok, "recall source page");
    }
    void LearnAll(k::KnowledgeSystem& World)
    {
        for (std::size_t Document = 0; Document < k::BookCount; ++Document)
            for (std::size_t Page = 0; Page < k::Books()[Document].pageCount; ++Page)
                Learn(World, Document, Page);
    }
    template<class Operation>
    void RejectUnchanged(k::KnowledgeSystem& World, Operation Action, const char* Description)
    {
        const auto Before = World.ExportState();
        Check(!Action().ok, Description);
        Check(World.ExportState() == Before, "rejected transaction preserves all state");
    }
    template<class Mutation>
    void RejectSave(k::KnowledgeSystem& World, Mutation Mutate, const char* Description)
    {
        auto Candidate = World.ExportState();
        Mutate(Candidate);
        RejectUnchanged(World, [&] { return World.ImportState(Candidate); }, Description);
    }
    void CatalogContracts()
    {
        std::set<std::string_view> IDs;
        std::size_t Pages = 0;
        for (const auto& Book : k::Books())
        {
            Check(IDs.insert(Book.id).second && !Book.id.empty(), "unique document id");
            Check(!Book.title.empty() && !Book.author.empty(), "document attribution present");
            Check(Book.pageCount >= 2 && Book.pageCount <= 3, "bounded multi-page document");
            Check(static_cast<std::size_t>(Book.topic) < k::SkillCount, "valid knowledge topic");
            Pages += Book.pageCount;
            for (std::size_t Page = 0; Page < Book.pageCount; ++Page)
            {
                const auto& Text = Book.pages[Page];
                Check(Text.text.size() > 100 && !Text.prompt.empty(), "original page has prose and a question");
                Check(Text.correct < Text.answers.size(), "answer index is in range");
                std::set<std::string_view> Answers;
                for (const auto Answer : Text.answers)
                    Check(!Answer.empty() && Answers.insert(Answer).second, "three distinct answer choices");
            }
        }
        Check(Pages == 15, "six documents contain fifteen pages");
        auto VerifyPattern = [&](const auto& Pattern)
        {
            Check(IDs.insert(Pattern.id).second, "unique crafting/building id");
            Check(Pattern.requirementCount > 0 && Pattern.requirementCount <= Pattern.requirements.size(), "source requirements bounded");
            for (std::size_t Index = 0; Index < Pattern.requirementCount; ++Index)
            {
                const auto Source = Pattern.requirements[Index];
                Check(Source.document < k::BookCount, "source document exists");
                Check(Source.page < k::Books()[Source.document].pageCount, "source page exists");
            }
            Check(Pattern.costCount > 0 && Pattern.costCount <= Pattern.costs.size(), "material requirements bounded");
            for (std::size_t Index = 0; Index < Pattern.costCount; ++Index)
            {
                const auto Cost = Pattern.costs[Index];
                Check(static_cast<std::size_t>(Cost.item) < k::ItemCount && Cost.quantity > 0 && Cost.quantity <= k::MaxInventory, "positive valid material cost");
            }
            Check(Pattern.practiceXP > 0, "material practice has an XP reward");
        };
        for (const auto& Pattern : k::Recipes()) VerifyPattern(Pattern);
        for (const auto& Pattern : k::Enchantments()) VerifyPattern(Pattern);
        for (const auto& Pattern : k::Modules()) VerifyPattern(Pattern);
        for (const auto& Quest : k::Quests())
        {
            Check(IDs.insert(Quest.id).second, "unique quest id");
            std::set<std::string_view> Outcomes;
            for (const auto& Choice : Quest.choices)
                Check(!Choice.name.empty() && Outcomes.insert(Choice.outcome).second, "each quest offers distinct named outcomes");
        }
    }
    void ReadingAndRecall()
    {
        k::KnowledgeSystem World;
        RejectUnchanged(World, [&] { return World.RecordActiveReading(0, 0, -1); }, "negative dwell rejected");
        RejectUnchanged(World, [&] { return World.RecordActiveReading(0, 0, 0); }, "zero dwell rejected");
        RejectUnchanged(World, [&] { return World.RecordActiveReading(0, 0, std::numeric_limits<double>::quiet_NaN()); }, "NaN dwell rejected");
        RejectUnchanged(World, [&] { return World.RecordActiveReading(0, 0, std::numeric_limits<double>::infinity()); }, "infinite dwell rejected");
        RejectUnchanged(World, [&] { return World.RecordActiveReading(k::BookCount, 0, 1); }, "missing book rejected");
        RejectUnchanged(World, [&] { return World.RecordActiveReading(3, 2, 1); }, "unused page rejected");
        Check(World.RecordActiveReading(0, 0, 1000000).ok, "large finite tick accepted with bounded credit");
        Check(World.GetPageProgress(0, 0).activeMilliseconds == 2000, "one stalled tick credits at most two seconds");
        for (int Index = 0; Index < 400; ++Index) World.RecordActiveReading(0, 0, 2);
        Check(World.GetPageProgress(0, 0).activeMilliseconds == k::MaxPageMilliseconds, "engagement counter has a total cap");
        Check(World.GetXP(k::Skill::Alchemy) == 0 && !World.IsRecipeUnlocked(0), "idle reading grants neither XP nor knowledge");

        k::KnowledgeSystem FastReader;
        RejectUnchanged(FastReader, [&] { return FastReader.Recall(0, 0, 3); }, "invalid answer index rejected");
        Check(!FastReader.Recall(0, 0, 1).ok, "incorrect answer not accepted");
        Check(FastReader.GetPageProgress(0, 0).attempts == 1 && FastReader.GetXP(k::Skill::Alchemy) == 0, "wrong answer records attempt without XP");
        const auto Correct = FastReader.Recall(0, 0, 0);
        Check(Correct.ok && Correct.xp == 8 && Correct.discovered, "correct answer grants one recall reward and recipe discovery");
        Check(FastReader.GetPageProgress(0, 0).activeMilliseconds == 0 && FastReader.IsRecipeUnlocked(0), "fast readers can demonstrate recall immediately");
        const auto Before = FastReader.ExportState();
        const auto Repeat = FastReader.Recall(0, 0, 0);
        Check(Repeat.ok && Repeat.xp == 0 && !Repeat.discovered && FastReader.ExportState() == Before, "repeated recall never farms XP or discoveries");
        for (int Index = 0; Index < 300; ++Index) FastReader.Recall(0, 1, 0);
        Check(FastReader.GetPageProgress(0, 1).attempts == 255, "failed attempt count remains bounded");
        Learn(FastReader, 0, 1);
        Check(FastReader.GetPageProgress(0, 1).recalled, "attempt cap does not lock out a later correct answer");
    }
    void CraftingAndEnchanting()
    {
        k::KnowledgeSystem World;
        RejectUnchanged(World, [&] { return World.Craft(0); }, "unknown recipe cannot consume materials");
        RejectUnchanged(World, [&] { return World.Craft(k::RecipeCount); }, "missing recipe rejected");
        Learn(World, 0, 0);
        const auto Craft = World.Craft(0);
        Check(Craft.ok && Craft.xp == 12, "alchemy consumes materials for practice XP");
        Check(World.GetItemCount(k::ItemId::Glassleaf) == 2 && World.GetItemCount(k::ItemId::LumenDew) == 3 && World.GetItemCount(k::ItemId::LumenDraught) == 1, "draught transaction uses exact recipe quantities");
        Check(World.ExportState().craftedCounts[0] == 1 && World.GetXP(k::Skill::Alchemy) == 20, "craft recorded with recall and practice XP");
        Check(World.Craft(0).ok, "second craft succeeds while resources remain");
        RejectUnchanged(World, [&] { return World.Craft(0); }, "insufficient materials leave state unchanged");
        Check(World.GrantItem(k::ItemId::Glassleaf, 2).ok, "refill ingredient for capacity test");
        Check(World.GrantItem(k::ItemId::LumenDraught, 997).ok, "fill output stack");
        RejectUnchanged(World, [&] { return World.Craft(0); }, "full output stack cannot consume ingredients");
        RejectUnchanged(World, [&] { return World.GrantItem(k::ItemId::Count, 1); }, "unknown item cannot be granted");
        RejectUnchanged(World, [&] { return World.GrantItem(k::ItemId::Stone, std::numeric_limits<int>::max()); }, "huge item grant is bounded");
        RejectUnchanged(World, [&] { return World.ConsumeItem(k::ItemId::Stone, -1); }, "negative consumption rejected");
        RejectUnchanged(World, [&] { return World.ConsumeItem(k::ItemId::Stone, 21); }, "excess consumption rejected");

        k::KnowledgeSystem Enchanter;
        Learn(Enchanter, 1, 0);
        Check(!Enchanter.IsEnchantmentUnlocked(0), "first part alone does not unlock enchantment");
        RejectUnchanged(Enchanter, [&] { return Enchanter.Enchant(0); }, "partial pattern cannot be enchanted");
        Learn(Enchanter, 1, 1);
        RejectUnchanged(Enchanter, [&] { return Enchanter.Enchant(0); }, "known pattern still requires materials");
        Check(Enchanter.GrantItem(k::ItemId::WardInk, 3).ok, "provide three enchanting materials");
        for (int Rank = 1; Rank <= 3; ++Rank)
        {
            Check(Enchanter.Enchant(0).ok, "enchantment rank succeeds");
            Check(Enchanter.ExportState().enchantmentRanks[0] == Rank, "enchantment rank advances once");
        }
        RejectUnchanged(Enchanter, [&] { return Enchanter.Enchant(0); }, "maximum enchantment rank cannot be charged again");
        Learn(Enchanter, 1, 2);
        Check(!Enchanter.IsEnchantmentUnlocked(1), "quiet step requires the separate marginal discovery");
        Learn(Enchanter, 4, 0);
        Check(Enchanter.IsEnchantmentUnlocked(1), "margin note completes quiet step pattern");
        Check(Enchanter.GrantItem(k::ItemId::BindingResin, 1).ok && Enchanter.GrantItem(k::ItemId::LumenDraught, 1).ok, "quiet step ingredients provided");
        Check(Enchanter.Enchant(1).ok && Enchanter.GetItemCount(k::ItemId::BindingResin) == 0 && Enchanter.GetItemCount(k::ItemId::LumenDraught) == 0, "quiet step consumes both ingredients");
    }
    void ConstructionAndCouncil()
    {
        k::KnowledgeSystem World;
        LearnAll(World);
        Check(World.GrantItem(k::ItemId::BindingResin, 1).ok && World.GrantItem(k::ItemId::WardInk, 1).ok, "provide camp and beacon resources");
        RejectUnchanged(World, [&] { return World.Build(1); }, "camp requires an actual foundation");
        RejectUnchanged(World, [&] { return World.Build(2); }, "beacon requires an actual foundation");
        const auto Before = World.ExportState();
        Check(World.CanBuild(0).ok && World.ExportState() == Before, "build preflight is read only");
        Check(World.Build(0).ok && World.GetItemCount(k::ItemId::Stone) == 16, "foundation consumes four stone");
        Check(World.Build(1).ok && World.GetItemCount(k::ItemId::Timber) == 11, "camp construction consumes timber and resin");
        Check(World.Build(2).ok && World.GetItemCount(k::ItemId::Stone) == 14, "beacon construction consumes its materials");
        Check(World.GrantItem(k::ItemId::Stone, 200).ok, "provide material for bounded settlement test");
        for (int Count = 3; Count < k::MaxBuilds; ++Count) Check(World.Build(0).ok, "each additional foundation paid for");
        RejectUnchanged(World, [&] { return World.Build(0); }, "settlement limit is enforced before charging");
        RejectUnchanged(World, [&] { return World.Build(k::ModuleCount); }, "missing module rejected");

        for (std::size_t Branch = 0; Branch < 3; ++Branch)
        {
            k::KnowledgeSystem Council;
            RejectUnchanged(Council, [&] { return Council.ChooseQuest(1, Branch); }, "archive waits for lantern decision");
            Check(Council.ChooseQuest(0, Branch).ok, "first council decision succeeds");
            RejectUnchanged(Council, [&] { return Council.ChooseQuest(0, Branch); }, "council reward cannot be reclaimed");
            Check(Council.ChooseQuest(1, Branch).ok, "archive decision follows lantern decision");
            RejectUnchanged(Council, [&] { return Council.ChooseQuest(2, Branch); }, "settlement decision waits for physical foundation");
            Learn(Council, 2, 0);
            Check(Council.Build(0).ok && Council.ChooseQuest(2, Branch).ok, "settlement decision succeeds after foundation");
            const auto State = Council.ExportState();
            Check(State.kindness == (Branch == 0 ? 6 : 0) && State.balance == (Branch == 1 ? 6 : 0) && State.insight == (Branch == 2 ? 6 : 0), "three branches have distinct persistent consequences");
            Check(Council.GetXP(k::Skill::Architecture) == 48, "council reward accounted once per completed quest");
            k::KnowledgeSystem Restored;
            Check(Restored.ImportState(State).ok && Restored.ExportState() == State, "each complete quest branch survives restore");
        }
        k::KnowledgeSystem FullPack;
        Check(FullPack.GrantItem(k::ItemId::LumenDew, 995).ok, "fill council reward stack");
        RejectUnchanged(FullPack, [&] { return FullPack.ChooseQuest(0, 0); }, "overflowing council reward cannot commit choice or consequences");
        RejectUnchanged(FullPack, [&] { return FullPack.ChooseQuest(0, 3); }, "invalid council answer rejected");
    }
    void DeterministicGathering()
    {
        k::KnowledgeSystem First(918273), Same(918273), Other(918274);
        bool Difference = false;
        for (std::size_t Index = 0; Index < k::GatherNodeCount; ++Index)
        {
            Check(First.LandmarkSeed(Index) == Same.LandmarkSeed(Index), "same world seed has stable landmark arrangement");
            Difference = Difference || First.LandmarkSeed(Index) != Other.LandmarkSeed(Index);
            Check(First.Gather(Index).ok && Same.Gather(Index).ok, "each finite garden node grants its resource");
            Check(First.ExportState() == Same.ExportState(), "deterministic gathering matches between worlds");
            const auto BeforeRepeat = First.ExportState();
            Check(First.Gather(Index).ok && Same.Gather(Index).ok, "an explicit repeat action can tend a renewable node");
            Check(First.ExportState().gatheredMask == BeforeRepeat.gatheredMask && First.ExportState().xp == BeforeRepeat.xp, "repeat gathering changes neither historical visit mask nor XP");
            Check(First.ExportState().inventory != BeforeRepeat.inventory, "repeat gathering actually replenishes material");
        }
        Check(Difference, "different seed changes landmark arrangement");
        Check(First.ExportState().gatheredMask == 65535 && First.GetXP(k::Skill::Alchemy) == 0, "historical gathered mask records all nodes without passive skill XP");
        RejectUnchanged(First, [&] { return First.Gather(k::GatherNodeCount); }, "outside garden node rejected");
        k::KnowledgeSystem Full;
        for (std::size_t Index = 0; Index < 7; ++Index)
        {
            const auto Item = static_cast<k::ItemId>(Index);
            Check(Full.GrantItem(Item, k::MaxInventory - Full.GetItemCount(Item)).ok, "fill raw material stacks");
        }
        RejectUnchanged(Full, [&] { return Full.Gather(0); }, "full pack leaves gathering claim available");
    }
    template<class Operation>
    void CompleteWithGarden(k::KnowledgeSystem& World, Operation Action)
    {
        for (int Round = 0; Round < 32; ++Round)
        {
            const auto Before = World.ExportState();
            if (Action().ok) return;
            Check(World.ExportState() == Before, "resource shortfall does not partly apply an action");
            for (std::size_t Node = 0; Node < k::GatherNodeCount; ++Node) World.Gather(Node);
            Check(World.ExportState().xp == Before.xp, "active supply collection grants no skill experience");
        }
        Check(false, "garden resources must make requested progress reachable");
    }
    void EverySeedSupportsProgress()
    {
        for (std::uint32_t Seed = 0; Seed < 256; ++Seed)
        {
            k::KnowledgeSystem World(Seed);
            LearnAll(World);
            const auto Initial = World.ExportState();
            for (std::size_t Node = 0; Node < k::GatherNodeCount; ++Node) Check(World.Gather(Node).ok, "initial resource route succeeds");
            const auto Gathered = World.ExportState();
            for (std::size_t Item = 0; Item < 7; ++Item) Check(Gathered.inventory[Item] > Initial.inventory[Item], "every seed exposes all seven raw materials");
            Check(World.ChooseQuest(0, Seed % 3u).ok && World.ChooseQuest(1, (Seed + 1u) % 3u).ok, "varied council choices leave crafting paths reachable");
            CompleteWithGarden(World, [&] { return World.Build(0); });
            // All supply comes from initial inventory, selected council gifts and active nodes.
            // In particular this progression test never calls GrantItem to bypass the economy.
            for (int Rank = 0; Rank < 3; ++Rank)
            {
                CompleteWithGarden(World, [&] { return World.Craft(1); });
                CompleteWithGarden(World, [&] { return World.Enchant(0); });
                CompleteWithGarden(World, [&] { return World.Craft(0); });
                CompleteWithGarden(World, [&] { return World.Craft(2); });
                CompleteWithGarden(World, [&] { return World.Enchant(1); });
            }
            CompleteWithGarden(World, [&] { return World.Craft(2); });
            CompleteWithGarden(World, [&] { return World.Build(1); });
            CompleteWithGarden(World, [&] { return World.Craft(1); });
            CompleteWithGarden(World, [&] { return World.Build(2); });
            Check(World.ChooseQuest(2, (Seed + 2u) % 3u).ok, "all council routes remain available after building");
            const auto Completed = World.ExportState();
            Check(Completed.enchantmentRanks[0] == 3 && Completed.enchantmentRanks[1] == 3, "every seed supports both full enchantments");
            for (int Count : Completed.craftedCounts) Check(Count > 0, "every alchemy recipe used on each seed");
            for (int Count : Completed.builtCounts) Check(Count > 0, "every module built on each seed");
            k::KnowledgeSystem Restored;
            Check(Restored.ImportState(Completed).ok && Restored.ExportState() == Completed, "renewable economy progression survives save restoration");
            const auto BeforeRepeat = Restored.ExportState();
            Check(Restored.Gather(0).ok && Restored.ExportState().inventory != BeforeRepeat.inventory, "previously harvested nodes remain renewable after reload");
        }
    }
    void StateValidation()
    {
        k::KnowledgeSystem World(847);
        LearnAll(World);
        Check(World.Craft(0).ok && World.Craft(1).ok && World.Craft(2).ok, "three alchemy recipes usable");
        Check(World.Enchant(0).ok && World.Build(0).ok && World.Build(1).ok, "mixed progression uses shared material inventory");
        Check(World.ChooseQuest(0, 0).ok && World.ChooseQuest(1, 1).ok && World.ChooseQuest(2, 2).ok, "mixed quest branch recorded");
        Check(World.RecordActiveReading(3, 0, 0.125).ok && World.Gather(7).ok, "save includes engagement and gathering");
        k::KnowledgeSystem Restored(0);
        Check(Restored.ImportState(World.ExportState()).ok && Restored.ExportState() == World.ExportState(), "complete state roundtrip preserves every field");
        Check(Restored.GetWorldSeed() == 847 && Restored.LandmarkSeed(8) == World.LandmarkSeed(8), "save restores deterministic world base");
        RejectSave(World, [](auto& S) { S.version = 0; }, "unsupported schema rejected");
        RejectSave(World, [](auto& S) { S.inventory[0] = -1; }, "negative inventory rejected");
        RejectSave(World, [](auto& S) { S.inventory[0] = 1000; }, "oversize inventory rejected");
        RejectSave(World, [](auto& S) { S.gatheredMask |= (1u << 16); }, "unknown gathering bits rejected");
        RejectSave(World, [](auto& S) { S.reading[0][0].activeMilliseconds = k::MaxPageMilliseconds + 1; }, "oversize engagement rejected");
        RejectSave(World, [](auto& S) { S.reading[0][0].attempts = 256; }, "oversize attempts rejected");
        RejectSave(World, [](auto& S) { S.reading[0][0].attempts = 0; }, "recalled page needs a recorded attempt");
        RejectSave(World, [](auto& S) { S.reading[3][2].activeMilliseconds = 1; }, "nonexistent page progress rejected");
        RejectSave(World, [](auto& S) { S.discoveredRecipes[0] = false; }, "recipe discovery must match recalled source");
        RejectSave(World, [](auto& S) { S.craftedCounts[0] = -1; }, "negative practice count rejected");
        RejectSave(World, [](auto& S) { S.craftedCounts[0] = 1001; }, "oversize practice count rejected");
        RejectSave(World, [](auto& S) { S.enchantmentRanks[0] = 4; }, "oversize enchantment rank rejected");
        RejectSave(World, [](auto& S) { S.builtCounts[0] = 33; }, "oversize settlement rejected");
        RejectSave(World, [](auto& S) { S.builtCounts[0] = 0; }, "camp without ground rejected");
        RejectSave(World, [](auto& S) { S.questChoices[1] = -1; }, "quest consequence without predecessor rejected");
        RejectSave(World, [](auto& S) { S.questChoices[0] = 3; }, "unknown quest choice rejected");
        RejectSave(World, [](auto& S) { ++S.kindness; }, "consequence values cannot diverge from choices");
        RejectSave(World, [](auto& S) { ++S.xp[0]; }, "XP cannot diverge from recall and paid practice");
        RejectSave(World, [](auto& S) { S.xp[0] = std::numeric_limits<int>::max(); }, "huge XP rejected");
        k::KnowledgeSystem NewWorld;
        RejectSave(NewWorld, [](auto& S) { S.discoveredRecipes[0] = true; }, "recipe cannot be known without source");
        RejectSave(NewWorld, [](auto& S) { S.enchantmentRanks[0] = 1; }, "enchantment requires learned sources on restore");
        RejectSave(NewWorld, [](auto& S) { S.builtCounts[0] = 1; }, "build requires learned sources on restore");
        Check(NewWorld.GetItemCount(k::ItemId::Count) == 0 && NewWorld.GetLevel(k::Skill::Count) == 0, "invalid enum getters are bounded");
    }
    void BoundedPracticeAndMixedTransitions()
    {
        k::KnowledgeSystem Crafter;
        Learn(Crafter, 0, 0);
        for (int Count = 0; Count < 1000; ++Count)
        {
            if (Crafter.GetItemCount(k::ItemId::Glassleaf) < 2) Check(Crafter.GrantItem(k::ItemId::Glassleaf, 2).ok, "practice refill glassleaf");
            if (Crafter.GetItemCount(k::ItemId::LumenDew) < 1) Check(Crafter.GrantItem(k::ItemId::LumenDew, 1).ok, "practice refill dew");
            Check(Crafter.Craft(0).ok && Crafter.ConsumeItem(k::ItemId::LumenDraught, 1).ok, "each practice performs a paid craft and output consumption");
        }
        Check(Crafter.GetXP(k::Skill::Alchemy) == k::MaxXP && Crafter.GetLevel(k::Skill::Alchemy) == 10, "XP and level remain capped through paid practice");
        RejectUnchanged(Crafter, [&] { return Crafter.Craft(0); }, "recipe practice record has finite capacity");
        k::KnowledgeSystem Reload;
        Check(Reload.ImportState(Crafter.ExportState()).ok, "capped XP state remains restorable");

        k::KnowledgeSystem World(321);
        std::uint32_t Sequence = 19;
        for (int Step = 0; Step < 1500; ++Step)
        {
            Sequence = Sequence * 1664525u + 1013904223u;
            const auto Index = static_cast<std::size_t>(Sequence >> 16);
            switch (Sequence % 9u)
            {
                case 0: World.Recall(Index % 7, (Index / 7) % 4, (Index / 28) % 4); break;
                case 1: World.RecordActiveReading(Index % 7, (Index / 7) % 4, 0.25); break;
                case 2: World.Craft(Index % 4); break;
                case 3: World.Enchant(Index % 3); break;
                case 4: World.Build(Index % 4); break;
                case 5: World.ChooseQuest(Index % 4, (Index / 4) % 4); break;
                case 6: World.Gather(Index % 20); break;
                case 7: World.GrantItem(static_cast<k::ItemId>(Index % 11), 1); break;
                case 8: World.ConsumeItem(static_cast<k::ItemId>(Index % 11), 1); break;
            }
            Check(Reload.ImportState(World.ExportState()).ok && Reload.ExportState() == World.ExportState(), "every mixed production transition emits a valid restorable state");
        }
    }
}

int main()
{
    CatalogContracts();
    ReadingAndRecall();
    CraftingAndEnchanting();
    ConstructionAndCouncil();
    DeterministicGathering();
    EverySeedSupportsProgress();
    StateValidation();
    BoundedPracticeAndMixedTransitions();
    std::cout << "KNOWLEDGE_TEST_PASS " << Checks << " checks; original catalogs, recall, paid practice, quests, seed and save invariants\n";
}
