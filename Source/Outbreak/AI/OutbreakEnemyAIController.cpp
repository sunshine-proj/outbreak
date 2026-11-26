#include "AI/OutbreakEnemyAIController.h"

#include "AI/OutbreakBlackboardKeys.h"
#include "AI/OutbreakEnemyCoordinationSubsystem.h"
#include "AI/OutbreakEnemyDefinition.h"
#include "AI/OutbreakPatrolRoute.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BrainComponent.h"
#include "Characters/OutbreakEnemyCharacter.h"
#include "CollisionQueryParams.h"
#include "Combat/OutbreakTeam.h"
#include "Components/CapsuleComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "EnvironmentQuery/EnvQuery.h"
#include "EnvironmentQuery/EnvQueryManager.h"
#include "GameFramework/Character.h"
#include "HAL/IConsoleManager.h"
#include "Navigation/CrowdFollowingComponent.h"
#include "NavigationSystem.h"
#include "Outbreak.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Hearing.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Perception/AISense_Hearing.h"
#include "Perception/AISense_Sight.h"
#include "Stealth/OutbreakDetection.h"
#include "Stealth/OutbreakLightingSubsystem.h"
#include "Stealth/OutbreakNoiseTypes.h"
#include "Stealth/OutbreakStealthSettings.h"
#include "Stealth/OutbreakVisibility.h"
#include "Weapons/OutbreakWeaponComponent.h"

namespace
{
	TAutoConsoleVariable<int32> CVarDebugEnemies(
		TEXT("outbreak.AI.DebugEnemies"), 0,
		TEXT("1 = draw enemy sight range, hearing range, state, suspicion, target and destination at each decision update."),
		ECVF_Cheat);
}

AOutbreakEnemyAIController::AOutbreakEnemyAIController(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UCrowdFollowingComponent>(TEXT("PathFollowingComponent")))
{
	SetPerceptionComponent(*CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("PerceptionComponent")));

	// Vision and hearing are separate perception channels (AGENTS.md §6.2).
	SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));
	SightConfig->DetectionByAffiliation.bDetectEnemies = true;
	SightConfig->DetectionByAffiliation.bDetectNeutrals = false;
	SightConfig->DetectionByAffiliation.bDetectFriendlies = false;

	HearingConfig = CreateDefaultSubobject<UAISenseConfig_Hearing>(TEXT("HearingConfig"));
	HearingConfig->DetectionByAffiliation.bDetectEnemies = true;
	HearingConfig->DetectionByAffiliation.bDetectNeutrals = false;
	HearingConfig->DetectionByAffiliation.bDetectFriendlies = false;

	GetPerceptionComponent()->ConfigureSense(*SightConfig);
	GetPerceptionComponent()->ConfigureSense(*HearingConfig);
	GetPerceptionComponent()->SetDominantSense(SightConfig->GetSenseImplementation());

	SetGenericTeamId(OutbreakTeam::ToGenericTeamId(EOutbreakTeam::Hostile));
}

const UOutbreakEnemyDefinition* AOutbreakEnemyAIController::GetDefinition() const
{
	const UOutbreakEnemyDefinition* Definition = Enemy ? Enemy->GetEnemyDefinition() : nullptr;
	return Definition ? Definition : GetDefault<UOutbreakEnemyDefinition>();
}

double AOutbreakEnemyAIController::GetTimeSeconds() const
{
	const UWorld* World = GetWorld();
	return World ? World->GetTimeSeconds() : 0.0;
}

bool AOutbreakEnemyAIController::IsAlive(const AActor* Actor)
{
	if (!IsValid(Actor))
	{
		return false;
	}
	const UOutbreakHealthComponent* Health = Actor->FindComponentByClass<UOutbreakHealthComponent>();
	return !Health || Health->IsAlive();
}

void AOutbreakEnemyAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	Enemy = Cast<AOutbreakEnemyCharacter>(InPawn);
	if (!InPawn)
	{
		return;
	}

	WeaponComponent = InPawn->FindComponentByClass<UOutbreakWeaponComponent>();
	HealthComponent = InPawn->FindComponentByClass<UOutbreakHealthComponent>();

	SetGenericTeamId(FGenericTeamId::GetTeamIdentifier(InPawn));
	ApplyPerceptionConfig();

	GetPerceptionComponent()->OnTargetPerceptionUpdated.AddUniqueDynamic(this, &ThisClass::HandleTargetPerceptionUpdated);
	InPawn->OnTakeAnyDamage.AddUniqueDynamic(this, &ThisClass::HandleTakeAnyDamage);
	if (HealthComponent)
	{
		HealthComponent->OnHealthStateChanged.AddUniqueDynamic(this, &ThisClass::HandleHealthStateChanged);
	}

	HomeLocation = InPawn->GetActorLocation();
	MoveLocation = HomeLocation;

	if (UBehaviorTree* Tree = GetDefinition()->BehaviorTree)
	{
		RunBehaviorTree(Tree);
	}
	else
	{
		UE_LOG(LogOutbreak, Warning, TEXT("%s: enemy definition %s has no BehaviorTree; %s will not act."),
			*GetName(), *GetNameSafe(GetDefinition()), *InPawn->GetName());
	}
}

void AOutbreakEnemyAIController::OnUnPossess()
{
	if (APawn* OldPawn = GetPawn())
	{
		OldPawn->OnTakeAnyDamage.RemoveDynamic(this, &ThisClass::HandleTakeAnyDamage);
	}
	if (HealthComponent)
	{
		HealthComponent->OnHealthStateChanged.RemoveDynamic(this, &ThisClass::HandleHealthStateChanged);
	}
	GetPerceptionComponent()->OnTargetPerceptionUpdated.RemoveDynamic(this, &ThisClass::HandleTargetPerceptionUpdated);

	if (UOutbreakEnemyCoordinationSubsystem* Coordination = GetCoordination())
	{
		Coordination->RemoveEngagement(this);
	}
	ResetTactics();
	bRetreating = false;

	Enemy = nullptr;
	WeaponComponent = nullptr;
	HealthComponent = nullptr;

	Super::OnUnPossess();
}

