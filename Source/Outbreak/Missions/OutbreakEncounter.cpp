#include "Missions/OutbreakEncounter.h"

#include "AI/OutbreakEnemyAIController.h"
#include "Characters/OutbreakEnemyCharacter.h"
#include "Components/BoxComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Missions/OutbreakAlarmSubsystem.h"
#include "Missions/OutbreakMissionSubsystem.h"
#include "Missions/OutbreakObjective.h"
#include "Outbreak.h"
#include "TimerManager.h"

AOutbreakEncounter::AOutbreakEncounter()
{
	PrimaryActorTick.bCanEverTick = false;

	TriggerArea = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerArea"));
	TriggerArea->SetBoxExtent(FVector(500.0f, 500.0f, 200.0f));
	TriggerArea->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	TriggerArea->SetCanEverAffectNavigation(false);
	TriggerArea->SetHiddenInGame(true);
	RootComponent = TriggerArea;

#if WITH_EDITORONLY_DATA
	// Triggers must be listening even when the area is far from the players.
	bIsSpatiallyLoaded = false;
#endif
}

void AOutbreakEncounter::Configure(EOutbreakEncounterTrigger InTrigger, FName InObjectiveId, const TArray<FOutbreakEncounterGroup>& InGroups,
	const TArray<AActor*>& InSpawnPoints, FName InSpawnedTag, float InDelay, EOutbreakEncounterOrder InOrder)
{
	TriggerType = InTrigger;
	ObjectiveId = InObjectiveId;
	Groups = InGroups;
	SpawnPoints.Reset();
	for (AActor* Point : InSpawnPoints)
	{
		SpawnPoints.Add(Point);
	}
	SpawnedTag = InSpawnedTag;
	Delay = InDelay;
	Order = InOrder;
}

void AOutbreakEncounter::BeginPlay()
{
	Super::BeginPlay();
	if (HasAuthority())
	{
		BindTrigger();
	}
}

void AOutbreakEncounter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(DelayTimer);
		if (UOutbreakMissionSubsystem* Mission = World->GetSubsystem<UOutbreakMissionSubsystem>())
		{
			Mission->OnMissionStateChanged.RemoveDynamic(this, &ThisClass::HandleMissionStateChanged);
			Mission->OnObjectiveStateChanged.RemoveDynamic(this, &ThisClass::HandleObjectiveStateChanged);
		}
		if (UOutbreakAlarmSubsystem* Alarm = World->GetSubsystem<UOutbreakAlarmSubsystem>())
		{
			Alarm->OnAlarmRaised.RemoveDynamic(this, &ThisClass::HandleAlarmRaised);
		}
	}
	Super::EndPlay(EndPlayReason);
}

void AOutbreakEncounter::BindTrigger()
{
	UWorld* World = GetWorld();
	if (bBound || !World || !HasAuthority())
	{
		return;
	}
	bBound = true;

	UOutbreakMissionSubsystem* Mission = World->GetSubsystem<UOutbreakMissionSubsystem>();
	switch (TriggerType)
	{
	case EOutbreakEncounterTrigger::MissionStart:
		if (Mission)
		{
			Mission->OnMissionStateChanged.AddUniqueDynamic(this, &ThisClass::HandleMissionStateChanged);
		}
		break;

	case EOutbreakEncounterTrigger::ObjectiveActivated:
	case EOutbreakEncounterTrigger::ObjectiveCompleted:
		if (ObjectiveId.IsNone())
		{
			UE_LOG(LogOutbreak, Error, TEXT("%s: objective trigger without an ObjectiveId; it never fires."), *GetName());
		}
		else if (Mission)
		{
			Mission->OnObjectiveStateChanged.AddUniqueDynamic(this, &ThisClass::HandleObjectiveStateChanged);
		}
		break;

	case EOutbreakEncounterTrigger::Alarm:
		if (UOutbreakAlarmSubsystem* Alarm = World->GetSubsystem<UOutbreakAlarmSubsystem>())
		{
			Alarm->OnAlarmRaised.AddUniqueDynamic(this, &ThisClass::HandleAlarmRaised);
			if (Alarm->IsAlarmRaised())
			{
				Trigger();
			}
		}
		break;

	case EOutbreakEncounterTrigger::PlayerEntersArea:
		TriggerArea->SetCollisionProfileName(UCollisionProfile::CustomCollisionProfileName);
		TriggerArea->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		TriggerArea->SetCollisionResponseToAllChannels(ECR_Ignore);
		TriggerArea->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
		TriggerArea->SetGenerateOverlapEvents(true);
		TriggerArea->OnComponentBeginOverlap.AddUniqueDynamic(this, &ThisClass::HandleAreaOverlap);
		TriggerArea->UpdateOverlaps();
		break;

	default:
		break;
	}
}

void AOutbreakEncounter::HandleMissionStateChanged(EOutbreakMissionState NewState)
{
	if (NewState == EOutbreakMissionState::InProgress)
	{
		Trigger();
	}
}

void AOutbreakEncounter::HandleObjectiveStateChanged(int32 ObjectiveIndex, EOutbreakObjectiveState NewState)
{
	const UOutbreakMissionSubsystem* Mission = GetWorld() ? GetWorld()->GetSubsystem<UOutbreakMissionSubsystem>() : nullptr;
	const UOutbreakObjective* Objective = Mission ? Mission->GetObjective(ObjectiveIndex) : nullptr;
	if (!Objective || Objective->GetObjectiveId() != ObjectiveId)
	{
		return;
	}
	const EOutbreakObjectiveState Wanted = TriggerType == EOutbreakEncounterTrigger::ObjectiveActivated
		? EOutbreakObjectiveState::Active : EOutbreakObjectiveState::Completed;
	if (NewState == Wanted)
	{
		Trigger();
	}
}

