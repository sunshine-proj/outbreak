#include "AI/OutbreakSquadAIController.h"

#include "AI/OutbreakBlackboardKeys.h"
#include "AI/OutbreakSquadPositioning.h"
#include "AI/OutbreakTeammateAIConfig.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Combat/OutbreakHealthComponent.h"
#include "Combat/OutbreakTeam.h"
#include "Navigation/CrowdFollowingComponent.h"
#include "Outbreak.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Perception/AISense_Sight.h"
#include "Squad/OutbreakSquadComponent.h"
#include "Squad/OutbreakSquadMemberComponent.h"
#include "Squad/OutbreakSquadRoleDefinition.h"
#include "Squad/OutbreakTeammateCharacter.h"
#include "Weapons/OutbreakWeaponComponent.h"

AOutbreakSquadAIController::AOutbreakSquadAIController(const FObjectInitializer& ObjectInitializer)
	// Detour crowd path following: teammates steer around each other instead of colliding.
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UCrowdFollowingComponent>(TEXT("PathFollowingComponent")))
{
	SetPerceptionComponent(*CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("PerceptionComponent")));

	SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));
	SightConfig->DetectionByAffiliation.bDetectEnemies = true;
	SightConfig->DetectionByAffiliation.bDetectNeutrals = false;
	SightConfig->DetectionByAffiliation.bDetectFriendlies = false;

	GetPerceptionComponent()->ConfigureSense(*SightConfig);
	GetPerceptionComponent()->SetDominantSense(SightConfig->GetSenseImplementation());

	SetGenericTeamId(OutbreakTeam::ToGenericTeamId(EOutbreakTeam::Player));
}

const UOutbreakTeammateAIConfig* AOutbreakSquadAIController::GetAIConfig() const
{
	return AIConfig ? AIConfig.Get() : GetDefault<UOutbreakTeammateAIConfig>();
}

void AOutbreakSquadAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	if (!InPawn)
	{
		return;
	}

	SquadMember = InPawn->FindComponentByClass<UOutbreakSquadMemberComponent>();
	WeaponComponent = InPawn->FindComponentByClass<UOutbreakWeaponComponent>();
	HealthComponent = InPawn->FindComponentByClass<UOutbreakHealthComponent>();

	const AOutbreakTeammateCharacter* Teammate = Cast<AOutbreakTeammateCharacter>(InPawn);
	const UOutbreakTeammateAIConfig* PawnConfig = Teammate ? Teammate->GetTeammateAIConfig() : nullptr;
	// Data assets are immutable at runtime; the pointer is non-const only to be a UPROPERTY.
	AIConfig = const_cast<UOutbreakTeammateAIConfig*>(PawnConfig ? PawnConfig : GetDefault<UOutbreakTeammateAIConfig>());

	SetGenericTeamId(FGenericTeamId::GetTeamIdentifier(InPawn));
	ApplyPerceptionConfig();

	if (SquadMember)
	{
		SquadMember->OnCommandChanged.AddUniqueDynamic(this, &ThisClass::HandleCommandChanged);
	}

	MoveLocation = InPawn->GetActorLocation();

	if (AIConfig->BehaviorTree)
	{
		RunBehaviorTree(AIConfig->BehaviorTree);
	}
	else
	{
		UE_LOG(LogOutbreak, Warning, TEXT("%s: no BehaviorTree in teammate AI config %s; %s will not act."),
			*GetName(), *GetNameSafe(AIConfig), *InPawn->GetName());
	}
}

void AOutbreakSquadAIController::OnUnPossess()
{
	if (SquadMember)
	{
		SquadMember->OnCommandChanged.RemoveDynamic(this, &ThisClass::HandleCommandChanged);
	}
	SquadMember = nullptr;
	WeaponComponent = nullptr;
	HealthComponent = nullptr;

	Super::OnUnPossess();
}

