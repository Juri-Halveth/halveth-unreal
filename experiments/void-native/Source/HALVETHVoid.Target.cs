using UnrealBuildTool;
public class HALVETHVoidTarget : TargetRules {
    public HALVETHVoidTarget(TargetInfo Target) : base(Target) {
        Type = TargetType.Game;
        DefaultBuildSettings = BuildSettingsVersion.V7;
        IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
        ExtraModuleNames.Add("HALVETHVoid");
    }
}
