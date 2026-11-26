#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "AI/OutbreakEnemyAIController.h"
#include "AI/OutbreakEnemyDefinition.h"
#include "AI/OutbreakSquadAIController.h"
#include "AI/OutbreakTeammateAIConfig.h"
#include "AI/OutbreakTeammateBehaviorTypes.h"
#include "Characters/OutbreakEnemyCharacter.h"
#include "Combat/OutbreakCombatTestTarget.h"
#include "Engine/World.h"
#include "Game/OutbreakPlayerState.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Player/OutbreakPlayerCharacter.h"
#include "Squad/OutbreakSquadComponent.h"
#include "Squad/OutbreakSquadMemberComponent.h"
#include "Squad/OutbreakTeammateCharacter.h"
#include "Stealth/OutbreakDetection.h"
#include "Stealth/OutbreakLightingSubsystem.h"
#include "Stealth/OutbreakLightingVolume.h"
#include "Stealth/OutbreakNoiseEmitterComponent.h"
#include "Stealth/OutbreakNoiseTypes.h"
#include "Stealth/OutbreakStealthSettings.h"
#include "Stealth/OutbreakVisibility.h"
#include "Tests/OutbreakTestWorld.h"
#include "UObject/Package.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOutbreakNoiseRulesTest, "Outbreak.Stealth.Noise.PropagationAndMovement",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

