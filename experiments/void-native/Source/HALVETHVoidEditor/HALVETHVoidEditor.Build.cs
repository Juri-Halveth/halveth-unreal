using UnrealBuildTool;
public class HALVETHVoidEditor : ModuleRules {
    public HALVETHVoidEditor(ReadOnlyTargetRules Target) : base(Target) {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        CppStandard = CppStandardVersion.Cpp20;
        PrivateDependencyModuleNames.AddRange(new[] { "Core", "CoreUObject", "Engine", "UnrealEd", "AssetRegistry", "AssetTools", "MeshDescription", "StaticMeshDescription", "MeshUtilities", "Json", "HALVETHVoid" });
    }
}
