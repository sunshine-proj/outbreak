#include "Misc/AutomationTest.h"
#include "Modules/ModuleManager.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FOutbreakModuleLoadedTest,
	"Outbreak.Smoke.ModuleLoaded",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

/** Smoke test: confirms the game module is loaded and that the automation test pipeline can discover Outbreak tests. */
bool FOutbreakModuleLoadedTest::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("Outbreak module is loaded"), FModuleManager::Get().IsModuleLoaded(TEXT("Outbreak")));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
