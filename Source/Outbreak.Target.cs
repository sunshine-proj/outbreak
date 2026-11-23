using UnrealBuildTool;

public class OutbreakTarget : TargetRules
{
	public OutbreakTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;

		ExtraModuleNames.Add("Outbreak");
	}
}
