#include "AI/OutbreakEnemyAIController.h"

#include "AI/OutbreakBlackboardKeys.h"
#include "AI/OutbreakEnemyDefinition.h"
#include "AI/OutbreakPatrolRoute.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BrainComponent.h"
#include "Characters/OutbreakEnemyCharacter.h"
#include "Combat/OutbreakTeam.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "Navigation/CrowdFollowingComponent.h"
#include "Outbreak.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Hearing.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Perception/AISense_Hearing.h"
#include "Perception/AISense_Sight.h"
#include "Weapons/OutbreakWeaponComponent.h"

namespace
{
	TAutoConsoleVariable<int32> CVarDebugEnemies(
		TEXT("outbreak.AI.DebugEnemies"), 0,
		TEXT("1 = draw enemy sight range, hearing range, state, target and destination at each decision update."),
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

void AOutbreakEnemyAIController::ReportAlert(const FVector& Location)
{
	bHasAlert = true;
	AlertLocation = Location;
	AlertTime = GetTimeSeconds();
}

void AOutbreakEnemyAIController::HandleTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
	// Sight is polled at each decision update; hearing is an event that raises an alert.
	if (Stimulus.Type == UAISense::GetSenseID<UAISense_Hearing>() && Stimulus.WasSuccessfullySensed())
	{
		ReportAlert(Stimulus.StimulusLocation);
	}
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

	// Hit by someone it cannot see: turn and look where the attack came from.
	if (!IsActorVisible(Attacker))
	{
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
	bHasAlert = false;
	ClearFocus(EAIFocusPriority::Gameplay);
	StopMovement();
	if (BrainComponent)
	{
		BrainComponent->StopLogic(TEXT("Downed"));
	}
}

// --- Perception and targeting --------------------------------------------------------------------

bool AOutbreakEnemyAIController::IsActorVisible(const AActor* Actor) const
{
	return Actor && VisibleHostiles.ContainsByPredicate(
		[Actor](const TWeakObjectPtr<AActor>& Entry) { return Entry.Get() == Actor; });
}

void AOutbreakEnemyAIController::UpdateTargets()
{
	const TWeakObjectPtr<AActor> PreviousTarget = CurrentTarget;
	VisibleHostiles.Reset();
	CurrentTarget = nullptr;

	const APawn* ControlledPawn = GetPawn();
	if (!ControlledPawn)
	{
		return;
	}

	TArray<AActor*> PerceivedActors;
	GetPerceptionComponent()->GetCurrentlyPerceivedActors(UAISense_Sight::StaticClass(), PerceivedActors);

	const UOutbreakEnemyDefinition* Definition = GetDefinition();
	FOutbreakTargetScoreWeights Weights;
	Weights.DistancePenaltyPerMeter = Definition->DistancePenaltyPerMeter;
	Weights.RecentAttackerBonus = Definition->RecentAttackerBonus;
	Weights.PlayerBonus = Definition->PlayerBonus;

	const bool bAttackerIsRecent = GetTimeSeconds() - LastAttackTime <= Definition->AttackerMemory;
	float BestScore = TNumericLimits<float>::Lowest();

	for (AActor* Actor : PerceivedActors)
	{
		if (!IsValid(Actor) || !OutbreakTeam::IsHostile(ControlledPawn, Actor))
		{
			continue;
		}
		const UOutbreakHealthComponent* ActorHealth = Actor->FindComponentByClass<UOutbreakHealthComponent>();
		if (ActorHealth && !ActorHealth->IsAlive())
		{
			continue;
		}

		VisibleHostiles.Add(Actor);

		const APawn* ActorPawn = Cast<APawn>(Actor);
		const float Score = OutbreakEnemyTargeting::ScoreTarget(
			FVector::Dist(ControlledPawn->GetActorLocation(), Actor->GetActorLocation()),
			bAttackerIsRecent && LastAttacker.Get() == Actor,
			ActorPawn && ActorPawn->IsPlayerControlled(),
			Weights);
		if (Score > BestScore)
		{
			BestScore = Score;
			CurrentTarget = Actor;
		}
	}

	if (CurrentTarget.IsValid())
	{
		LastSeenTargetLocation = CurrentTarget->GetActorLocation();
		return;
	}

	// Lost sight of a living target: go to where it was last seen.
	if (const AActor* Lost = PreviousTarget.Get())
	{
		const UOutbreakHealthComponent* LostHealth = Lost->FindComponentByClass<UOutbreakHealthComponent>();
		if (!LostHealth || LostHealth->IsAlive())
		{
			ReportAlert(LastSeenTargetLocation);
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

	UpdateTargets();

	const UOutbreakEnemyDefinition* Definition = GetDefinition();
	const AOutbreakPatrolRoute* Route = Enemy ? Enemy->GetPatrolRoute() : nullptr;

	FOutbreakEnemyStateContext Context;
	Context.bHasVisibleTarget = CurrentTarget.IsValid();
	Context.bHasAlert = bHasAlert;
	Context.AlertElapsed = static_cast<float>(GetTimeSeconds() - AlertTime);
	Context.AlertDuration = Definition->AlertDuration;
	Context.bHasPatrolRoute = Route && Route->GetNumPoints() > 0;

	const EOutbreakEnemyState NewState = OutbreakEnemyBehavior::SelectState(Context);

	// A target in sight supersedes the alert; an expired alert is dropped.
	if (NewState == EOutbreakEnemyState::Combat || (bHasAlert && Context.AlertElapsed >= Context.AlertDuration))
	{
		bHasAlert = false;
	}
	if (NewState != CurrentState && NewState == EOutbreakEnemyState::Patrol)
	{
		PatrolArrivalTime = -1.0;
	}
	CurrentState = NewState;
	MoveLocation = ComputeMoveLocation();

	if (Enemy)
	{
		// Alert and combat at sprint speed; idle and patrol at walk speed.
		const bool bHurry = NewState == EOutbreakEnemyState::Combat || NewState == EOutbreakEnemyState::Alerted;
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

FVector AOutbreakEnemyAIController::ComputeMoveLocation()
{
	const UOutbreakEnemyDefinition* Definition = GetDefinition();
	const FVector PawnLocation = GetPawn()->GetActorLocation();

	switch (CurrentState)
	{
	case EOutbreakEnemyState::Patrol:
		return UpdatePatrol();

	case EOutbreakEnemyState::Alerted:
		return AlertLocation;

	case EOutbreakEnemyState::Combat:
		if (const AActor* Target = CurrentTarget.Get())
		{
			return OutbreakEnemyTactics::ComputeCombatPosition(PawnLocation, Target->GetActorLocation(), IsActorVisible(Target),
				Definition->MinCombatRange, Definition->PreferredCombatRange, Definition->MaxCombatRange);
		}
		return PawnLocation;

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
	case EOutbreakEnemyState::Patrol:  StateColor = FColor::Cyan; break;
	case EOutbreakEnemyState::Alerted: StateColor = FColor::Orange; break;
	case EOutbreakEnemyState::Combat:  StateColor = FColor::Red; break;
	default: break;
	}

	FVector EyesLocation;
	FRotator EyesRotation;
	ControlledPawn->GetActorEyesViewPoint(EyesLocation, EyesRotation);
	const float HalfAngle = FMath::DegreesToRadians(Definition->PeripheralVisionHalfAngle);
	DrawDebugCone(World, EyesLocation, EyesRotation.Vector(), Definition->SightRadius, HalfAngle, HalfAngle, 16, StateColor, false, Duration);

	if (Definition->HearingRange > 0.0f)
	{
		DrawDebugCircle(World, ControlledPawn->GetActorLocation(), Definition->HearingRange, 32, FColor::Blue, false, Duration,
			0, 0.0f, FVector(1.0f, 0.0f, 0.0f), FVector(0.0f, 1.0f, 0.0f), false);
	}

	const FString Label = FString::Printf(TEXT("%s  %s"),
		*OutbreakEnemyBehavior::GetStateDisplayName(CurrentState).ToString(), *GetNameSafe(CurrentTarget.Get()));
	DrawDebugString(World, ControlledPawn->GetActorLocation() + FVector(0.0f, 0.0f, 120.0f), Label, nullptr, StateColor, Duration);
	DrawDebugLine(World, ControlledPawn->GetActorLocation(), MoveLocation, FColor::White, false, Duration);

	if (bHasAlert)
	{
		DrawDebugSphere(World, AlertLocation, 50.0f, 8, FColor::Orange, false, Duration);
	}
#endif
}
