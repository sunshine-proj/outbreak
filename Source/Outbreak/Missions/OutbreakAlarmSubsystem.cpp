#include "Missions/OutbreakAlarmSubsystem.h"

#include "Characters/OutbreakEnemyCharacter.h"
#include "Combat/OutbreakHealthComponent.h"
#include "Engine/World.h"
#include "Game/OutbreakGameState.h"
#include "Missions/OutbreakMissionDefinition.h"
#include "Missions/OutbreakMissionSubsystem.h"
#include "Outbreak.h"
#include "TimerManager.h"

void UOutbreakAlarmSubsystem::Deinitialize()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(PendingTimer);
	}
	Super::Deinitialize();
}

bool UOutbreakAlarmSubsystem::IsAlarmEnabled() const
{
	const UOutbreakMissionSubsystem* Mission = GetWorld() ? GetWorld()->GetSubsystem<UOutbreakMissionSubsystem>() : nullptr;
	const UOutbreakMissionDefinition* Definition = Mission ? Mission->GetMissionDefinition() : nullptr;
	return Definition && Definition->bAlarmEnabled;
}

void UOutbreakAlarmSubsystem::ReportCombatStarted(AActor* Spotter)
{
	if (!Spotter || bRaised || PendingSpotter.IsValid() || !IsAlarmEnabled())
	{
		return;
	}

	const UOutbreakMissionDefinition* Definition = GetWorld()->GetSubsystem<UOutbreakMissionSubsystem>()->GetMissionDefinition();
	PendingSpotter = Spotter;
	if (Definition->AlarmDelay <= 0.0f)
	{
		ResolvePendingAlarm();
		return;
	}
	UE_LOG(LogOutbreak, Log, TEXT("%s spotted the squad: alarm in %.1f s unless silenced."), *Spotter->GetName(), Definition->AlarmDelay);
	GetWorld()->GetTimerManager().SetTimer(PendingTimer, this, &ThisClass::ResolvePendingAlarm, Definition->AlarmDelay, false);
}

bool UOutbreakAlarmSubsystem::CanStillRaise(const AActor* Spotter)
{
	if (!IsValid(Spotter))
	{
		return false;
	}
	const UOutbreakHealthComponent* Health = Spotter->FindComponentByClass<UOutbreakHealthComponent>();
	if (Health && !Health->IsAlive())
	{
		return false;
	}
	// A spotter that lost the squad again (back to searching or idle) does not raise it either.
	const AOutbreakEnemyCharacter* Enemy = Cast<AOutbreakEnemyCharacter>(Spotter);
	return !Enemy || Enemy->IsInCombat();
}

void UOutbreakAlarmSubsystem::ResolvePendingAlarm()
{
	AActor* Spotter = PendingSpotter.Get();
	PendingSpotter.Reset();
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(PendingTimer);
	}

	if (CanStillRaise(Spotter))
	{
		RaiseAlarm(Spotter->GetActorLocation(), Spotter);
	}
	else
	{
		UE_LOG(LogOutbreak, Log, TEXT("Alarm averted: the spotter was silenced in time."));
	}
}

void UOutbreakAlarmSubsystem::RaiseAlarm(FVector Location, AActor* RaisedBy)
{
	if (bRaised)
	{
		return;
	}
	bRaised = true;
	AlarmLocation = Location;
	UE_LOG(LogOutbreak, Log, TEXT("ALARM raised by %s."), *GetNameSafe(RaisedBy));
	PublishAlarmState();
	OnAlarmRaised.Broadcast(Location);
}

void UOutbreakAlarmSubsystem::ResetAlarm()
{
	PendingSpotter.Reset();
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(PendingTimer);
	}
	if (!bRaised)
	{
		return;
	}
	bRaised = false;
	UE_LOG(LogOutbreak, Log, TEXT("Alarm silenced."));
	PublishAlarmState();
	OnAlarmReset.Broadcast();
}

void UOutbreakAlarmSubsystem::PublishAlarmState() const
{
	const UWorld* World = GetWorld();
	if (AOutbreakGameState* GameState = World ? World->GetGameState<AOutbreakGameState>() : nullptr)
	{
		GameState->SetAlarm(bRaised, AlarmLocation);
	}
}
