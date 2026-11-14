using UnrealBuildTool;
public class RV_PIATarget : TargetRules
{
    public RV_PIATarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Game;
        DefaultBuildSettings = BuildSettingsVersion.V5;
        IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_6;
        ExtraModuleNames.Add("RV_PIA");
    }
}
