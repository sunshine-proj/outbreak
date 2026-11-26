#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Interaction/OutbreakInteractionComponent.h"
#include "Player/OutbreakPlayerCharacter.h"
#include "Tests/OutbreakTestWorld.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FOutbreakPlayerCharacterComponentsTest,
	"Outbreak.Player.Character.Components",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

/** The character has a first-person camera driven by control rotation, and an interaction component. */
bool FOutbreakPlayerCharacterComponentsTest::RunTest(const FString& Parameters)
{
	FOutbreakTestWorld TestWorld;
	AOutbreakPlayerCharacter* Character = TestWorld.Spawn<AOutbreakPlayerCharacter>();
	if (!TestNotNull(TEXT("Character spawned"), Character))
	{
		return false;
	}

	const UCameraComponent* Camera = Character->GetFirstPersonCamera();
	if (TestNotNull(TEXT("First-person camera exists"), Camera))
	{
		TestTrue(TEXT("Camera uses pawn control rotation"), Camera->bUsePawnControlRotation);
		TestEqual(TEXT("Camera is at standing eye height"),
			static_cast<double>(Camera->GetRelativeLocation().Z),
			static_cast<double>(Character->GetMovementSettings().StandingEyeHeight));
	}

	TestNotNull(TEXT("Interaction component exists"), Character->GetInteractionComponent());
	TestTrue(TEXT("Character can crouch"), Character->GetCharacterMovement()->CanEverCrouch());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FOutbreakPlayerCharacterMovementSettingsTest,
	"Outbreak.Player.Character.MovementSettingsApplied",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

/** MovementSettings are pushed to the movement component when the character initializes. */
bool FOutbreakPlayerCharacterMovementSettingsTest::RunTest(const FString& Parameters)
{
	FOutbreakTestWorld TestWorld;
	AOutbreakPlayerCharacter* Character = TestWorld.Spawn<AOutbreakPlayerCharacter>();
	if (!TestNotNull(TEXT("Character spawned"), Character))
	{
		return false;
	}

	const FOutbreakMovementSettings& Settings = Character->GetMovementSettings();
	const UCharacterMovementComponent* Movement = Character->GetCharacterMovement();

	TestEqual(TEXT("MaxWalkSpeed is WalkSpeed"), Movement->MaxWalkSpeed, Settings.WalkSpeed);
	TestEqual(TEXT("MaxWalkSpeedCrouched is CrouchSpeed"), Movement->MaxWalkSpeedCrouched, Settings.CrouchSpeed);
	TestEqual(TEXT("JumpZVelocity applied"), Movement->JumpZVelocity, Settings.JumpZVelocity);
	TestEqual(TEXT("AirControl applied"), Movement->AirControl, Settings.AirControl);
	TestEqual(TEXT("Crouched half height applied"), Movement->GetCrouchedHalfHeight(), Settings.CrouchedHalfHeight);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FOutbreakPlayerCharacterSprintTest,
	"Outbreak.Player.Character.Sprint",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

/** Sprinting switches between walk and sprint speed, and is not allowed while crouching. */
bool FOutbreakPlayerCharacterSprintTest::RunTest(const FString& Parameters)
{
	FOutbreakTestWorld TestWorld;
	AOutbreakPlayerCharacter* Character = TestWorld.Spawn<AOutbreakPlayerCharacter>();
	if (!TestNotNull(TEXT("Character spawned"), Character))
	{
		return false;
	}

	const FOutbreakMovementSettings& Settings = Character->GetMovementSettings();
	const UCharacterMovementComponent* Movement = Character->GetCharacterMovement();

	TestTrue(TEXT("StartSprinting succeeds while standing"), Character->StartSprinting());
	TestTrue(TEXT("IsSprinting after start"), Character->IsSprinting());
	TestEqual(TEXT("Sprint speed applied"), Movement->MaxWalkSpeed, Settings.SprintSpeed);

	Character->StopSprinting();
	TestFalse(TEXT("Not sprinting after stop"), Character->IsSprinting());
	TestEqual(TEXT("Walk speed restored"), Movement->MaxWalkSpeed, Settings.WalkSpeed);

	Character->StartSprinting();
	Character->BeginCrouch();
	TestTrue(TEXT("Crouch requested"), Character->IsCrouchedOrCrouchRequested());
	TestFalse(TEXT("Crouching cancels sprint"), Character->IsSprinting());
	TestEqual(TEXT("Walk speed after crouch cancels sprint"), Movement->MaxWalkSpeed, Settings.WalkSpeed);

	TestFalse(TEXT("StartSprinting fails while crouching"), Character->StartSprinting());
	TestFalse(TEXT("Still not sprinting"), Character->IsSprinting());

	Character->EndCrouch();
	TestFalse(TEXT("Crouch request cleared"), Character->IsCrouchedOrCrouchRequested());
	TestTrue(TEXT("StartSprinting succeeds after standing up"), Character->StartSprinting());
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