void AOutbreakEnemyAIController::ApplyPerceptionConfig()
{
	const UOutbreakEnemyDefinition* Definition = GetDefinition();

	SightConfig->SightRadius = Definition->SightRadius;
	SightConfig->LoseSightRadius = FMath::Max(Definition->LoseSightRadius, Definition->SightRadius);
	SightConfig->PeripheralVisionAngleDegrees = Definition->PeripheralVisionHalfAngle;
	SightConfig->SetMaxAge(Definition->SightMaxAge);

	HearingConfig->HearingRange = Definition->HearingRange;
	HearingConfig->SetMaxAge(Definition->HearingMaxAge);

	GetPerceptionComponent()->ConfigureSense(*SightConfig);
	GetPerceptionComponent()->ConfigureSense(*HearingConfig);
}

// --- Stimuli -------------------------------------------------------------------------------------

void AOutbreakEnemyAIController::ReportStimulus(const FVector& Location, float SuspicionAmount)
{
	if (SuspicionAmount <= 0.0f)
	{
		return;
	}
	Suspicion = FMath::Clamp(Suspicion + SuspicionAmount, 0.0f, 1.0f);
	StimulusLocation = Location;
	bHasStimulus = true;
}

void AOutbreakEnemyAIController::ReceivePropagatedAlert(const FVector& Location)
{
	if (CurrentState != EOutbreakEnemyState::Combat && IsAlive(GetPawn()))
	{
		ReportStimulus(Location, 1.0f);
	}
}

void AOutbreakEnemyAIController::HandleTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
	// Sight is evaluated at each decision update; hearing arrives here as events.
	const APawn* ControlledPawn = GetPawn();
	if (!ControlledPawn || Stimulus.Type != UAISense::GetSenseID<UAISense_Hearing>() || !Stimulus.WasSuccessfullySensed())
	{
		return;
	}

	// Propagation: distance falloff and obstacles between the noise and this enemy's ears.
	FVector EarLocation;
	FRotator EarRotation;
	ControlledPawn->GetActorEyesViewPoint(EarLocation, EarRotation);

	FOutbreakNoiseEvent Noise;
	Noise.Type = OutbreakNoise::GetNoiseTypeFromTag(Stimulus.Tag);
	Noise.Loudness = Stimulus.Strength;

	const UOutbreakStealthSettings* Settings = GetDefault<UOutbreakStealthSettings>();
	const int32 Occluders = OutbreakNoise::CountOccluders(GetWorld(), Stimulus.StimulusLocation, EarLocation,
		Settings->MaxOccluders, { ControlledPawn, Actor });
	const float Perceived = OutbreakNoise::ComputePerceivedLoudness(Noise.Loudness,
		FVector::Dist(Stimulus.StimulusLocation, EarLocation), OutbreakNoise::GetEffectiveRange(Noise),
		Occluders, Settings->OcclusionAttenuation);

	const UOutbreakEnemyDefinition* Definition = GetDefinition();
	ReportStimulus(Stimulus.StimulusLocation,
		OutbreakDetection::NoiseSuspicion(Perceived, Definition->NoiseSuspicionGain, Definition->HearingSensitivity));
}

void AOutbreakEnemyAIController::HandleTakeAnyDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType,
	AController* InstigatedBy, AActor* DamageCauser)
{
	AActor* Attacker = InstigatedBy && InstigatedBy->GetPawn() ? InstigatedBy->GetPawn() : DamageCauser;
	if (!Attacker || !OutbreakTeam::IsHostile(GetPawn(), Attacker))
	{
		return;
	}

	LastAttacker = Attacker;
	LastAttackTime = GetTimeSeconds();

	if (IsActorVisible(Attacker))
	{
		// Shot by someone in plain view: detected at once.
		Detection.FindOrAdd(Attacker) = 1.0f;
	}
	else
	{
		// Shot from out of sight: fully alarmed, heading for where the attack came from.
		ReportAlert(Attacker->GetActorLocation());
	}
}

void AOutbreakEnemyAIController::HandleHealthStateChanged(UOutbreakHealthComponent* InHealthComponent,
	EOutbreakHealthState NewState, EOutbreakHealthState OldState)
{
	if (NewState == EOutbreakHealthState::Alive)
	{
		return;
	}

	CurrentTarget = nullptr;
	VisibleHostiles.Reset();
	Detection.Reset();
	Suspicion = 0.0f;
	bHasStimulus = false;
	if (UOutbreakEnemyCoordinationSubsystem* Coordination = GetCoordination())
	{
		Coordination->RemoveEngagement(this);
	}
	ResetTactics();
	bRetreating = false;
	ClearFocus(EAIFocusPriority::Gameplay);
	ClearFocus(EAIFocusPriority::Default);
	StopMovement();
	if (BrainComponent)
	{
		BrainComponent->StopLogic(TEXT("Downed"));
	}
}

// --- Sight and detection -------------------------------------------------------------------------

bool AOutbreakEnemyAIController::IsActorVisible(const AActor* Actor) const
{
	return Actor && VisibleHostiles.ContainsByPredicate(
		[Actor](const TWeakObjectPtr<AActor>& Entry) { return Entry.Get() == Actor; });
}

float AOutbreakEnemyAIController::GetDetectionOf(const AActor* Target) const
{
	if (!Target)
	{
		return 0.0f;
	}
	if (CurrentTarget.Get() == Target)
	{
		return 1.0f;
	}
	for (const TPair<TWeakObjectPtr<AActor>, float>& Entry : Detection)
	{
		if (Entry.Key.Get() == Target)
		{
			return Entry.Value;
		}
	}
	return 0.0f;
}