void AOutbreakSquadAIController::ApplyPerceptionConfig()
{
	const UOutbreakTeammateAIConfig* Config = GetAIConfig();
	SightConfig->SightRadius = Config->SightRadius;
	SightConfig->LoseSightRadius = FMath::Max(Config->LoseSightRadius, Config->SightRadius);
	SightConfig->PeripheralVisionAngleDegrees = Config->PeripheralVisionHalfAngle;
	SightConfig->SetMaxAge(Config->SightMaxAge);
	GetPerceptionComponent()->ConfigureSense(*SightConfig);
}

void AOutbreakSquadAIController::HandleCommandChanged(UOutbreakSquadMemberComponent* Member)
{
	// React to a new order at once instead of waiting for the next service tick.
	UpdateBehavior();
}

// --- Perception and target tracking --------------------------------------------------------------

void AOutbreakSquadAIController::UpdateThreat()
{
	VisibleHostiles.Reset();
	CurrentThreat = nullptr;

	const APawn* ControlledPawn = GetPawn();
	if (!ControlledPawn || !GetPerceptionComponent())
	{
		return;
	}

	TArray<AActor*> PerceivedActors;
	GetPerceptionComponent()->GetCurrentlyPerceivedActors(UAISense_Sight::StaticClass(), PerceivedActors);

	float BestDistanceSquared = TNumericLimits<float>::Max();
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
		const float DistanceSquared = FVector::DistSquared(ControlledPawn->GetActorLocation(), Actor->GetActorLocation());
		if (DistanceSquared < BestDistanceSquared)
		{
			BestDistanceSquared = DistanceSquared;
			CurrentThreat = Actor;
		}
	}
}

bool AOutbreakSquadAIController::IsActorVisible(const AActor* Actor) const
{
	return Actor && VisibleHostiles.ContainsByPredicate(
		[Actor](const TWeakObjectPtr<AActor>& Entry) { return Entry.Get() == Actor; });
}

AActor* AOutbreakSquadAIController::GetCombatTarget() const
{
	if (CurrentDecision.Behavior == EOutbreakTeammateBehavior::AttackTarget && SquadMember)
	{
		AActor* OrderedTarget = SquadMember->GetCurrentCommand().TargetActor;
		if (IsValid(OrderedTarget))
		{
			return OrderedTarget;
		}
	}
	return CurrentThreat.Get();
}

float AOutbreakSquadAIController::GetAimTolerance() const
{
	return GetAIConfig()->AimTolerance;
}

float AOutbreakSquadAIController::GetIdleReloadFraction() const
{
	return GetAIConfig()->IdleReloadFraction;
}

float AOutbreakSquadAIController::GetEngagementRange() const
{
	const float MaxRange = GetAIConfig()->MaxEngagementRange;
	const UOutbreakSquadRoleDefinition* Role = SquadMember ? SquadMember->GetRole() : nullptr;
	return Role && Role->PreferredEngagementRange > 0.0f ? FMath::Min(Role->PreferredEngagementRange, MaxRange) : MaxRange;
}

// --- Decision ------------------------------------------------------------------------------------

UOutbreakSquadComponent* AOutbreakSquadAIController::GetSquad() const
{
	return SquadMember ? SquadMember->GetSquad() : nullptr;
}

APawn* AOutbreakSquadAIController::GetPlayerPawn() const
{
	const UOutbreakSquadComponent* Squad = GetSquad();
	const UOutbreakSquadMemberComponent* PlayerMember = Squad ? Squad->GetPlayerMember() : nullptr;
	return PlayerMember ? Cast<APawn>(PlayerMember->GetOwner()) : nullptr;
}

