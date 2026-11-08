using UnrealBuildTool;
public class EarthContactEditorTarget : TargetRules
{
    public EarthContactEditorTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Editor;
        DefaultBuildSettings = BuildSettingsVersion.Latest;
        IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
        ExtraModuleNames.Add("EarthContact");
    }
}
