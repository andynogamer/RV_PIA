using UnrealBuildTool;
public class RV_PIA : ModuleRules
{
    public RV_PIA(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new[] { "Core", "CoreUObject", "Engine", "InputCore", "ChaosVehicles" });
    }
}
