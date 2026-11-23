using UnrealBuildTool;

public class OutbreakEditorTarget : TargetRules
{
	public OutbreakEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;

		ExtraModuleNames.Add("Outbreak");
	}
}