float AOutbreakEnemyAIController::GetHighestDetectionOf(const UWorld* World, const AActor* Target)
{
	float Highest = 0.0f;
	if (!World || !Target)
	{
		return Highest;
	}
	for (TActorIterator<AOutbreakEnemyAIController> It(World); It; ++It)
	{
		if (IsAlive(It->GetPawn()))
		{
			Highest = FMath::Max(Highest, It->GetDetectionOf(Target));
		}
	}
	return Highest;
}

float AOutbreakEnemyAIController::ComputeTargetVisibility(const AActor* Target) const
{
	const APawn* ControlledPawn = GetPawn();
	const UWorld* World = GetWorld();
	if (!ControlledPawn || !Target || !World)
	{
		return 0.0f;
	}

	const UOutbreakEnemyDefinition* Definition = GetDefinition();
	FVector EyeLocation;
	FRotator EyeRotation;
	ControlledPawn->GetActorEyesViewPoint(EyeLocation, EyeRotation);

	const FVector TargetLocation = Target->GetActorLocation();
	const FVector ToTarget = TargetLocation - EyeLocation;

	// Obstruction: how many of head, chest and legs are in line of sight.
	const ACharacter* TargetCharacter = Cast<ACharacter>(Target);
	const float HalfHeight = TargetCharacter ? TargetCharacter->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() : 0.0f;
	const FVector SamplePoints[] = {
		TargetLocation + FVector(0.0f, 0.0f, HalfHeight * 0.8f),
		TargetLocation,
		TargetLocation - FVector(0.0f, 0.0f, HalfHeight * 0.6f)
	};
	FCollisionQueryParams Params(SCENE_QUERY_STAT(OutbreakEnemyVisibility), false, ControlledPawn);
	Params.AddIgnoredActor(Target);
	int32 Exposed = 0;
	for (const FVector& Point : SamplePoints)
	{
		FHitResult Hit;
		Exposed += World->LineTraceSingleByChannel(Hit, EyeLocation, Point, ECC_Visibility, Params) ? 0 : 1;
	}

	UOutbreakLightingSubsystem* Lighting = World->GetSubsystem<UOutbreakLightingSubsystem>();

	FOutbreakVisibilityInputs Inputs;
	Inputs.LightLevel = Lighting ? Lighting->GetLightLevelAt(TargetLocation, Target) : 1.0f;
	Inputs.ExposedFraction = Exposed / 3.0f;
	Inputs.Speed = Target->GetVelocity().Size2D();
	Inputs.bCrouched = TargetCharacter && TargetCharacter->bIsCrouched;
	Inputs.Distance = ToTarget.Size();
	Inputs.SightRadius = Definition->SightRadius;
	Inputs.AngleFromView = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(
		FVector::DotProduct(EyeRotation.Vector(), ToTarget.GetSafeNormal()), -1.0f, 1.0f)));
	Inputs.PeripheralHalfAngle = Definition->PeripheralVisionHalfAngle;

	return OutbreakVisibility::ComputeVisibility(Inputs, Definition->VisibilityWeights);
}

void AOutbreakEnemyAIController::UpdateSight(float DeltaSeconds)
{
	VisibleHostiles.Reset();
	const APawn* ControlledPawn = GetPawn();
	const UOutbreakEnemyDefinition* Definition = GetDefinition();

	TArray<AActor*> PerceivedActors;
	GetPerceptionComponent()->GetCurrentlyPerceivedActors(UAISense_Sight::StaticClass(), PerceivedActors);

	// Seen targets gain detection by their visibility.
	TSet<const AActor*> Seen;
	for (AActor* Actor : PerceivedActors)
	{
		if (!IsAlive(Actor) || !OutbreakTeam::IsHostile(ControlledPawn, Actor))
		{
			continue;
		}
		VisibleHostiles.Add(Actor);
		Seen.Add(Actor);

		float& Level = Detection.FindOrAdd(Actor);
		Level = OutbreakDetection::UpdateDetection(Level, true, ComputeTargetVisibility(Actor),
			Definition->SightDetectionRate, Definition->DetectionDecayRate, DeltaSeconds);
	}

	// Unseen targets fade; dead or destroyed ones are forgotten.
	float HighestDetection = 0.0f;
	const AActor* MostNoticed = nullptr;
	for (auto It = Detection.CreateIterator(); It; ++It)
	{
		const AActor* Actor = It.Key().Get();
		if (!IsAlive(Actor))
		{
			It.RemoveCurrent();
			continue;
		}
		if (!Seen.Contains(Actor))
		{
			It.Value() = OutbreakDetection::UpdateDetection(It.Value(), false, 0.0f,
				Definition->SightDetectionRate, Definition->DetectionDecayRate, DeltaSeconds);
			if (It.Value() <= 0.0f)
			{
				It.RemoveCurrent();
				continue;
			}
		}
		else if (It.Value() > HighestDetection)
		{
			HighestDetection = It.Value();
			MostNoticed = Actor;
		}
	}

	// A glimpse of someone makes their position the thing to look at.
	if (MostNoticed && HighestDetection >= Definition->SuspiciousThreshold)
	{
		StimulusLocation = MostNoticed->GetActorLocation();
		bHasStimulus = true;
	}

	// Choose the target among detected, visible hostiles. Allies already fighting a target make it
	// less attractive (fire spreads across the squad), and the current target is kept unless another
	// is clearly better (Phase 9 target switching).
	FOutbreakTargetScoreWeights Weights;
	Weights.DistancePenaltyPerMeter = Definition->DistancePenaltyPerMeter;
	Weights.RecentAttackerBonus = Definition->RecentAttackerBonus;
	Weights.PlayerBonus = Definition->PlayerBonus;
	const bool bAttackerIsRecent = GetTimeSeconds() - LastAttackTime <= Definition->AttackerMemory;
	const FOutbreakEnemyTacticsSettings& Tactics = Definition->Tactics;
	const UOutbreakEnemyCoordinationSubsystem* Coordination = Tactics.CoordinationRadius > 0.0f ? GetCoordination() : nullptr;

	TArray<AActor*> Candidates;
	TArray<float> Scores;
	int32 CurrentIndex = INDEX_NONE;
	for (const TWeakObjectPtr<AActor>& Entry : VisibleHostiles)
	{
		AActor* Actor = Entry.Get();
		const float* Level = Detection.Find(Actor);
		if (!Actor || !Level || *Level < 1.0f)
		{
			continue;
		}
		const APawn* ActorPawn = Cast<APawn>(Actor);
		FOutbreakTargetCandidate Candidate;
		Candidate.DistanceCm = FVector::Dist(ControlledPawn->GetActorLocation(), Actor->GetActorLocation());
		Candidate.bIsRecentAttacker = bAttackerIsRecent && LastAttacker.Get() == Actor;
		Candidate.bIsPlayer = ActorPawn && ActorPawn->IsPlayerControlled();
		Candidate.OtherAttackers = Coordination
			? Coordination->CountAlliesEngaging(Actor, ControlledPawn->GetActorLocation(), Tactics.CoordinationRadius, this)
			: 0;

		if (Actor == CurrentTarget.Get())
		{
			CurrentIndex = Candidates.Num();
		}
		Candidates.Add(Actor);
		Scores.Add(OutbreakEnemyCoordination::ScoreCandidate(Candidate, Weights, Tactics.SharedTargetPenalty));
	}

	const int32 Chosen = OutbreakEnemyCoordination::SelectTarget(Scores, CurrentIndex, Tactics.TargetSwitchMargin);
	AActor* BestTarget = Candidates.IsValidIndex(Chosen) ? Candidates[Chosen] : nullptr;

	if (BestTarget)
	{
		CurrentTarget = BestTarget;
		LastSeenTargetLocation = BestTarget->GetActorLocation();
		LastSeenTargetTime = GetTimeSeconds();
		LastOwnSightTime = LastSeenTargetTime;
	}
	else if (!IsAlive(CurrentTarget.Get()))
	{
		// The tracked target died or was destroyed: no search for it.
		CurrentTarget = nullptr;
	}

	Suspicion = OutbreakDetection::UpdateSuspicion(Suspicion, Definition->SuspicionDecayRate, DeltaSeconds, HighestDetection);
	if (CurrentTarget.IsValid())
	{
		Suspicion = 1.0f;
	}
	if (Suspicion < Definition->SuspiciousThreshold)
	{
		bHasStimulus = false;
	}
}

