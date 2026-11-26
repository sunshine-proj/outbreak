#include "Missions/OutbreakEliminateTargetObjective.h"

#include "Combat/OutbreakHealthComponent.h"
#include "EngineUtils.h"
#include "GameFramework/Actor.h"
#include "Missions/OutbreakMissionSubsystem.h"
#include "Outbreak.h"

#define LOCTEXT_NAMESPACE "OutbreakObjectives"

void UOutbreakEliminateTargetObjective::Activate(UOutbreakMissionSubsystem& Mission)
{
	Targets.Reset();
	EliminatedCount = 0;

	for (TActorIterator<AActor> It(Mission.GetWorld()); It; ++It)
	{
		if (IsValid(*It) && It->ActorHasTag(TargetTag))
		{
			Targets.Add(*It);
		}
	}

	if (Targets.IsEmpty())
	{
		UE_LOG(LogOutbreak, Warning, TEXT("Eliminate Target objective: no actors tagged '%s'; the objective completes immediately."),
			*TargetTag.ToString());
	}
}

bool UOutbreakEliminateTargetObjective::IsEliminated(const TWeakObjectPtr<AActor>& Target)
{
	const AActor* Actor = Target.Get();
	if (!IsValid(Actor))
	{
		return true;
	}
	const UOutbreakHealthComponent* Health = Actor->FindComponentByClass<UOutbreakHealthComponent>();
	return Health && Health->GetHealthState() == EOutbreakHealthState::Dead;
}

EOutbreakObjectiveState UOutbreakEliminateTargetObjective::Evaluate(UOutbreakMissionSubsystem& Mission)
{
	EliminatedCount = 0;
	for (const TWeakObjectPtr<AActor>& Target : Targets)
	{
		EliminatedCount += IsEliminated(Target) ? 1 : 0;
	}
	return EliminatedCount >= Targets.Num() ? EOutbreakObjectiveState::Completed : EOutbreakObjectiveState::Active;
}

bool UOutbreakEliminateTargetObjective::Validate(TArray<FText>& OutErrors) const
{
	if (TargetTag.IsNone())
	{
		OutErrors.Add(LOCTEXT("EliminateNoTag", "Eliminate Target objective has no TargetTag."));
		return false;
	}
	return true;
}

FText UOutbreakEliminateTargetObjective::GetProgressText() const
{
	return Targets.IsEmpty()
		? FText::GetEmpty()
		: FText::Format(LOCTEXT("EliminateProgress", "{0} / {1}"), EliminatedCount, Targets.Num());
}

#undef LOCTEXT_NAMESPACE