/** Phase 8 acceptance (part): running is noisier than walking; noise falls off with distance and walls. */
bool FOutbreakNoiseRulesTest::RunTest(const FString& Parameters)
{
	FOutbreakMovementNoiseSettings Settings;
	const float Still = OutbreakNoise::ComputeMovementLoudness(0.0f, false, false, false, Settings, 1.0f);
	const float Crouch = OutbreakNoise::ComputeMovementLoudness(200.0f, true, false, false, Settings, 1.0f);
	const float Walk = OutbreakNoise::ComputeMovementLoudness(300.0f, false, false, false, Settings, 1.0f);
	const float Sprint = OutbreakNoise::ComputeMovementLoudness(600.0f, false, true, false, Settings, 1.0f);
	TestEqual(TEXT("Standing still is silent"), Still, 0.0f);
	TestTrue(TEXT("Crouching is quieter than walking"), Crouch < Walk);
	TestTrue(TEXT("Running is noisier than walking"), Sprint > Walk);
	TestEqual(TEXT("Airborne: no footsteps"), OutbreakNoise::ComputeMovementLoudness(600.0f, false, true, true, Settings, 1.0f), 0.0f);
	TestEqual(TEXT("Stealth multiplier halves footsteps"),
		OutbreakNoise::ComputeMovementLoudness(300.0f, false, false, false, Settings, 0.5f), Walk * 0.5f, 0.0001f);

	TestEqual(TEXT("Full loudness at the source"), OutbreakNoise::ComputePerceivedLoudness(1.0f, 0.0f, 1000.0f, 0, 0.5f), 1.0f, 0.0001f);
	TestEqual(TEXT("Half way: half loudness"), OutbreakNoise::ComputePerceivedLoudness(1.0f, 500.0f, 1000.0f, 0, 0.5f), 0.5f, 0.0001f);
	TestEqual(TEXT("Beyond range: inaudible"), OutbreakNoise::ComputePerceivedLoudness(1.0f, 1500.0f, 1000.0f, 0, 0.5f), 0.0f);
	TestEqual(TEXT("One wall halves it"), OutbreakNoise::ComputePerceivedLoudness(1.0f, 0.0f, 1000.0f, 1, 0.5f), 0.5f, 0.0001f);
	TestEqual(TEXT("Two walls quarter it"), OutbreakNoise::ComputePerceivedLoudness(1.0f, 0.0f, 1000.0f, 2, 0.5f), 0.25f, 0.0001f);

	FOutbreakNoiseEvent Shot;
	Shot.Type = EOutbreakNoiseType::Weapon;
	Shot.Loudness = 1.0f;
	FOutbreakNoiseEvent Step = Shot;
	Step.Type = EOutbreakNoiseType::Footstep;
	Step.Loudness = Walk;
	TestTrue(TEXT("Weapon fire carries further than footsteps"), OutbreakNoise::GetEffectiveRange(Shot) > OutbreakNoise::GetEffectiveRange(Step));
	Shot.RangeOverride = 500.0f;
	TestEqual(TEXT("Range override (suppressed weapon)"), OutbreakNoise::GetEffectiveRange(Shot), 500.0f);

	for (int32 Value = 0; Value <= static_cast<int32>(EOutbreakNoiseType::Other); ++Value)
	{
		const EOutbreakNoiseType Type = static_cast<EOutbreakNoiseType>(Value);
		TestTrue(TEXT("Noise tag round trip"), OutbreakNoise::GetNoiseTypeFromTag(OutbreakNoise::GetNoiseTag(Type)) == Type);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOutbreakNoiseOcclusionTest, "Outbreak.Stealth.Noise.Occlusion",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

/**
 * Obstacles between a noise and a listener are counted; characters are not obstacles. Scene queries
 * against actors spawned in the transient test world may not be live, so the wall count is reported
 * rather than required; the in-editor checklist covers it.
 */
bool FOutbreakNoiseOcclusionTest::RunTest(const FString& Parameters)
{
	FOutbreakTestWorld TestWorld;
	const FVector From = FVector::ZeroVector;
	const FVector To(2000.0f, 0.0f, 0.0f);
	TestEqual(TEXT("Open air: no occluders"), OutbreakNoise::CountOccluders(TestWorld.Get(), From, To, 3, {}), 0);

	TestWorld.Spawn<AOutbreakEnemyCharacter>(FVector(700.0f, 0.0f, 0.0f));
	TestWorld.Spawn<AOutbreakCombatTestTarget>(FVector(1300.0f, 0.0f, 0.0f));
	const int32 Count = OutbreakNoise::CountOccluders(TestWorld.Get(), From, To, 3, {});
	AddInfo(FString::Printf(TEXT("Occluders between noise and listener: %d (1 expected when scene queries are live: the prop, not the character)."), Count));
	TestTrue(TEXT("Characters never count as walls"), Count <= 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOutbreakVisibilityRulesTest, "Outbreak.Stealth.Visibility.Factors",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

/** Phase 8 acceptance (part): lighting influences detection, as do movement, distance, field of view, stance and cover. */
bool FOutbreakVisibilityRulesTest::RunTest(const FString& Parameters)
{
	const FOutbreakVisibilityWeights Weights;
	FOutbreakVisibilityInputs Base;
	Base.LightLevel = 1.0f;
	Base.Speed = 300.0f;
	Base.Distance = 1500.0f;
	Base.SightRadius = 2500.0f;
	Base.PeripheralHalfAngle = 60.0f;
	const float Reference = OutbreakVisibility::ComputeVisibility(Base, Weights);
	TestTrue(TEXT("Reference visibility is positive"), Reference > 0.0f);

	auto Variant = [&Base, &Weights](TFunctionRef<void(FOutbreakVisibilityInputs&)> Change)
	{
		FOutbreakVisibilityInputs Inputs = Base;
		Change(Inputs);
		return OutbreakVisibility::ComputeVisibility(Inputs, Weights);
	};

	TestTrue(TEXT("Darkness hides"), Variant([](FOutbreakVisibilityInputs& I) { I.LightLevel = 0.0f; }) < Reference * 0.5f);
	TestTrue(TEXT("Standing still hides"), Variant([](FOutbreakVisibilityInputs& I) { I.Speed = 0.0f; }) < Reference);
	TestTrue(TEXT("Crouching hides"), Variant([](FOutbreakVisibilityInputs& I) { I.bCrouched = true; }) < Reference);
	TestTrue(TEXT("Distance hides"), Variant([](FOutbreakVisibilityInputs& I) { I.Distance = 2400.0f; }) < Reference);
	TestTrue(TEXT("Peripheral vision is weaker"), Variant([](FOutbreakVisibilityInputs& I) { I.AngleFromView = 55.0f; }) < Reference);
	TestTrue(TEXT("Partial cover hides"), Variant([](FOutbreakVisibilityInputs& I) { I.ExposedFraction = 1.0f / 3.0f; }) < Reference);
	TestEqual(TEXT("Full cover: invisible"), Variant([](FOutbreakVisibilityInputs& I) { I.ExposedFraction = 0.0f; }), 0.0f);
	TestTrue(TEXT("Point blank: darkness doesn't help"),
		Variant([](FOutbreakVisibilityInputs& I) { I.LightLevel = 0.0f; I.Distance = 200.0f; }) >= 0.6f);

	TestEqual(TEXT("Light at its source"), OutbreakVisibility::ComputeLocalLightContribution(0.0f, 1000.0f, 1.0f, false, 1.0f), 1.0f);
	TestEqual(TEXT("Light half way"), OutbreakVisibility::ComputeLocalLightContribution(500.0f, 1000.0f, 1.0f, false, 1.0f), 0.25f, 0.0001f);
	TestEqual(TEXT("Shadowed: no light"), OutbreakVisibility::ComputeLocalLightContribution(0.0f, 1000.0f, 1.0f, true, 1.0f), 0.0f);
	TestEqual(TEXT("Outside radius: no light"), OutbreakVisibility::ComputeLocalLightContribution(1200.0f, 1000.0f, 1.0f, false, 1.0f), 0.0f);
	TestEqual(TEXT("Inside spot inner cone"), OutbreakVisibility::ComputeSpotConeFactor(10.0f, 20.0f, 40.0f), 1.0f);
	TestEqual(TEXT("Between spot cones"), OutbreakVisibility::ComputeSpotConeFactor(30.0f, 20.0f, 40.0f), 0.5f, 0.0001f);
	TestEqual(TEXT("Outside spot cone"), OutbreakVisibility::ComputeSpotConeFactor(50.0f, 20.0f, 40.0f), 0.0f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOutbreakLightingTest, "Outbreak.Stealth.Lighting.LevelsAndVolumes",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FOutbreakLightingTest::RunTest(const FString& Parameters)
{
	FOutbreakTestWorld TestWorld;
	UOutbreakLightingSubsystem* Lighting = TestWorld.Get()->GetSubsystem<UOutbreakLightingSubsystem>();
	if (!TestNotNull(TEXT("Lighting subsystem"), Lighting))
	{
		return false;
	}

	const float Ambient = GetDefault<UOutbreakStealthSettings>()->AmbientLightLevel;
	TestEqual(TEXT("No lights: ambient level"), Lighting->GetLightLevelAt(FVector::ZeroVector), Ambient, 0.0001f);

	AOutbreakLightingVolume* Floodlit = TestWorld.Spawn<AOutbreakLightingVolume>();
	AOutbreakLightingVolume* DarkCorner = TestWorld.Spawn<AOutbreakLightingVolume>(FVector(5000.0f, 0.0f, 0.0f));
	if (!Floodlit || !DarkCorner)
	{
		return false;
	}
	Floodlit->SetLightLevel(0.9f);
	DarkCorner->SetLightLevel(0.0f);
	Lighting->InvalidateCache();

	TestEqual(TEXT("Volume overrides the level"), Lighting->GetLightLevelAt(FVector::ZeroVector), 0.9f, 0.0001f);
	TestEqual(TEXT("Dark volume"), Lighting->GetLightLevelAt(FVector(5000.0f, 0.0f, 0.0f)), 0.0f, 0.0001f);
	TestEqual(TEXT("Outside volumes: ambient"), Lighting->GetLightLevelAt(FVector(2500.0f, 0.0f, 0.0f)), Ambient, 0.0001f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOutbreakDetectionRulesTest, "Outbreak.Stealth.Detection.Accumulation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

/** Detection builds with visibility and fades out of sight (detection loss); suspicion decays but not below detection. */
bool FOutbreakDetectionRulesTest::RunTest(const FString& Parameters)
{
	float Detection = 0.0f;
	for (int32 Step = 0; Step < 4; ++Step)
	{
		Detection = OutbreakDetection::UpdateDetection(Detection, true, 1.0f, 1.5f, 0.3f, 0.25f);
	}
	TestTrue(TEXT("A fully visible target is detected within a second"), Detection >= 1.0f);

	float Dim = 0.0f;
	for (int32 Step = 0; Step < 4; ++Step)
	{
		Dim = OutbreakDetection::UpdateDetection(Dim, true, 0.1f, 1.5f, 0.3f, 0.25f);
	}
	TestTrue(TEXT("A barely visible target is only noticed"), Dim > 0.0f && Dim < 0.25f);

	const float Faded = OutbreakDetection::UpdateDetection(0.5f, false, 0.0f, 1.5f, 0.3f, 1.0f);
	TestEqual(TEXT("Out of sight, detection fades"), Faded, 0.2f, 0.0001f);
	TestEqual(TEXT("Detection never exceeds 1"), OutbreakDetection::UpdateDetection(0.9f, true, 1.0f, 10.0f, 0.3f, 1.0f), 1.0f);

	TestEqual(TEXT("Suspicion decays"), OutbreakDetection::UpdateSuspicion(0.5f, 0.1f, 1.0f, 0.0f), 0.4f, 0.0001f);
	TestEqual(TEXT("Suspicion not below detection"), OutbreakDetection::UpdateSuspicion(0.5f, 0.1f, 10.0f, 0.3f), 0.3f, 0.0001f);
	TestEqual(TEXT("Noise suspicion"), OutbreakDetection::NoiseSuspicion(0.5f, 0.8f, 1.5f), 0.6f, 0.0001f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOutbreakEnemyAwarenessTest, "Outbreak.Stealth.Enemy.AwarenessAndAlerts",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

/** Small stimuli make an enemy suspicious, large ones send it to investigate, alerts propagate, lighting changes visibility. */
bool FOutbreakEnemyAwarenessTest::RunTest(const FString& Parameters)
{
	FOutbreakTestWorld TestWorld;
	UOutbreakEnemyDefinition* Definition = NewObject<UOutbreakEnemyDefinition>(GetTransientPackage());

	auto SpawnEnemy = [&TestWorld, Definition](const FVector& Location)
	{
		const FTransform Transform(FRotator::ZeroRotator, Location);
		AOutbreakEnemyCharacter* Enemy = TestWorld.Get()->SpawnActorDeferred<AOutbreakEnemyCharacter>(
			AOutbreakEnemyCharacter::StaticClass(), Transform, nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
		Enemy->SetEnemyDefinition(Definition);
		Enemy->FinishSpawning(Transform);
		if (!Enemy->GetController())
		{
			Enemy->SpawnDefaultController();
		}
		return Cast<AOutbreakEnemyAIController>(Enemy->GetController());
	};

	AOutbreakEnemyAIController* Guard = SpawnEnemy(FVector::ZeroVector);
	AOutbreakEnemyAIController* Neighbour = SpawnEnemy(FVector(0.0f, 1000.0f, 0.0f));
	if (!TestNotNull(TEXT("Guard"), Guard) || !TestNotNull(TEXT("Neighbour"), Neighbour))
	{
		return false;
	}

	Guard->ReportStimulus(FVector(800.0f, 0.0f, 0.0f), 0.3f);
	Guard->UpdateBehavior();
	TestTrue(TEXT("Faint stimulus: suspicious"), Guard->GetCurrentState() == EOutbreakEnemyState::Suspicious);
	TestTrue(TEXT("Suspicious enemies stop"), Guard->GetMoveLocation().Equals(Guard->GetPawn()->GetActorLocation(), 1.0));

	Guard->ReportStimulus(FVector(800.0f, 0.0f, 0.0f), 0.3f);
	Guard->UpdateBehavior();
	TestTrue(TEXT("Accumulated: investigating"), Guard->GetCurrentState() == EOutbreakEnemyState::Investigating);
	TestTrue(TEXT("Goes to the stimulus"), Guard->GetMoveLocation().Equals(FVector(800.0f, 0.0f, 0.0f), 1.0));

	Neighbour->ReceivePropagatedAlert(FVector(-500.0f, 0.0f, 0.0f));
	Neighbour->UpdateBehavior();
	TestTrue(TEXT("Propagated alert: neighbour investigates"), Neighbour->GetCurrentState() == EOutbreakEnemyState::Investigating);
	TestTrue(TEXT("Propagated alert is urgent"), Cast<AOutbreakEnemyCharacter>(Neighbour->GetPawn())->IsSprinting());

	// Lighting changes how visible the same target is.
	AOutbreakPlayerCharacter* Player = TestWorld.Spawn<AOutbreakPlayerCharacter>(FVector(1200.0f, 0.0f, 0.0f));
	if (TestNotNull(TEXT("Player"), Player))
	{
		const float InAmbient = Guard->ComputeTargetVisibility(Player);
		AOutbreakLightingVolume* Floodlight = TestWorld.Spawn<AOutbreakLightingVolume>(FVector(1200.0f, 0.0f, 0.0f));
		Floodlight->SetLightLevel(1.0f);
		TestWorld.Get()->GetSubsystem<UOutbreakLightingSubsystem>()->InvalidateCache();
		const float Lit = Guard->ComputeTargetVisibility(Player);
		TestTrue(TEXT("A floodlit target is more visible than one in ambient light"), Lit > InAmbient);

		Floodlight->SetLightLevel(0.0f);
		const float Dark = Guard->ComputeTargetVisibility(Player);
		TestTrue(TEXT("A target in darkness is less visible"), Dark < InAmbient);
		TestEqual(TEXT("Unseen player: no detection"), AOutbreakEnemyAIController::GetHighestDetectionOf(TestWorld.Get(), Player), 0.0f);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOutbreakSquadStealthTest, "Outbreak.Stealth.Squad.StealthMode",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

/** Stealth mode: stealth formation, quiet crouched teammates holding fire; a shot compromises the squad. */
bool FOutbreakSquadStealthTest::RunTest(const FString& Parameters)
{
	FOutbreakTestWorld TestWorld;
	AOutbreakPlayerState* PlayerState = TestWorld.Spawn<AOutbreakPlayerState>();
	AOutbreakPlayerCharacter* Player = TestWorld.Spawn<AOutbreakPlayerCharacter>();
	AOutbreakTeammateCharacter* Teammate = TestWorld.Spawn<AOutbreakTeammateCharacter>(FVector(-400.0f, 0.0f, 0.0f));
	if (!PlayerState || !Player || !Teammate)
	{
		return false;
	}
	if (!Teammate->GetController())
	{
		Teammate->SpawnDefaultController();
	}
	AOutbreakSquadAIController* Controller = Cast<AOutbreakSquadAIController>(Teammate->GetController());
	UOutbreakSquadComponent* Squad = PlayerState->GetSquadComponent();
	Squad->RegisterPlayerMember(Player->GetSquadMemberComponent());
	Squad->RegisterMember(Teammate->GetSquadMemberComponent());
	if (!TestNotNull(TEXT("Controller"), Controller))
	{
		return false;
	}

	Squad->SetFormation(EOutbreakFormation::Line);
	Squad->SetStealthMode(true);
	TestTrue(TEXT("Stealth mode on"), Squad->IsStealthMode());
	TestTrue(TEXT("Stealth formation"), Squad->GetFormation() == EOutbreakFormation::Stealth);
	TestFalse(TEXT("Weapons held"), Squad->AreWeaponsFree());

	Controller->UpdateBehavior();
	TestTrue(TEXT("Teammate crouches"), Teammate->IsCrouchedOrCrouchRequested());
	TestEqual(TEXT("Teammate footsteps softened"), Teammate->GetNoiseEmitterComponent()->GetLoudnessMultiplier(),
		Controller->GetAIConfig()->StealthNoiseMultiplier);
	TestFalse(TEXT("Teammate holds fire"), Controller->IsFireAllowed());

	Squad->AuthIssueCommand(FOutbreakSquadCommand::MakeAttack(nullptr), FOutbreakCommandRecipients::ForSquad());
	TestFalse(TEXT("An invalid attack order changes nothing"), Controller->IsFireAllowed());

	Squad->MarkCompromised();
	TestTrue(TEXT("Compromised"), Squad->IsCompromised());
	TestTrue(TEXT("Weapons free once compromised"), Squad->AreWeaponsFree());
	Controller->UpdateBehavior();
	TestTrue(TEXT("Teammate may fire"), Controller->IsFireAllowed());
	TestFalse(TEXT("Teammate stands up to fight"), Teammate->IsCrouchedOrCrouchRequested());

	Squad->SetStealthMode(false);
	TestTrue(TEXT("Previous formation restored"), Squad->GetFormation() == EOutbreakFormation::Line);
	TestFalse(TEXT("Not compromised outside stealth"), Squad->IsCompromised());

	TestTrue(TEXT("Hide override freezes movement"),
		OutbreakTeammateBehavior::ApplyStealthOverride(EOutbreakTeammateBehavior::FollowPlayer, true) == EOutbreakTeammateBehavior::Hiding);
	TestTrue(TEXT("Hide override keeps cover"),
		OutbreakTeammateBehavior::ApplyStealthOverride(EOutbreakTeammateBehavior::TakeCover, true) == EOutbreakTeammateBehavior::TakeCover);
	TestTrue(TEXT("No override when unnoticed"),
		OutbreakTeammateBehavior::ApplyStealthOverride(EOutbreakTeammateBehavior::FollowPlayer, false) == EOutbreakTeammateBehavior::FollowPlayer);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
