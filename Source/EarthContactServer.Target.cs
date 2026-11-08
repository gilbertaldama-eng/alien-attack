using UnrealBuildTool;
public class EarthContactServerTarget : TargetRules
{
    public EarthContactServerTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Server;
        DefaultBuildSettings = BuildSettingsVersion.Latest;
        IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
        ExtraModuleNames.Add("EarthContact");
    }
}