void AOutbreakSquadAIController::UpdateOrderProgress()
{
	const APawn* ControlledPawn = GetPawn();
	if (!SquadMember || !ControlledPawn || SquadMember->GetCommandStatus() != EOutbreakSquadCommandStatus::Active)
	{
		return;
	}

	const UOutbreakTeammateAIConfig* Config = GetAIConfig();
	const FOutbreakSquadCommand Command = SquadMember->GetCurrentCommand();
	const FVector PawnLocation = ControlledPawn->GetActorLocation();

	switch (Command.Type)
	{
	case EOutbreakSquadCommandType::Move:
		if (FVector::Dist2D(PawnLocation, Command.TargetLocation) <= Config->MoveArrivalRadius)
		{
			// Arrived: the order is done, and the teammate holds the destination.
			SquadMember->CompleteCommand();
			SquadMember->AssignCommand(FOutbreakSquadCommand::MakeHold(Command.TargetLocation));
		}
		break;

	case EOutbreakSquadCommandType::Regroup:
		if (!GetPlayerPawn() || FVector::Dist2D(PawnLocation, ComputeFollowLocation()) <= Config->RegroupArrivalRadius)
		{
			SquadMember->CompleteCommand();
		}
		break;

	case EOutbreakSquadCommandType::Attack:
	{
		const AActor* Target = Command.TargetActor;
		const UOutbreakHealthComponent* TargetHealth = IsValid(Target) ? Target->FindComponentByClass<UOutbreakHealthComponent>() : nullptr;
		if (!IsValid(Target) || (TargetHealth && !TargetHealth->IsAlive()))
		{
			SquadMember->CompleteCommand();
		}
		break;
	}

	default:
		// Follow and Hold last until replaced or cancelled.
		break;
	}
}

void AOutbreakSquadAIController::UpdateBehavior()
{
	if (bUpdatingBehavior)
	{
		return;
	}
	TGuardValue<bool> UpdatingGuard(bUpdatingBehavior, true);

	APawn* ControlledPawn = GetPawn();
	if (!ControlledPawn)
	{
		return;
	}

	// A downed teammate does nothing (recovery is Phase 10).
	if (HealthComponent && !HealthComponent->IsAlive())
	{
		CurrentDecision = FOutbreakBehaviorDecision();
		MoveLocation = ControlledPawn->GetActorLocation();
		VisibleHostiles.Reset();
		CurrentThreat = nullptr;
		if (SquadMember)
		{
			SquadMember->SetCurrentBehavior(CurrentDecision.Behavior);
		}
		WriteBlackboard();
		return;
	}

	UpdateThreat();
	UpdateOrderProgress();

	const UOutbreakTeammateAIConfig* Config = GetAIConfig();
	const APawn* PlayerPawn = GetPlayerPawn();

	FOutbreakBehaviorContext Context;
	Context.ActiveCommand = SquadMember ? SquadMember->GetActiveCommandType() : EOutbreakSquadCommandType::None;
	Context.bHasThreat = CurrentThreat.IsValid();
	Context.bLowHealth = HealthComponent && HealthComponent->GetHealth() < HealthComponent->GetMaxHealth() * Config->SurvivalHealthFraction;
	Context.bHasPlayer = PlayerPawn != nullptr;
	Context.DistanceToFollowLocation = PlayerPawn ? FVector::Dist2D(ControlledPawn->GetActorLocation(), ComputeFollowLocation()) : 0.0f;
	Context.bCurrentlyFollowing = CurrentDecision.Behavior == EOutbreakTeammateBehavior::FollowPlayer;
	Context.FollowStartDistance = Config->FollowStartDistance;
	Context.FollowStopDistance = Config->FollowStopDistance;

	const FOutbreakBehaviorDecision NewDecision = OutbreakTeammateBehavior::SelectBehavior(Context, Config->GetEffectivePriorityOrder());
	const bool bBehaviorChanged = NewDecision.Behavior != CurrentDecision.Behavior;
	CurrentDecision = NewDecision;
	MoveLocation = ComputeMoveLocation(NewDecision.Behavior, bBehaviorChanged);

	if (SquadMember)
	{
		SquadMember->SetCurrentBehavior(NewDecision.Behavior);
	}
	WriteBlackboard();
}

// --- Positioning ---------------------------------------------------------------------------------