void AOutbreakEnemyAIController::PropagateAlert(const FVector& Location)
{
	const float Radius = GetDefinition()->AlertPropagationRadius;
	const APawn* ControlledPawn = GetPawn();
	if (Radius <= 0.0f || !ControlledPawn)
	{
		return;
	}

	for (TActorIterator<AOutbreakEnemyAIController> It(GetWorld()); It; ++It)
	{
		const APawn* OtherPawn = It->GetPawn();
		if (*It != this && OtherPawn && FVector::Dist(OtherPawn->GetActorLocation(), ControlledPawn->GetActorLocation()) <= Radius)
		{
			It->ReceivePropagatedAlert(Location);
		}
	}
}

// --- Decision ------------------------------------------------------------------------------------

void AOutbreakEnemyAIController::UpdateBehavior()
{
	if (bUpdatingBehavior)
	{
		return;
	}
	TGuardValue<bool> UpdatingGuard(bUpdatingBehavior, true);

	APawn* ControlledPawn = GetPawn();
	if (!ControlledPawn || (HealthComponent && !HealthComponent->IsAlive()))
	{
		return;
	}

	const double Now = GetTimeSeconds();
	const float DeltaSeconds = LastUpdateTime < 0.0 ? 0.0f : static_cast<float>(FMath::Clamp(Now - LastUpdateTime, 0.0, 1.0));
	LastUpdateTime = Now;

	UpdateSight(DeltaSeconds);

	const UOutbreakEnemyDefinition* Definition = GetDefinition();

	// Allies that still see the target keep it tracked (shared sightings).
	RefreshFromSharedSightings(Now);

	// A detected target out of sight for too long is lost: search where it was last seen, or
	// disengage when that is beyond the leash.
	if (CurrentTarget.IsValid() && Now - LastSeenTargetTime > Definition->LoseTargetTime)
	{
		LoseTarget(Now);
	}

	UpdateRetreat(Now);

	const AOutbreakPatrolRoute* Route = Enemy ? Enemy->GetPatrolRoute() : nullptr;

	FOutbreakEnemyStateContext Context;
	Context.bRetreating = bRetreating;
	Context.bHasDetectedTarget = CurrentTarget.IsValid();
	Context.TimeSinceTargetSeen = static_cast<float>(Now - LastSeenTargetTime);
	Context.LoseTargetTime = Definition->LoseTargetTime;
	Context.bSearching = Now - SearchStartTime < Definition->SearchDuration;
	Context.Suspicion = Suspicion;
	Context.SuspiciousThreshold = Definition->SuspiciousThreshold;
	Context.InvestigateThreshold = Definition->InvestigateThreshold;
	Context.bHasStimulusLocation = bHasStimulus;
	Context.bHasPatrolRoute = Route && Route->GetNumPoints() > 0;

	const EOutbreakEnemyState NewState = OutbreakEnemyBehavior::SelectState(Context);
	const bool bStateChanged = NewState != CurrentState;

	if (bStateChanged && NewState == EOutbreakEnemyState::Combat)
	{
		SearchStartTime = -1.0e9;
		PropagateAlert(LastSeenTargetLocation);
	}
	if (bStateChanged && NewState == EOutbreakEnemyState::Patrol)
	{
		PatrolArrivalTime = -1.0;
	}

	CurrentState = NewState;
	UpdateEngagementReport(NewState);
	MoveLocation = ComputeMoveLocation(bStateChanged);

	// Look towards whatever made it suspicious (lowest focus priority; target focus wins).
	if ((NewState == EOutbreakEnemyState::Suspicious || NewState == EOutbreakEnemyState::Investigating) && bHasStimulus)
	{
		SetFocalPoint(StimulusLocation, EAIFocusPriority::Default);
	}
	else
	{
		ClearFocus(EAIFocusPriority::Default);
	}

	if (Enemy)
	{
		// The Advance style (heavies) walks into a fight; everyone runs when retreating.
		const bool bFightsAtWalk = Definition->Tactics.CombatStyle == EOutbreakEnemyCombatStyle::Advance;
		const bool bHurry = (NewState == EOutbreakEnemyState::Combat && !bFightsAtWalk)
			|| NewState == EOutbreakEnemyState::Retreating
			|| (NewState == EOutbreakEnemyState::Investigating && Suspicion >= Definition->UrgentThreshold);
		if (bHurry && !Enemy->IsSprinting())
		{
			Enemy->StartSprinting();
		}
		else if (!bHurry && Enemy->IsSprinting())
		{
			Enemy->StopSprinting();
		}
		Enemy->SetEnemyState(NewState);
	}

	WriteBlackboard();
	DrawDebug();
}

