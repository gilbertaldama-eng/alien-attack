using UnrealBuildTool;
public class EarthContactClientTarget : TargetRules
{
    public EarthContactClientTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Client;
        DefaultBuildSettings = BuildSettingsVersion.Latest;
        IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
        ExtraModuleNames.Add("EarthContact");
    }
}
