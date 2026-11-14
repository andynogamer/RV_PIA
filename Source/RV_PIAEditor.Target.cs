using UnrealBuildTool;
public class RV_PIAEditorTarget : TargetRules
{
    public RV_PIAEditorTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Editor;
        DefaultBuildSettings = BuildSettingsVersion.V5;
        IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_6;
        ExtraModuleNames.Add("RV_PIA");
    }
}
