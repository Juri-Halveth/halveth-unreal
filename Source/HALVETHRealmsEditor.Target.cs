using UnrealBuildTool;
using System.Collections.Generic;

public class HALVETHRealmsEditorTarget : TargetRules
{
    public HALVETHRealmsEditorTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Editor;
        DefaultBuildSettings = BuildSettingsVersion.V7;
        IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
        ExtraModuleNames.AddRange(new[] { "HALVETHRealms", "HALVETHRealmsEditor" });
    }
}
