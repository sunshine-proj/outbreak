#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Interaction/OutbreakInteractable.h"
#include "Interaction/OutbreakInteractionComponent.h"
#include "Interaction/OutbreakTestInteractableActor.h"
#include "Player/OutbreakPlayerCharacter.h"
#include "Tests/OutbreakTestWorld.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FOutbreakInteractableToggleTest,
	"Outbreak.Interaction.TestInteractable.Toggle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

/** The test interactable implements the interface, toggles on interaction and updates its prompt. */
bool FOutbreakInteractableToggleTest::RunTest(const FString& Parameters)
{
	FOutbreakTestWorld TestWorld;
	AOutbreakTestInteractableActor* Interactable = TestWorld.Spawn<AOutbreakTestInteractableActor>();
	if (!TestNotNull(TEXT("Interactable spawned"), Interactable))
	{
		return false;
	}

	TestTrue(TEXT("Implements IOutbreakInteractable"), Interactable->Implements<UOutbreakInteractable>());
	TestTrue(TEXT("Can interact by default"), IOutbreakInteractable::Execute_CanInteract(Interactable, nullptr));

	const FText PromptBefore = IOutbreakInteractable::Execute_GetInteractionPrompt(Interactable, nullptr);
	TestFalse(TEXT("Prompt is not empty"), PromptBefore.IsEmpty());

	IOutbreakInteractable::Execute_Interact(Interactable, nullptr);
	TestTrue(TEXT("Active after first interaction"), Interactable->IsActive());
	TestEqual(TEXT("Interaction counted"), Interactable->GetInteractionCount(), 1);

	const FText PromptAfter = IOutbreakInteractable::Execute_GetInteractionPrompt(Interactable, nullptr);
	TestFalse(TEXT("Prompt changes after toggling"), PromptAfter.EqualTo(PromptBefore));

	IOutbreakInteractable::Execute_Interact(Interactable, nullptr);
	TestFalse(TEXT("Inactive after second interaction"), Interactable->IsActive());

	Interactable->SetInteractionEnabled(false);
	TestFalse(TEXT("Cannot interact when disabled"), IOutbreakInteractable::Execute_CanInteract(Interactable, nullptr));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FOutbreakInteractionRangeTest,
	"Outbreak.Interaction.Component.TargetValidation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

/**
 * Target validation accepts interactables within range and rejects targets that are out of range,
 * disabled, or do not implement the interface. The line trace itself is verified in the editor.
 */
bool FOutbreakInteractionRangeTest::RunTest(const FString& Parameters)
{
	FOutbreakTestWorld TestWorld;
	AOutbreakPlayerCharacter* Character = TestWorld.Spawn<AOutbreakPlayerCharacter>();
	AOutbreakTestInteractableActor* Interactable = TestWorld.Spawn<AOutbreakTestInteractableActor>(FVector(100.0f, 0.0f, 0.0f));
	if (!TestNotNull(TEXT("Character spawned"), Character) || !TestNotNull(TEXT("Interactable spawned"), Interactable))
	{
		return false;
	}

	const UOutbreakInteractionComponent* Interaction = Character->GetInteractionComponent();
	const float Range = Interaction->GetInteractionRange();
	TestTrue(TEXT("Interaction range is positive"), Range > 0.0f);

	TestTrue(TEXT("Valid well inside range"), Interaction->IsValidInteractionTarget(Interactable, Range * 0.5f));
	TestTrue(TEXT("Valid exactly at range"), Interaction->IsValidInteractionTarget(Interactable, Range));
	TestFalse(TEXT("Invalid beyond range"), Interaction->IsValidInteractionTarget(Interactable, Range + 1.0f));
	TestFalse(TEXT("Invalid for negative distance"), Interaction->IsValidInteractionTarget(Interactable, -1.0f));

	TestFalse(TEXT("Invalid for null actor"), Interaction->IsValidInteractionTarget(nullptr, 0.0f));
	TestFalse(TEXT("Invalid for non-interactable actor"), Interaction->IsValidInteractionTarget(Character, 0.0f));

	Interactable->SetInteractionEnabled(false);
	TestFalse(TEXT("Invalid when interactable refuses"), Interaction->IsValidInteractionTarget(Interactable, 0.0f));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
