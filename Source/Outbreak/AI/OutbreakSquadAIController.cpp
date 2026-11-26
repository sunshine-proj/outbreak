#include "AI/OutbreakSquadAIController.h"

#include "AI/OutbreakBlackboardKeys.h"
#include "AI/OutbreakEnemyAIController.h"
#include "AI/OutbreakMedicalAssist.h"
#include "Combat/OutbreakMedicalComponent.h"
#include "Combat/OutbreakMedicalSettings.h"
#include "Inventory/OutbreakInventoryComponent.h"
#include "Inventory/OutbreakItemDefinition.h"
#include "Characters/OutbreakCharacter.h"
#include "Stealth/OutbreakNoiseEmitterComponent.h"
#include "AI/OutbreakSquadPositioning.h"
#include "AI/OutbreakTeammateAIConfig.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Combat/OutbreakHealthComponent.h"
#include "Combat/OutbreakTeam.h"
#include "EnvironmentQuery/EnvQuery.h"
#include "EnvironmentQuery/EnvQueryManager.h"
#include "NavigationSystem.h"
#include "Navigation/CrowdFollowingComponent.h"
#include "Outbreak.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Perception/AISense_Sight.h"
#include "Squad/OutbreakSquadComponent.h"
#include "Squad/OutbreakSquadMemberComponent.h"
#include "Squad/OutbreakSquadRoleDefinition.h"
#include "Squad/OutbreakSquadSettings.h"
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
	InPawn->OnTakeAnyDamage.AddUniqueDynamic(this, &ThisClass::HandleTakeAnyDamage);

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
	if (APawn* OldPawn = GetPawn())
	{
		OldPawn->OnTakeAnyDamage.RemoveDynamic(this, &ThisClass::HandleTakeAnyDamage);
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

	if (const AActor* Threat = CurrentThreat.Get())
	{
		LastThreatLocation = Threat->GetActorLocation();
		LastThreatTime = GetTimeSeconds();
	}
}

double AOutbreakSquadAIController::GetTimeSeconds() const
{
	const UWorld* World = GetWorld();
	return World ? World->GetTimeSeconds() : 0.0;
}

bool AOutbreakSquadAIController::HasRememberedThreat() const
{
	return GetTimeSeconds() - LastThreatTime <= GetAIConfig()->ThreatMemory;
}

bool AOutbreakSquadAIController::IsUnderFire() const
{
	return GetTimeSeconds() - LastDamageTime <= GetAIConfig()->UnderFireMemory;
}

void AOutbreakSquadAIController::HandleTakeAnyDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType,
	AController* InstigatedBy, AActor* DamageCauser)
{
	LastDamageTime = GetTimeSeconds();

	// An unseen shooter still tells us roughly where the danger is.
	const AActor* Attacker = InstigatedBy && InstigatedBy->GetPawn() ? InstigatedBy->GetPawn() : DamageCauser;
	if (Attacker && !CurrentThreat.IsValid() && OutbreakTeam::IsHostile(GetPawn(), Attacker))
	{
		LastThreatLocation = Attacker->GetActorLocation();
		LastThreatTime = LastDamageTime;
	}
}

bool AOutbreakSquadAIController::GetCoverThreatLocation(FVector& OutLocation) const
{
	if (const AActor* Threat = CurrentThreat.Get())
	{
		OutLocation = Threat->GetActorLocation();
		return true;
	}
	if (HasRememberedThreat())
	{
		OutLocation = LastThreatLocation;
		return true;
	}
	return false;
}

bool AOutbreakSquadAIController::IsFireAllowed() const
{
	const UOutbreakSquadComponent* Squad = GetSquad();
	if (!Squad || Squad->AreWeaponsFree() || IsUnderFire())
	{
		return true;
	}
	return SquadMember && SquadMember->GetActiveCommandType() == EOutbreakSquadCommandType::Attack;
}

