using UnrealBuildTool;
using System.Collections.Generic;

public class HALVETHRealmsTarget : TargetRules
{
    public HALVETHRealmsTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Game;
        DefaultBuildSettings = BuildSettingsVersion.V7;
        IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
        ExtraModuleNames.Add("HALVETHRealms");
    }
}
