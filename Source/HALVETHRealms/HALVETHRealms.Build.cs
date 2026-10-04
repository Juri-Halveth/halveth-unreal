using UnrealBuildTool;

public class HALVETHRealms : ModuleRules
{
    public HALVETHRealms(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        CppStandard = CppStandardVersion.Cpp20;
        PublicDependencyModuleNames.AddRange(new[] { "Core", "CoreUObject", "Engine", "InputCore", "Json", "ProceduralMeshComponent" });
    }
}