void AOutbreakSquadAIController::UpdateStealth(bool& bOutShouldHide)
{
	bOutShouldHide = false;
	AOutbreakCharacter* Soldier = Cast<AOutbreakCharacter>(GetPawn());
	UOutbreakSquadComponent* Squad = GetSquad();
	if (!Soldier)
	{
		return;
	}

	const UOutbreakTeammateAIConfig* Config = GetAIConfig();
	const bool bStealth = Squad && Squad->IsStealthMode();
	NoticedLevel = AOutbreakEnemyAIController::GetHighestDetectionOf(GetWorld(), Soldier);

	// Detected outright, or shot at: the squad's cover is blown.
	if (bStealth && (NoticedLevel >= 1.0f || IsUnderFire()))
	{
		Squad->MarkCompromised();
	}
	const bool bQuiet = bStealth && !Squad->IsCompromised();

	// Quieter movement: crouch and soften footsteps while quiet.
	if (bQuiet && !Soldier->IsCrouchedOrCrouchRequested())
	{
		Soldier->BeginCrouch();
		bCrouchedForStealth = true;
	}
	else if (!bQuiet && bCrouchedForStealth)
	{
		Soldier->EndCrouch();
		bCrouchedForStealth = false;
	}
	if (UOutbreakNoiseEmitterComponent* Noise = Soldier->GetNoiseEmitterComponent())
	{
		Noise->SetLoudnessMultiplier(bQuiet ? Config->StealthNoiseMultiplier : 1.0f);
	}

	bOutShouldHide = bQuiet && NoticedLevel >= Config->StealthHideThreshold;
}

bool AOutbreakSquadAIController::GetCoverAnchorLocation(FVector& OutLocation) const
{
	const EOutbreakSquadCommandType Command = SquadMember ? SquadMember->GetActiveCommandType() : EOutbreakSquadCommandType::None;
	if (Command == EOutbreakSquadCommandType::Defend || Command == EOutbreakSquadCommandType::Cover)
	{
		OutLocation = SquadMember->GetCurrentCommand().TargetLocation;
		return true;
	}
	return false;
}

// --- Cover queries -------------------------------------------------------------------------------

void AOutbreakSquadAIController::RequestCoverQueryIfNeeded(bool bNeedCover)
{
	const UOutbreakTeammateAIConfig* Config = GetAIConfig();
	if (!bNeedCover || !HasRememberedThreat())
	{
		// Cover is relative to a threat; without one, old results are meaningless.
		if (!HasRememberedThreat())
		{
			bHasCover = false;
		}
		return;
	}

	const double Now = GetTimeSeconds();
	if (!Config->CoverQuery || bCoverQueryPending || Now - LastCoverQueryTime < Config->CoverQueryInterval || !GetPawn())
	{
		return;
	}

	FEnvQueryRequest Request(Config->CoverQuery, GetPawn());
	bCoverQueryPending = Request.Execute(EEnvQueryRunMode::SingleResult, this, &ThisClass::HandleCoverQueryResult) != INDEX_NONE;
	LastCoverQueryTime = Now;
}

