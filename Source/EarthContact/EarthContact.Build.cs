using UnrealBuildTool;
public class EarthContact : ModuleRules
{
    public EarthContact(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new[] { "Core", "CoreUObject", "Engine", "InputCore", "ProceduralMeshComponent", "AssetRegistry" });
    }
}
