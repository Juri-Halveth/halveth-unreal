using UnrealBuildTool;
public class HALVETHVoidEditorTarget : TargetRules {
    public HALVETHVoidEditorTarget(TargetInfo Target) : base(Target) {
        Type = TargetType.Editor;
        DefaultBuildSettings = BuildSettingsVersion.V7;
        IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
        ExtraModuleNames.AddRange(new[] { "HALVETHVoid", "HALVETHVoidEditor" });
    }
}