void AOutbreakSquadAIController::HandleCoverQueryResult(TSharedPtr<FEnvQueryResult> Result)
{
	bCoverQueryPending = false;
	bHasCover = Result.IsValid() && Result->IsSuccessful() && Result->Items.Num() > 0;
	if (bHasCover)
	{
		CoverLocation = Result->GetItemAsLocation(0);
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

	case EOutbreakSquadCommandType::Retreat:
		if (bHasRetreatLocation && FVector::Dist2D(PawnLocation, RetreatLocation) <= Config->MoveArrivalRadius)
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
		// Follow, Hold, Defend and Cover last until replaced or cancelled.
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

	// A downed teammate does nothing until revived.
	if (HealthComponent && !HealthComponent->IsAlive())
	{
		AssistPatient = nullptr;
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
	Context.bUnderFire = IsUnderFire();

	// Cover is wanted when shot at, when hurt, or when ordered to defend or take cover.
	const bool bNeedCover = Context.bUnderFire || Context.bLowHealth
		|| Context.ActiveCommand == EOutbreakSquadCommandType::Cover
		|| Context.ActiveCommand == EOutbreakSquadCommandType::Defend;
	RequestCoverQueryIfNeeded(bNeedCover);
	Context.bHasCover = bHasCover;
	Context.DistanceToFollowLocation = PlayerPawn ? FVector::Dist2D(ControlledPawn->GetActorLocation(), ComputeFollowLocation()) : 0.0f;
	Context.bCurrentlyFollowing = CurrentDecision.Behavior == EOutbreakTeammateBehavior::FollowPlayer;
	Context.FollowStartDistance = Config->FollowStartDistance;
	Context.FollowStopDistance = Config->FollowStopDistance;

	UpdateSelfHeal();
	UpdateAssistPatient(Context);

	bool bShouldHide = false;
	UpdateStealth(bShouldHide);

	FOutbreakBehaviorDecision NewDecision = OutbreakTeammateBehavior::SelectBehavior(Context, Config->GetEffectivePriorityOrder());
	NewDecision.Behavior = OutbreakTeammateBehavior::ApplyStealthOverride(NewDecision.Behavior, bShouldHide);
	const bool bBehaviorChanged = NewDecision.Behavior != CurrentDecision.Behavior;
	CurrentDecision = NewDecision;

	// Stop a revive this teammate started if it is now doing something else.
	UOutbreakMedicalComponent* Medical = ControlledPawn->FindComponentByClass<UOutbreakMedicalComponent>();
	if (Medical && Medical->IsReviving() && NewDecision.Behavior != EOutbreakTeammateBehavior::ReviveMember)
	{
		Medical->CancelRevive();
	}

	MoveLocation = ComputeMoveLocation(NewDecision.Behavior, bBehaviorChanged);

	if (SquadMember)
	{
		SquadMember->SetCurrentBehavior(NewDecision.Behavior);
	}
	WriteBlackboard();
}

// --- Medical assistance --------------------------------------------------------------------------

void AOutbreakSquadAIController::UpdateSelfHeal()
{
	const UOutbreakTeammateAIConfig* Config = GetAIConfig();
	APawn* ControlledPawn = GetPawn();
	UOutbreakInventoryComponent* Inventory = ControlledPawn ? ControlledPawn->FindComponentByClass<UOutbreakInventoryComponent>() : nullptr;
	if (!Inventory || !HealthComponent || Config->SelfHealHealthFraction <= 0.0f || IsUnderFire()
		|| HealthComponent->GetHealthFraction() > Config->SelfHealHealthFraction)
	{
		return;
	}
	Inventory->UseMedicalOn(ControlledPawn);
}

void AOutbreakSquadAIController::UpdateAssistPatient(FOutbreakBehaviorContext& Context)
{
	AssistPatient = nullptr;
	const UOutbreakTeammateAIConfig* Config = GetAIConfig();
	const UOutbreakSquadComponent* Squad = GetSquad();
	const APawn* ControlledPawn = GetPawn();
	if (!Squad || !ControlledPawn || Config->MedicalAssistRadius <= 0.0f)
	{
		return;
	}

	const UOutbreakInventoryComponent* Inventory = ControlledPawn->FindComponentByClass<UOutbreakInventoryComponent>();
	const UOutbreakMedicalComponent* OwnMedical = ControlledPawn->FindComponentByClass<UOutbreakMedicalComponent>();

	// Other teammates who could help: alive AI teammates (the player decides for themself).
	TArray<const AActor*> OtherHelpers;
	for (const UOutbreakSquadMemberComponent* Member : Squad->GetTeammates())
	{
		const AActor* Owner = Member ? Member->GetOwner() : nullptr;
		if (Owner && Owner != ControlledPawn && Member->GetStatus() == EOutbreakSquadMemberStatus::Active)
		{
			OtherHelpers.Add(Owner);
		}
	}

	TArray<AActor*> PatientActors;
	TArray<FOutbreakAssistPatient> Patients;
	for (const UOutbreakSquadMemberComponent* Member : Squad->GetMembers())
	{
		AActor* Owner = Member ? Member->GetOwner() : nullptr;
		const UOutbreakHealthComponent* Health = Owner ? Owner->FindComponentByClass<UOutbreakHealthComponent>() : nullptr;
		if (!Owner || Owner == ControlledPawn || !Health || Health->GetHealthState() == EOutbreakHealthState::Dead)
		{
			continue;
		}

		FOutbreakAssistPatient& Patient = Patients.AddDefaulted_GetRef();
		Patient.Location = Owner->GetActorLocation();
		Patient.bDowned = Health->IsIncapacitated();
		Patient.HealthFraction = Health->GetHealthFraction();
		const UOutbreakMedicalComponent* Reviver = UOutbreakMedicalComponent::FindReviverOf(GetWorld(), Owner);
		Patient.bBeingRevivedByOther = Reviver && Reviver != OwnMedical;
		for (const AActor* Helper : OtherHelpers)
		{
			Patient.NearestOtherHelperDistance = FMath::Min(Patient.NearestOtherHelperDistance,
				static_cast<float>(FVector::Dist(Helper->GetActorLocation(), Patient.Location)));
		}
		PatientActors.Add(Owner);
	}

	// Healing needs a medical item; whether one works on a given patient is checked on arrival.
	const bool bCarriesMedical = Inventory && Inventory->GetItems().ContainsByPredicate([](const FOutbreakItemAmount& Entry)
	{
		return Entry.Item && Entry.Count > 0 && Entry.Item->Category == EOutbreakItemCategory::Medical;
	});

	const int32 Chosen = OutbreakMedicalAssist::ChoosePatient(ControlledPawn->GetActorLocation(), Patients,
		Config->MedicalAssistRadius, bCarriesMedical && Config->HealOthersHealthFraction > 0.0f, Config->HealOthersHealthFraction);
	if (Chosen == INDEX_NONE)
	{
		return;
	}
	AssistPatient = PatientActors[Chosen];
	Context.bHasDownedSquadmate = Patients[Chosen].bDowned;
	Context.bHasHurtSquadmate = !Patients[Chosen].bDowned;
}

FVector AOutbreakSquadAIController::ComputeAssistLocation(EOutbreakTeammateBehavior Behavior)
{
	APawn* ControlledPawn = GetPawn();
	AActor* Patient = AssistPatient.Get();
	if (!Patient)
	{
		return ControlledPawn->GetActorLocation();
	}

	const UOutbreakMedicalSettings* Settings = GetDefault<UOutbreakMedicalSettings>();
	const float Range = Behavior == EOutbreakTeammateBehavior::ReviveMember ? Settings->ReviveRange : Settings->MedicalUseRange;
	if (FVector::Dist(ControlledPawn->GetActorLocation(), Patient->GetActorLocation()) > Range * 0.8f)
	{
		return Patient->GetActorLocation();
	}

	// Arrived: work on the patient and stay put.
	if (Behavior == EOutbreakTeammateBehavior::ReviveMember)
	{
		UOutbreakMedicalComponent* Medical = ControlledPawn->FindComponentByClass<UOutbreakMedicalComponent>();
		if (Medical && !Medical->IsReviving())
		{
			Medical->StartRevive(Patient);
		}
	}
	else if (UOutbreakInventoryComponent* Inventory = ControlledPawn->FindComponentByClass<UOutbreakInventoryComponent>())
	{
		Inventory->UseMedicalOn(Patient);
	}
	return ControlledPawn->GetActorLocation();
}

// --- Positioning ---------------------------------------------------------------------------------

void AOutbreakSquadAIController::GetSlot(int32& OutSlotIndex, int32& OutSlotCount) const
{
	OutSlotIndex = 0;
	OutSlotCount = 1;
	const UOutbreakSquadComponent* Squad = GetSquad();
	if (!Squad)
	{
		return;
	}

	// Slots are shared among living teammates so the formation closes up when someone falls.
	TArray<UOutbreakSquadMemberComponent*> LivingTeammates = Squad->GetTeammates();
	LivingTeammates.RemoveAll([](const UOutbreakSquadMemberComponent* Member)
	{
		return Member->GetStatus() == EOutbreakSquadMemberStatus::Dead;
	});
	OutSlotIndex = FMath::Max(0, LivingTeammates.IndexOfByKey(SquadMember.Get()));
	OutSlotCount = FMath::Max(1, LivingTeammates.Num());
}

bool AOutbreakSquadAIController::ProjectToNavigation(FVector& InOutLocation) const
{
	UNavigationSystemV1* NavigationSystem = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	if (!NavigationSystem || !NavigationSystem->GetDefaultNavDataInstance(FNavigationSystem::DontCreate))
	{
		// No navigation data (tests, or a level without a navmesh): accept the point as is.
		return true;
	}

	FNavLocation Projected;
	if (NavigationSystem->ProjectPointToNavigation(InOutLocation, Projected, FVector(100.0f, 100.0f, 250.0f)))
	{
		InOutLocation = Projected.Location;
		return true;
	}
	return false;
}

FVector AOutbreakSquadAIController::ComputeFollowLocation() const
{
	const APawn* ControlledPawn = GetPawn();
	const APawn* PlayerPawn = GetPlayerPawn();
	const UOutbreakSquadComponent* Squad = GetSquad();
	if (!PlayerPawn || !Squad)
	{
		return ControlledPawn ? ControlledPawn->GetActorLocation() : FVector::ZeroVector;
	}

	int32 SlotIndex = 0;
	int32 SlotCount = 1;
	GetSlot(SlotIndex, SlotCount);

	const UOutbreakSquadSettings* Settings = GetDefault<UOutbreakSquadSettings>();
	const EOutbreakFormation Formation = Squad->GetFormation();
	FVector Slot = OutbreakSquadPositioning::ComputeFormationLocation(PlayerPawn->GetActorLocation(), PlayerPawn->GetActorRotation(),
		Formation, SlotIndex, SlotCount, Settings->GetFormationSpacing(Formation));
	if (ProjectToNavigation(Slot))
	{
		return Slot;
	}

	// The slot is off the navmesh (a wall, a narrow passage): fall back to single file behind the player.
	FVector ColumnSlot = OutbreakSquadPositioning::ComputeFormationLocation(PlayerPawn->GetActorLocation(), PlayerPawn->GetActorRotation(),
		EOutbreakFormation::Column, SlotIndex, SlotCount, Settings->GetFormationSpacing(EOutbreakFormation::Column));
	return ProjectToNavigation(ColumnSlot) ? ColumnSlot : PlayerPawn->GetActorLocation();
}

FVector AOutbreakSquadAIController::ComputeDefendLocation() const
{
	const FVector DefendPoint = SquadMember ? SquadMember->GetCurrentCommand().TargetLocation : GetPawn()->GetActorLocation();
	const UOutbreakTeammateAIConfig* Config = GetAIConfig();

	// Under fire, use cover if it is close enough to keep defending the point.
	if (IsUnderFire() && bHasCover && FVector::Dist(CoverLocation, DefendPoint) <= Config->DefendRadius)
	{
		return CoverLocation;
	}

	// Otherwise spread around the point, facing out, so defenders don't stack on one spot.
	int32 SlotIndex = 0;
	int32 SlotCount = 1;
	GetSlot(SlotIndex, SlotCount);
	FVector Slot = DefendPoint + OutbreakSquadPositioning::ComputeFormationOffset(EOutbreakFormation::Defensive, SlotIndex, SlotCount,
		FMath::Min(Config->DefendSpacing, Config->DefendRadius));
	return ProjectToNavigation(Slot) ? Slot : DefendPoint;
}

FVector AOutbreakSquadAIController::ComputeRetreatLocation() const
{
	const APawn* ControlledPawn = GetPawn();
	const FVector Self = ControlledPawn->GetActorLocation();

	// Away from the threat if one is known; otherwise back, away from where the player faces.
	FVector Away;
	FVector ThreatLocation;
	if (GetCoverThreatLocation(ThreatLocation))
	{
		Away = (Self - ThreatLocation).GetSafeNormal2D();
	}
	else if (const APawn* PlayerPawn = GetPlayerPawn())
	{
		Away = -PlayerPawn->GetActorForwardVector().GetSafeNormal2D();
	}
	else
	{
		Away = -ControlledPawn->GetActorForwardVector().GetSafeNormal2D();
	}
	if (Away.IsNearlyZero())
	{
		Away = -ControlledPawn->GetActorForwardVector().GetSafeNormal2D();
	}

	const float Distance = GetAIConfig()->RetreatDistance;
	for (const float Fraction : { 1.0f, 0.6f, 0.3f })
	{
		FVector Candidate = Self + Away * Distance * Fraction;
		if (ProjectToNavigation(Candidate))
		{
			return Candidate;
		}
	}
	return Self;
}

FVector AOutbreakSquadAIController::ComputeMoveLocation(EOutbreakTeammateBehavior Behavior, bool bBehaviorChanged)
{
	const APawn* ControlledPawn = GetPawn();
	const FVector PawnLocation = ControlledPawn->GetActorLocation();
	// Stopping keeps the previous stop point while the behaviour is unchanged, so the Behavior Tree
	// is not asked to re-path to a slightly different spot every update.
	const FVector StopLocation = bBehaviorChanged ? PawnLocation : MoveLocation;

	if (Behavior != EOutbreakTeammateBehavior::Retreat)
	{
		bHasRetreatLocation = false;
	}

	switch (Behavior)
	{
	case EOutbreakTeammateBehavior::FollowPlayer:
	case EOutbreakTeammateBehavior::Regroup:
		return ComputeFollowLocation();

	case EOutbreakTeammateBehavior::ReviveMember:
	case EOutbreakTeammateBehavior::HealMember:
		return ComputeAssistLocation(Behavior);

	case EOutbreakTeammateBehavior::SeekSafety:
		return bHasCover ? CoverLocation : ComputeFollowLocation();

	case EOutbreakTeammateBehavior::HoldPosition:
	case EOutbreakTeammateBehavior::MoveToLocation:
		return SquadMember ? SquadMember->GetCurrentCommand().TargetLocation : PawnLocation;

	case EOutbreakTeammateBehavior::DefendPosition:
		return ComputeDefendLocation();

	case EOutbreakTeammateBehavior::TakeCover:
		if (bHasCover)
		{
			return CoverLocation;
		}
		// No cover found (yet): hold the ordered position, or stay put.
		return SquadMember && SquadMember->GetActiveCommandType() == EOutbreakSquadCommandType::Cover
			? SquadMember->GetCurrentCommand().TargetLocation
			: StopLocation;

	case EOutbreakTeammateBehavior::Retreat:
		if (bBehaviorChanged || !bHasRetreatLocation)
		{
			RetreatLocation = ComputeRetreatLocation();
			bHasRetreatLocation = true;
		}
		return RetreatLocation;

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

	case EOutbreakTeammateBehavior::Hiding:
		// Freeze exactly where it was noticed.
		return bBehaviorChanged ? PawnLocation : MoveLocation;

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