FVector AOutbreakEnemyAIController::UpdatePatrol()
{
	const AOutbreakPatrolRoute* Route = Enemy ? Enemy->GetPatrolRoute() : nullptr;
	if (!Route || Route->GetNumPoints() == 0)
	{
		return HomeLocation;
	}

	const UOutbreakEnemyDefinition* Definition = GetDefinition();
	PatrolIndex = FMath::Clamp(PatrolIndex, 0, Route->GetNumPoints() - 1);
	FVector Point = Route->GetWorldPoint(PatrolIndex);

	if (FVector::Dist2D(GetPawn()->GetActorLocation(), Point) <= Definition->PatrolAcceptanceRadius)
	{
		const double Now = GetTimeSeconds();
		if (PatrolArrivalTime < 0.0)
		{
			PatrolArrivalTime = Now;
		}
		if (Now - PatrolArrivalTime >= Definition->PatrolWaitTime)
		{
			PatrolIndex = AOutbreakPatrolRoute::GetNextIndex(PatrolIndex, Route->GetNumPoints(), Route->IsLooping(), PatrolDirection);
			PatrolArrivalTime = -1.0;
			Point = Route->GetWorldPoint(PatrolIndex);
		}
	}
	return Point;
}

FVector AOutbreakEnemyAIController::UpdateSearch()
{
	const UOutbreakEnemyDefinition* Definition = GetDefinition();
	const double Now = GetTimeSeconds();

	const bool bArrived = bHasSearchPoint
		&& FVector::Dist2D(GetPawn()->GetActorLocation(), SearchPoint) <= Definition->PatrolAcceptanceRadius;
	if (bArrived && SearchPointArrivalTime < 0.0)
	{
		SearchPointArrivalTime = Now;
	}

	if (!bHasSearchPoint || (bArrived && Now - SearchPointArrivalTime >= Definition->SearchPointWait))
	{
		SearchPoint = SearchCenter;
		if (UNavigationSystemV1* NavigationSystem = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld()))
		{
			FNavLocation RandomPoint;
			if (NavigationSystem->GetRandomReachablePointInRadius(SearchCenter, Definition->SearchRadius, RandomPoint))
			{
				SearchPoint = RandomPoint.Location;
			}
		}
		bHasSearchPoint = true;
		SearchPointArrivalTime = -1.0;
	}
	return SearchPoint;
}

FVector AOutbreakEnemyAIController::ComputeMoveLocation(bool bStateChanged)
{
	const FVector PawnLocation = GetPawn()->GetActorLocation();

	switch (CurrentState)
	{
	case EOutbreakEnemyState::Patrol:
		return UpdatePatrol();

	case EOutbreakEnemyState::Suspicious:
		// Stop where it noticed something and look.
		return bStateChanged ? PawnLocation : MoveLocation;

	case EOutbreakEnemyState::Investigating:
		return StimulusLocation;

	case EOutbreakEnemyState::Searching:
		return UpdateSearch();

	case EOutbreakEnemyState::Combat:
		return ComputeCombatMoveLocation(GetTimeSeconds());

	case EOutbreakEnemyState::Retreating:
		CombatMove = EOutbreakEnemyCombatMove::Retreat;
		return RetreatLocation;

	case EOutbreakEnemyState::Idle:
	default:
		return HomeLocation;
	}
}

float AOutbreakEnemyAIController::GetEngagementRange() const
{
	return GetDefinition()->MaxCombatRange;
}

float AOutbreakEnemyAIController::GetAimTolerance() const
{
	return GetDefinition()->AimTolerance;
}

float AOutbreakEnemyAIController::GetIdleReloadFraction() const
{
	return GetDefinition()->IdleReloadFraction;
}


// --- Tactics (Phase 9) ---------------------------------------------------------------------------

UOutbreakEnemyCoordinationSubsystem* AOutbreakEnemyAIController::GetCoordination() const
{
	const UWorld* World = GetWorld();
	return World ? World->GetSubsystem<UOutbreakEnemyCoordinationSubsystem>() : nullptr;
}

bool AOutbreakEnemyAIController::ProjectToNavigation(FVector& InOutLocation) const
{
	UNavigationSystemV1* NavigationSystem = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	if (!NavigationSystem || !NavigationSystem->GetDefaultNavDataInstance(FNavigationSystem::DontCreate))
	{
		// No navigation data (tests, or a level without a navmesh): accept the point as is.
		return true;
	}

	FNavLocation Projected;
	if (NavigationSystem->ProjectPointToNavigation(InOutLocation, Projected, FVector(150.0f, 150.0f, 250.0f)))
	{
		InOutLocation = Projected.Location;
		return true;
	}
	return false;
}

void AOutbreakEnemyAIController::ResetTactics()
{
	CombatMove = EOutbreakEnemyCombatMove::None;
	bHasCover = false;
	FlankOrder = INDEX_NONE;
}

void AOutbreakEnemyAIController::SetCoverLocation(const FVector& Location)
{
	CoverLocation = Location;
	bHasCover = true;
	LastCoverQueryTime = GetTimeSeconds();
}

