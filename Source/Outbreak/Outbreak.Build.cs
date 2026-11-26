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
			"Engine",
			"InputCore",
			// Public: UOutbreakInputConfig exposes Enhanced Input types in its header.
			"EnhancedInput",
			// Public: UOutbreakHUDWidget derives from UUserWidget.
			"UMG"
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"Slate",
			"SlateCore"
		});

		// Add further modules (AIModule, NavigationSystem, ...)
		// only when the code that needs them is added.
	}
}