FVector AOutbreakSquadAIController::ComputeFollowLocation() const
{
	const APawn* ControlledPawn = GetPawn();
	const APawn* PlayerPawn = GetPlayerPawn();
	const UOutbreakSquadComponent* Squad = GetSquad();
	if (!PlayerPawn || !Squad)
	{
		return ControlledPawn ? ControlledPawn->GetActorLocation() : FVector::ZeroVector;
	}

	// Slots are shared among living teammates so the formation closes up when someone falls.
	TArray<UOutbreakSquadMemberComponent*> LivingTeammates = Squad->GetTeammates();
	LivingTeammates.RemoveAll([](const UOutbreakSquadMemberComponent* Member)
	{
		return Member->GetStatus() == EOutbreakSquadMemberStatus::Dead;
	});

	const int32 SlotIndex = FMath::Max(0, LivingTeammates.IndexOfByKey(SquadMember.Get()));
	const UOutbreakTeammateAIConfig* Config = GetAIConfig();
	return OutbreakSquadPositioning::ComputeFollowLocation(PlayerPawn->GetActorLocation(), PlayerPawn->GetActorRotation(),
		SlotIndex, FMath::Max(1, LivingTeammates.Num()), Config->FollowDistance, Config->FollowSpacing, Config->FollowRowWidth);
}

FVector AOutbreakSquadAIController::ComputeMoveLocation(EOutbreakTeammateBehavior Behavior, bool bBehaviorChanged) const
{
	const APawn* ControlledPawn = GetPawn();
	const FVector PawnLocation = ControlledPawn->GetActorLocation();
	// Stopping keeps the previous stop point while the behaviour is unchanged, so the Behavior Tree
	// is not asked to re-path to a slightly different spot every update.
	const FVector StopLocation = bBehaviorChanged ? PawnLocation : MoveLocation;

	switch (Behavior)
	{
	case EOutbreakTeammateBehavior::FollowPlayer:
	case EOutbreakTeammateBehavior::Regroup:
	case EOutbreakTeammateBehavior::SeekSafety:
		return ComputeFollowLocation();

	case EOutbreakTeammateBehavior::HoldPosition:
	case EOutbreakTeammateBehavior::MoveToLocation:
		return SquadMember ? SquadMember->GetCurrentCommand().TargetLocation : PawnLocation;

	case EOutbreakTeammateBehavior::AttackTarget:
	{
		const AActor* Target = GetCombatTarget();
		if (!Target)
		{
			return StopLocation;
		}
		const float Range = GetEngagementRange();
		const FVector TargetLocation = Target->GetActorLocation();
		const float Distance = FVector::Dist(PawnLocation, TargetLocation);
		if (IsActorVisible(Target) && Distance <= Range)
		{
			return FVector::Dist(StopLocation, PawnLocation) > 100.0f ? PawnLocation : StopLocation;
		}
		// Approach to well inside engagement range along the line from the target.
		const FVector FromTarget = (PawnLocation - TargetLocation).GetSafeNormal2D();
		return TargetLocation + FromTarget * FMath::Min(Range * 0.75f, Distance);
	}

	case EOutbreakTeammateBehavior::EngageThreat:
	case EOutbreakTeammateBehavior::Idle:
	default:
		return StopLocation;
	}
}

void AOutbreakSquadAIController::WriteBlackboard() const
{
	UBlackboardComponent* BlackboardComponent = GetBlackboardComponent();
	if (!BlackboardComponent)
	{
		return;
	}

	BlackboardComponent->SetValueAsEnum(OutbreakBlackboardKeys::Behavior, static_cast<uint8>(CurrentDecision.Behavior));
	BlackboardComponent->SetValueAsVector(OutbreakBlackboardKeys::MoveLocation, MoveLocation);
	BlackboardComponent->SetValueAsObject(OutbreakBlackboardKeys::TargetActor, GetCombatTarget());
	BlackboardComponent->SetValueAsObject(OutbreakBlackboardKeys::PlayerActor, GetPlayerPawn());
}
