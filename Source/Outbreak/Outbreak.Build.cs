using UnrealBuildTool;

public class Outbreak : ModuleRules
{
	public Outbreak(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		// Lets gameplay code include headers by module-relative path,
		// e.g. #include "Squad/OutbreakSquadComponent.h".
		PublicIncludePaths.Add(ModuleDirectory);

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine"
		});

		// Add further modules (EnhancedInput, AIModule, NavigationSystem, UMG, ...)
		// only when the code that needs them is added.
	}
}
