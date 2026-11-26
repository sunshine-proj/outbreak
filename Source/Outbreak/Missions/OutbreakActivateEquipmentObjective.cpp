#include "Missions/OutbreakActivateEquipmentObjective.h"

#include "Missions/OutbreakMissionEquipment.h"
#include "Missions/OutbreakMissionSubsystem.h"
#include "Missions/OutbreakObjectiveQueries.h"
#include "Outbreak.h"

#define LOCTEXT_NAMESPACE "OutbreakObjectives"

void UOutbreakActivateEquipmentObjective::Activate(UOutbreakMissionSubsystem& Mission)
{
	Equipment.Reset();
	ActivatedCount = 0;

	for (AActor* Actor : OutbreakObjectiveQueries::FindActorsWithTag(Mission.GetWorld(), EquipmentTag))
	{
		if (AOutbreakMissionEquipment* Piece = Cast<AOutbreakMissionEquipment>(Actor))
		{
			Piece->SetLocked(false);
			Equipment.Add(Piece);
		}
	}

	if (Equipment.IsEmpty())
	{
		UE_LOG(LogOutbreak, Error, TEXT("Activate Equipment objective: no AOutbreakMissionEquipment tagged '%s' in the level; the objective fails."),
			*EquipmentTag.ToString());
	}
	else if (RequiredCount > Equipment.Num())
	{
		UE_LOG(LogOutbreak, Error, TEXT("Activate Equipment objective: needs %d but only %d tagged '%s'; all of them are required instead."),
			RequiredCount, Equipment.Num(), *EquipmentTag.ToString());
	}
}

int32 UOutbreakActivateEquipmentObjective::GetRequired() const
{
	return RequiredCount > 0 ? FMath::Min(RequiredCount, Equipment.Num()) : Equipment.Num();
}

EOutbreakObjectiveState UOutbreakActivateEquipmentObjective::Evaluate(UOutbreakMissionSubsystem& Mission)
{
	if (Equipment.IsEmpty())
	{
		return EOutbreakObjectiveState::Failed;
	}

	ActivatedCount = 0;
	int32 Usable = 0;
	for (const TWeakObjectPtr<AOutbreakMissionEquipment>& Piece : Equipment)
	{
		if (!Piece.IsValid())
		{
			continue;
		}
		++Usable;
		ActivatedCount += Piece->IsActivated() ? 1 : 0;
	}

	if (ActivatedCount >= GetRequired())
	{
		return EOutbreakObjectiveState::Completed;
	}
	// Equipment removed from the level (destroyed) can no longer be activated.
	return Usable < GetRequired() ? EOutbreakObjectiveState::Failed : EOutbreakObjectiveState::Active;
}

void UOutbreakActivateEquipmentObjective::Deactivate()
{
	// Equipment stays unlocked: using it after the objective has ended is harmless.
	Equipment.Reset();
}

bool UOutbreakActivateEquipmentObjective::Validate(TArray<FText>& OutErrors) const
{
	if (EquipmentTag.IsNone())
	{
		OutErrors.Add(LOCTEXT("ActivateNoTag", "Activate Equipment objective has no EquipmentTag."));
		return false;
	}
	return true;
}

FText UOutbreakActivateEquipmentObjective::GetProgressText() const
{
	return GetRequired() > 1
		? FText::Format(LOCTEXT("ActivateProgress", "{0} / {1}"), FMath::Min(ActivatedCount, GetRequired()), GetRequired())
		: FText::GetEmpty();
}

#undef LOCTEXT_NAMESPACE