void AOutbreakEnemyAIController::RefreshFromSharedSightings(double Now)
{
	const UOutbreakEnemyDefinition* Definition = GetDefinition();
	const UOutbreakEnemyCoordinationSubsystem* Coordination = GetCoordination();
	const AActor* Target = CurrentTarget.Get();
	const APawn* ControlledPawn = GetPawn();
	if (!Coordination || !Target || !ControlledPawn || Definition->Tactics.CoordinationRadius <= 0.0f || IsActorVisible(Target))
	{
		return;
	}

	FVector SharedLocation;
	double SharedTime = 0.0;
	if (Coordination->GetLatestSighting(Target, ControlledPawn->GetActorLocation(), Definition->Tactics.CoordinationRadius,
		this, SharedLocation, SharedTime) && SharedTime > LastSeenTargetTime)
	{
		LastSeenTargetLocation = SharedLocation;
		LastSeenTargetTime = SharedTime;
	}
}

void AOutbreakEnemyAIController::LoseTarget(double Now)
{
	const UOutbreakEnemyDefinition* Definition = GetDefinition();
	CurrentTarget = nullptr;
	ResetTactics();

	if (OutbreakEnemyTactics::IsWithinLeash(LastSeenTargetLocation, HomeLocation, Definition->Tactics.LeashDistance))
	{
		SearchStartTime = Now;
		SearchCenter = LastSeenTargetLocation;
		bHasSearchPoint = false;
		Suspicion = FMath::Max(Suspicion, Definition->InvestigateThreshold);
		return;
	}

	// Disengage: the target got away beyond the leash. Calm down and go home.
	SearchStartTime = -1.0e9;
	Suspicion = 0.0f;
	bHasStimulus = false;
}

void AOutbreakEnemyAIController::UpdateRetreat(double Now)
{
	const UOutbreakEnemyDefinition* Definition = GetDefinition();
	const FOutbreakEnemyTacticsSettings& Tactics = Definition->Tactics;

	if (bRetreating)
	{
		if (Now - RetreatStartTime >= Tactics.RetreatDuration)
		{
			bRetreating = false;
		}
		return;
	}

	// Only a fight is retreated from: a tracked target, or a recent attacker.
	const bool bInFight = CurrentTarget.IsValid() || Now - LastAttackTime <= Tactics.UnderFireTime;
	if (!bInFight)
	{
		return;
	}

	const float HealthFraction = HealthComponent && HealthComponent->GetMaxHealth() > 0.0f
		? HealthComponent->GetHealth() / HealthComponent->GetMaxHealth()
		: 1.0f;
	if (OutbreakEnemyTactics::ShouldRetreatForHealth(HealthFraction, Tactics.RetreatHealthFraction, bRetreatedForHealth))
	{
		bRetreatedForHealth = StartRetreat(Now);
		return;
	}

	const UOutbreakEnemyCoordinationSubsystem* Coordination = GetCoordination();
	const int32 Allies = 1 + (Coordination && GetPawn() && Tactics.CoordinationRadius > 0.0f
		? Coordination->CountAlliesInCombat(GetPawn()->GetActorLocation(), Tactics.CoordinationRadius, this)
		: 0);
	if (Now - LastNumbersRetreatTime >= Tactics.RetreatCooldown
		&& OutbreakEnemyTactics::IsOutnumbered(VisibleHostiles.Num(), Allies, Tactics.OutnumberedRatio))
	{
		if (StartRetreat(Now))
		{
			LastNumbersRetreatTime = Now;
		}
	}
}

bool AOutbreakEnemyAIController::StartRetreat(double Now)
{
	const APawn* ControlledPawn = GetPawn();
	FVector Threat;
	if (!ControlledPawn || !GetCoverThreatLocation(Threat))
	{
		return false;
	}
	const UOutbreakEnemyDefinition* Definition = GetDefinition();

	bRetreating = true;
	RetreatStartTime = Now;
	ResetTactics();

	RetreatLocation = OutbreakEnemyTactics::ClampToLeash(
		OutbreakEnemyTactics::ComputeRetreatPosition(ControlledPawn->GetActorLocation(), Threat, Definition->Tactics.RetreatDistance),
		HomeLocation, Definition->Tactics.LeashDistance);
	if (!ProjectToNavigation(RetreatLocation))
	{
		// Nowhere to go straight back (a wall behind): fall back home instead.
		RetreatLocation = HomeLocation;
	}
	return true;
}

void AOutbreakEnemyAIController::UpdateEngagementReport(EOutbreakEnemyState State)
{
	UOutbreakEnemyCoordinationSubsystem* Coordination = GetCoordination();
	const APawn* ControlledPawn = GetPawn();
	const bool bFighting = (State == EOutbreakEnemyState::Combat || State == EOutbreakEnemyState::Retreating)
		&& CurrentTarget.IsValid();

	if (!bFighting)
	{
		if (State != EOutbreakEnemyState::Retreating)
		{
			ResetTactics();
		}
		if (Coordination)
		{
			Coordination->RemoveEngagement(this);
		}
		return;
	}

	if (Coordination && ControlledPawn)
	{
		const FOutbreakEnemyTacticsSettings& Tactics = GetDefinition()->Tactics;
		const bool bCanFlank = Tactics.bCanFlank && Tactics.CombatStyle != EOutbreakEnemyCombatStyle::Melee
			&& State != EOutbreakEnemyState::Retreating;
		AActor* Target = CurrentTarget.Get();
		const bool bSeen = IsActorVisible(Target);
		Coordination->ReportEngagement(this, Target, ControlledPawn->GetActorLocation(), bCanFlank, bSeen,
			bSeen ? Target->GetActorLocation() : LastSeenTargetLocation);
	}
}