void AOutbreakEncounter::HandleAlarmRaised(FVector Location)
{
	Trigger();
}

void AOutbreakEncounter::HandleAreaOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	const APawn* Pawn = Cast<APawn>(OtherActor);
	if (Pawn && Pawn->IsPlayerControlled())
	{
		Trigger();
	}
}

void AOutbreakEncounter::Trigger()
{
	if (!HasAuthority() || (bTriggered && !bRepeatable))
	{
		return;
	}
	bTriggered = true;
	UE_LOG(LogOutbreak, Log, TEXT("Encounter %s triggered."), *GetName());

	if (Delay > 0.0f && GetWorld())
	{
		GetWorld()->GetTimerManager().SetTimer(DelayTimer, this, &ThisClass::Spawn, Delay, false);
	}
	else
	{
		Spawn();
	}
}

FTransform AOutbreakEncounter::GetSpawnTransform(int32 SpawnIndex) const
{
	TArray<const AActor*> Points;
	for (const TObjectPtr<AActor>& Point : SpawnPoints)
	{
		if (IsValid(Point))
		{
			Points.Add(Point);
		}
	}
	const AActor* Point = Points.IsEmpty() ? static_cast<const AActor*>(this) : Points[SpawnIndex % Points.Num()];
	// Second and later enemies at the same point stand in a ring around it.
	const int32 Round = Points.IsEmpty() ? SpawnIndex : SpawnIndex / Points.Num();
	FVector Offset = FVector::ZeroVector;
	if (Round > 0)
	{
		const float Angle = Round * 2.39996f;   // golden angle: no two enemies on the same spot
		Offset = FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0.0f) * (100.0f + 40.0f * Round);
	}
	return FTransform(FRotator(0.0f, Point->GetActorRotation().Yaw, 0.0f), Point->GetActorLocation() + Offset);
}

void AOutbreakEncounter::Spawn()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	int32 SpawnIndex = 0;
	for (const FOutbreakEncounterGroup& Group : Groups)
	{
		const TSubclassOf<AOutbreakEnemyCharacter> EnemyClass = Group.EnemyClass
			? Group.EnemyClass : TSubclassOf<AOutbreakEnemyCharacter>(AOutbreakEnemyCharacter::StaticClass());
		for (int32 Index = 0; Index < Group.Count; ++Index)
		{
			const FTransform Transform = GetSpawnTransform(SpawnIndex++);
			AOutbreakEnemyCharacter* Enemy = World->SpawnActorDeferred<AOutbreakEnemyCharacter>(EnemyClass, Transform, nullptr, nullptr,
				ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
			if (!Enemy)
			{
				continue;
			}
			if (Group.Definition)
			{
				Enemy->SetEnemyDefinition(Group.Definition);
			}
			if (Order == EOutbreakEncounterOrder::Guard && PatrolRoute)
			{
				Enemy->SetPatrolRoute(PatrolRoute);
			}
			if (!SpawnedTag.IsNone())
			{
				Enemy->Tags.Add(SpawnedTag);
			}
			Enemy->FinishSpawning(Transform);
			Spawned.Add(Enemy);
			ApplyOrder(Enemy);
		}
	}
	UE_LOG(LogOutbreak, Log, TEXT("Encounter %s spawned %d enemies."), *GetName(), SpawnIndex);
	OnEncounterSpawned();
}

void AOutbreakEncounter::ApplyOrder(AOutbreakEnemyCharacter* Enemy) const
{
	AOutbreakEnemyAIController* Controller = Enemy ? Cast<AOutbreakEnemyAIController>(Enemy->GetController()) : nullptr;
	if (!Controller || Order == EOutbreakEncounterOrder::Guard)
	{
		return;
	}

	FVector Target = InvestigateTarget ? InvestigateTarget->GetActorLocation() : GetActorLocation();
	if (Order == EOutbreakEncounterOrder::Hunt)
	{
		float Closest = TNumericLimits<float>::Max();
		const UOutbreakMissionSubsystem* Mission = GetWorld()->GetSubsystem<UOutbreakMissionSubsystem>();
		TArray<APawn*> Players = Mission ? Mission->GetLivingPlayerPawns() : TArray<APawn*>();
		if (Players.IsEmpty())
		{
			for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
			{
				if (APawn* Pawn = It->IsValid() ? It->Get()->GetPawn() : nullptr)
				{
					Players.Add(Pawn);
				}
			}
		}
		for (const APawn* Player : Players)
		{
			const float Distance = FVector::DistSquared(Player->GetActorLocation(), Enemy->GetActorLocation());
			if (Distance < Closest)
			{
				Closest = Distance;
				Target = Player->GetActorLocation();
			}
		}
	}
	Controller->ReportAlert(Target);
}

bool AOutbreakEncounter::IsCleared() const
{
	if (Spawned.IsEmpty())
	{
		return false;
	}
	for (const TWeakObjectPtr<AOutbreakEnemyCharacter>& Enemy : Spawned)
	{
		if (Enemy.IsValid() && !Enemy->IsDead())
		{
			return false;
		}
	}
	return true;
}

TArray<AOutbreakEnemyCharacter*> AOutbreakEncounter::GetSpawnedEnemies() const
{
	TArray<AOutbreakEnemyCharacter*> Result;
	for (const TWeakObjectPtr<AOutbreakEnemyCharacter>& Enemy : Spawned)
	{
		if (Enemy.IsValid())
		{
			Result.Add(Enemy.Get());
		}
	}
	return Result;
}
