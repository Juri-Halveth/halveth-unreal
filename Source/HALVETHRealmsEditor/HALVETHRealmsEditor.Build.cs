using UnrealBuildTool;

public class HALVETHRealmsEditor : ModuleRules
{
    public HALVETHRealmsEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PrivateDependencyModuleNames.AddRange(new[] {
            "Core", "CoreUObject", "Engine", "UnrealEd", "AssetRegistry", "AssetTools", "HALVETHRealms", "MeshDescription", "StaticMeshDescription"
        });
    }
}
