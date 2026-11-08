using UnrealBuildTool;
public class EarthContactTarget : TargetRules
{
    public EarthContactTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Game;
        DefaultBuildSettings = BuildSettingsVersion.Latest;
        IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
        ExtraModuleNames.Add("EarthContact");
    }
}
