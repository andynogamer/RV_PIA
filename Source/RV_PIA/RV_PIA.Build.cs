using UnrealBuildTool;
public class RV_PIA : ModuleRules
{
    public RV_PIA(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new[] { "Core", "CoreUObject", "Engine", "InputCore", "ChaosVehicles", "UMG" });
        PrivateDependencyModuleNames.AddRange(new[] { "Slate", "SlateCore", "RenderCore", "XRBase" });
        if (Target.Platform == UnrealTargetPlatform.Win64)
        {
            PublicSystemLibraries.AddRange(new[] { "dinput8.lib", "dxguid.lib", "runtimeobject.lib" });
        }
    }
}