bool AOutbreakEnemyAIController::UpdateFlank(const FVector& TargetLocation, bool& bOutReached)
{
	bOutReached = false;
	const UOutbreakEnemyDefinition* Definition = GetDefinition();
	const FOutbreakEnemyTacticsSettings& Tactics = Definition->Tactics;
	UOutbreakEnemyCoordinationSubsystem* Coordination = GetCoordination();
	const APawn* ControlledPawn = GetPawn();
	if (!Coordination || !ControlledPawn || Tactics.CoordinationRadius <= 0.0f || !Tactics.bCanFlank
		|| Tactics.CombatStyle == EOutbreakEnemyCombatStyle::Melee)
	{
		FlankOrder = INDEX_NONE;
		return false;
	}

	const FOutbreakFlankAssignment Assignment = Coordination->GetFlankAssignment(this, Tactics.CoordinationRadius,
		Tactics.MaxFlankersPerTarget);
	if (!Assignment.IsFlanking())
	{
		FlankOrder = INDEX_NONE;
		return false;
	}

	if (Assignment.FlankOrder != FlankOrder)
	{
		// New role: pick the side away from the rest of the target's squad (squad-aware flanking).
		TArray<FVector> OtherHostiles;
		for (const TPair<TWeakObjectPtr<AActor>, float>& Entry : Detection)
		{
			const AActor* Known = Entry.Key.Get();
			if (Known && Known != CurrentTarget.Get())
			{
				OtherHostiles.Add(Known->GetActorLocation());
			}
		}
		const float Distance = Definition->PreferredCombatRange;
		const FVector Left = OutbreakEnemyTactics::ComputeFlankPosition(TargetLocation, Assignment.PinLocation, Tactics.FlankAngle, 1, Distance);
		const FVector Right = OutbreakEnemyTactics::ComputeFlankPosition(TargetLocation, Assignment.PinLocation, Tactics.FlankAngle, -1, Distance);
		const int32 PreferredSide = OutbreakEnemyTactics::ChooseFlankSide(Left, Right, OtherHostiles);
		FlankSide = Assignment.FlankOrder % 2 == 0 ? PreferredSide : -PreferredSide;
		FlankOrder = Assignment.FlankOrder;
	}

	FVector Candidate = OutbreakEnemyTactics::ClampToLeash(
		OutbreakEnemyTactics::ComputeFlankPosition(TargetLocation, Assignment.PinLocation, Tactics.FlankAngle, FlankSide,
			Definition->PreferredCombatRange),
		HomeLocation, Tactics.LeashDistance);
	if (!ProjectToNavigation(Candidate))
	{
		// That side is off the navmesh: fight from the front instead (and re-pick a side next time).
		FlankOrder = INDEX_NONE;
		return false;
	}
	FlankLocation = Candidate;

	// The flank is made once round the side; from there it fights normally.
	bOutReached = OutbreakEnemyTactics::HasFlanked(TargetLocation, Assignment.PinLocation, ControlledPawn->GetActorLocation(), Tactics.FlankAngle);
	return true;
}

void AOutbreakEnemyAIController::RequestCoverQueryIfNeeded(double Now)
{
	const FOutbreakEnemyTacticsSettings& Tactics = GetDefinition()->Tactics;
	if (!Tactics.CoverQuery || Tactics.CombatStyle != EOutbreakEnemyCombatStyle::Ranged || bCoverQueryPending || !GetPawn()
		|| Now - LastCoverQueryTime < Tactics.CoverQueryInterval)
	{
		return;
	}

	FEnvQueryRequest Request(Tactics.CoverQuery, GetPawn());
	bCoverQueryPending = Request.Execute(EEnvQueryRunMode::SingleResult, this, &ThisClass::HandleCoverQueryResult) != INDEX_NONE;
	LastCoverQueryTime = Now;
}

void AOutbreakEnemyAIController::HandleCoverQueryResult(TSharedPtr<FEnvQueryResult> Result)
{
	bCoverQueryPending = false;
	if (CurrentState != EOutbreakEnemyState::Combat)
	{
		return;
	}
	bHasCover = Result.IsValid() && Result->IsSuccessful() && Result->Items.Num() > 0;
	if (bHasCover)
	{
		CoverLocation = Result->GetItemAsLocation(0);
	}
}

FVector AOutbreakEnemyAIController::ComputeCombatMoveLocation(double Now)
{
	const UOutbreakEnemyDefinition* Definition = GetDefinition();
	const FOutbreakEnemyTacticsSettings& Tactics = Definition->Tactics;
	const FVector PawnLocation = GetPawn()->GetActorLocation();

	const AActor* Target = CurrentTarget.Get();
	const bool bVisible = IsActorVisible(Target);
	const FVector TargetLocation = Target && bVisible ? Target->GetActorLocation() : LastSeenTargetLocation;

	// Cover that no longer gives a view of the target is dropped, and a new one sought.
	if (bHasCover && CombatMove == EOutbreakEnemyCombatMove::TakeCover
		&& FVector::Dist2D(PawnLocation, CoverLocation) <= Definition->PatrolAcceptanceRadius
		&& Now - LastOwnSightTime > Tactics.CoverRepositionTime)
	{
		bHasCover = false;
		LastCoverQueryTime = -1.0e9;
	}
	RequestCoverQueryIfNeeded(Now);

	FOutbreakEnemyCombatContext Context;
	Context.Style = Tactics.CombatStyle;
	Context.bTargetVisible = bVisible;
	Context.bHasCover = bHasCover;
	Context.bPreferCover = Tactics.bPreferCover;
	Context.bUnderFire = Now - LastAttackTime <= Tactics.UnderFireTime;
	Context.bReloading = WeaponComponent && WeaponComponent->IsReloading();
	bool bFlankReached = false;
	Context.bFlankRole = UpdateFlank(TargetLocation, bFlankReached);
	Context.bFlankReached = bFlankReached;

	CombatMove = OutbreakEnemyTactics::SelectCombatMove(Context);

	FVector Destination;
	switch (CombatMove)
	{
	case EOutbreakEnemyCombatMove::Flank:
		Destination = FlankLocation;
		break;
	case EOutbreakEnemyCombatMove::TakeCover:
		Destination = CoverLocation;
		break;
	case EOutbreakEnemyCombatMove::Charge:
	case EOutbreakEnemyCombatMove::Engage:
	default:
		Destination = OutbreakEnemyTactics::ComputeEngagePosition(Tactics.CombatStyle, PawnLocation, TargetLocation, bVisible,
			Definition->MinCombatRange, Definition->PreferredCombatRange, Definition->MaxCombatRange);
		break;
	}
	return OutbreakEnemyTactics::ClampToLeash(Destination, HomeLocation, Tactics.LeashDistance);
}

bool AOutbreakEnemyAIController::GetCoverThreatLocation(FVector& OutLocation) const
{
	if (const AActor* Target = CurrentTarget.Get())
	{
		OutLocation = IsActorVisible(Target) ? Target->GetActorLocation() : LastSeenTargetLocation;
		return true;
	}
	if (const AActor* Attacker = LastAttacker.Get())
	{
		OutLocation = Attacker->GetActorLocation();
		return true;
	}
	return false;
}

bool AOutbreakEnemyAIController::GetCoverAnchorLocation(FVector& OutLocation) const
{
	if (FlankOrder != INDEX_NONE)
	{
		OutLocation = FlankLocation;
		return true;
	}
	return false;
}

void AOutbreakEnemyAIController::WriteBlackboard() const
{
	UBlackboardComponent* BlackboardComponent = GetBlackboardComponent();
	if (!BlackboardComponent)
	{
		return;
	}

	BlackboardComponent->SetValueAsEnum(OutbreakBlackboardKeys::EnemyState, static_cast<uint8>(CurrentState));
	BlackboardComponent->SetValueAsVector(OutbreakBlackboardKeys::MoveLocation, MoveLocation);
	BlackboardComponent->SetValueAsObject(OutbreakBlackboardKeys::TargetActor, CurrentTarget.Get());
	BlackboardComponent->SetValueAsEnum(OutbreakBlackboardKeys::CombatMove, static_cast<uint8>(CombatMove));
}

void AOutbreakEnemyAIController::DrawDebug() const
{
#if ENABLE_DRAW_DEBUG
	const UWorld* World = GetWorld();
	const APawn* ControlledPawn = GetPawn();
	if (CVarDebugEnemies.GetValueOnGameThread() == 0 || !World || !ControlledPawn)
	{
		return;
	}

	const UOutbreakEnemyDefinition* Definition = GetDefinition();
	const float Duration = 0.3f;

	FColor StateColor = FColor::Green;
	switch (CurrentState)
	{
	case EOutbreakEnemyState::Patrol:        StateColor = FColor::Cyan; break;
	case EOutbreakEnemyState::Suspicious:    StateColor = FColor::Yellow; break;
	case EOutbreakEnemyState::Investigating: StateColor = FColor::Orange; break;
	case EOutbreakEnemyState::Combat:        StateColor = FColor::Red; break;
	case EOutbreakEnemyState::Searching:     StateColor = FColor::Magenta; break;
	case EOutbreakEnemyState::Retreating:    StateColor = FColor::Purple; break;
	default: break;
	}

	FVector EyesLocation;
	FRotator EyesRotation;
	ControlledPawn->GetActorEyesViewPoint(EyesLocation, EyesRotation);
	const float HalfAngle = FMath::DegreesToRadians(Definition->PeripheralVisionHalfAngle);
	DrawDebugCone(World, EyesLocation, EyesRotation.Vector(), Definition->SightRadius, HalfAngle, HalfAngle, 16, StateColor, false, Duration);

	const FString MoveName = CombatMove == EOutbreakEnemyCombatMove::None ? FString()
		: StaticEnum<EOutbreakEnemyCombatMove>()->GetNameStringByValue(static_cast<int64>(CombatMove));
	const FString Label = FString::Printf(TEXT("%s %s  suspicion %d%%  %s"),
		*OutbreakEnemyBehavior::GetStateDisplayName(CurrentState).ToString(), *MoveName, FMath::RoundToInt(Suspicion * 100.0f),
		*GetNameSafe(CurrentTarget.Get()));
	DrawDebugString(World, ControlledPawn->GetActorLocation() + FVector(0.0f, 0.0f, 120.0f), Label, nullptr, StateColor, Duration);
	DrawDebugLine(World, ControlledPawn->GetActorLocation(), MoveLocation, FColor::White, false, Duration);

	for (const TPair<TWeakObjectPtr<AActor>, float>& Entry : Detection)
	{
		if (const AActor* Actor = Entry.Key.Get())
		{
			DrawDebugString(World, Actor->GetActorLocation() + FVector(0.0f, 0.0f, 100.0f),
				FString::Printf(TEXT("seen %d%%"), FMath::RoundToInt(Entry.Value * 100.0f)), nullptr, StateColor, Duration);
		}
	}
	if (bHasStimulus)
	{
		DrawDebugSphere(World, StimulusLocation, 50.0f, 8, FColor::Orange, false, Duration);
	}
	if (bHasCover && CurrentState == EOutbreakEnemyState::Combat)
	{
		DrawDebugBox(World, CoverLocation, FVector(30.0f), FColor::Blue, false, Duration);
	}
	if (FlankOrder != INDEX_NONE)
	{
		DrawDebugSphere(World, FlankLocation, 60.0f, 8, FColor::Yellow, false, Duration);
	}
	if (Definition->Tactics.LeashDistance > 0.0f)
	{
		DrawDebugCircle(World, HomeLocation, Definition->Tactics.LeashDistance, 32, FColor::Silver, false, Duration,
			0, 0.0f, FVector(1.0f, 0.0f, 0.0f), FVector(0.0f, 1.0f, 0.0f), false);
	}
	if (CurrentState == EOutbreakEnemyState::Searching)
	{
		DrawDebugCircle(World, SearchCenter, Definition->SearchRadius, 24, FColor::Magenta, false, Duration,
			0, 0.0f, FVector(1.0f, 0.0f, 0.0f), FVector(0.0f, 1.0f, 0.0f), false);
	}
#endif
}
