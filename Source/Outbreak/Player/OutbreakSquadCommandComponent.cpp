#include "Player/OutbreakSquadCommandComponent.h"

#include "AI/OutbreakSquadPositioning.h"
#include "AI/OutbreakEnemyTypes.h"
#include "AI/OutbreakTeammateBehaviorTypes.h"
#include "Characters/OutbreakEnemyCharacter.h"
#include "Combat/OutbreakHealthComponent.h"
#include "EngineUtils.h"
#include "CollisionQueryParams.h"
#include "Combat/OutbreakTeam.h"
#include "Engine/World.h"
#include "Game/OutbreakPlayerState.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "Outbreak.h"
#include "Squad/OutbreakSquadComponent.h"
#include "Squad/OutbreakSquadMemberComponent.h"

#define LOCTEXT_NAMESPACE "OutbreakSquadCommands"

UOutbreakSquadCommandComponent::UOutbreakSquadCommandComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

UOutbreakSquadComponent* UOutbreakSquadCommandComponent::GetSquad() const
{
	const AController* Controller = Cast<AController>(GetOwner());
	const AOutbreakPlayerState* PlayerState = Controller ? Controller->GetPlayerState<AOutbreakPlayerState>() : nullptr;
	return PlayerState ? PlayerState->GetSquadComponent() : nullptr;
}

UOutbreakSquadMemberComponent* UOutbreakSquadCommandComponent::GetTeammateInSlot(int32 Slot) const
{
	const UOutbreakSquadComponent* Squad = GetSquad();
	if (!Squad)
	{
		return nullptr;
	}
	const TArray<UOutbreakSquadMemberComponent*> Teammates = Squad->GetTeammates();
	return Teammates.IsValidIndex(Slot - 1) ? Teammates[Slot - 1] : nullptr;
}

void UOutbreakSquadCommandComponent::PruneSelection()
{
	TArray<int32> ValidIds;
	if (const UOutbreakSquadComponent* Squad = GetSquad())
	{
		for (const UOutbreakSquadMemberComponent* Teammate : Squad->GetTeammates())
		{
			ValidIds.Add(Teammate->GetMemberId());
		}
	}
	Selection.Prune(ValidIds);
}

void UOutbreakSquadCommandComponent::SetFeedback(const FText& Feedback)
{
	LastFeedback = Feedback;
	UE_LOG(LogOutbreak, Log, TEXT("Squad: %s"), *Feedback.ToString());
}

void UOutbreakSquadCommandComponent::SelectAll()
{
	Selection.SelectAll();
}

void UOutbreakSquadCommandComponent::SelectSlot(int32 Slot)
{
	if (const UOutbreakSquadMemberComponent* Teammate = GetTeammateInSlot(Slot))
	{
		Selection.SelectOnly(Teammate->GetMemberId());
	}
}

void UOutbreakSquadCommandComponent::ToggleSlot(int32 Slot)
{
	if (const UOutbreakSquadMemberComponent* Teammate = GetTeammateInSlot(Slot))
	{
		Selection.Toggle(Teammate->GetMemberId());
	}
}

bool UOutbreakSquadCommandComponent::TraceCommandTarget(FVector& OutLocation, AActor*& OutActor) const
{
	const AController* Controller = Cast<AController>(GetOwner());
	const APawn* Pawn = Controller ? Controller->GetPawn() : nullptr;
	const UWorld* World = GetWorld();
	if (!Pawn || !World)
	{
		return false;
	}

	FVector ViewLocation;
	FRotator ViewRotation;
	Pawn->GetActorEyesViewPoint(ViewLocation, ViewRotation);

	const FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(OutbreakSquadCommandTrace), false, Pawn);
	FHitResult Hit;
	if (!World->LineTraceSingleByChannel(Hit, ViewLocation, ViewLocation + ViewRotation.Vector() * CommandTraceRange,
		CommandTraceChannel, QueryParams))
	{
		return false;
	}

	OutLocation = Hit.ImpactPoint;
	OutActor = Hit.GetActor();
	return true;
}

bool UOutbreakSquadCommandComponent::IssueCommand(EOutbreakSquadCommandType CommandType)
{
	UOutbreakSquadComponent* Squad = GetSquad();
	if (!Squad)
	{
		return false;
	}
	PruneSelection();

	FOutbreakSquadCommand Command = FOutbreakSquadCommand::Make(CommandType);

	// Defend the point under the crosshair if there is one; otherwise teammates defend where they stand.
	if (CommandType == EOutbreakSquadCommandType::Defend)
	{
		FVector HitLocation;
		AActor* HitActor = nullptr;
		if (TraceCommandTarget(HitLocation, HitActor))
		{
			Command.TargetLocation = HitLocation;
			Command.bHasTargetLocation = true;
		}
	}

	if (Command.RequiresLocation() || Command.RequiresTarget())
	{
		FVector HitLocation;
		AActor* HitActor = nullptr;
		if (!TraceCommandTarget(HitLocation, HitActor))
		{
			SetFeedback(LOCTEXT("NoTarget", "No target under the crosshair."));
			return false;
		}

		if (Command.RequiresLocation())
		{
			Command.TargetLocation = HitLocation;
			Command.bHasTargetLocation = true;
		}
		if (Command.RequiresTarget())
		{
			const AController* Controller = Cast<AController>(GetOwner());
			if (!OutbreakTeam::IsHostile(Controller ? Controller->GetPawn() : nullptr, HitActor))
			{
				SetFeedback(LOCTEXT("NotHostile", "Attack: no hostile under the crosshair."));
				return false;
			}
			Command.TargetActor = HitActor;
		}
	}

	// Count accepting recipients locally for immediate feedback; the server validates again.
	const FOutbreakCommandRecipients Recipients = Selection.ToRecipients();
	int32 Accepted = 0;
	int32 Total = 0;
	for (const UOutbreakSquadMemberComponent* Member : Squad->ResolveRecipients(Recipients))
	{
		++Total;
		if (Squad->ValidateCommandForMember(Command, Member) == EOutbreakCommandRejection::None)
		{
			++Accepted;
		}
	}

	if (Accepted == 0)
	{
		SetFeedback(FText::Format(LOCTEXT("NoneAccepted", "{0}: no available teammate."),
			OutbreakSquadCommands::GetCommandDisplayName(CommandType)));
		return false;
	}

	Squad->IssueCommand(Command, Recipients);
	SetFeedback(FText::Format(LOCTEXT("Issued", "{0}: {1} of {2} teammates."),
		OutbreakSquadCommands::GetCommandDisplayName(CommandType), Accepted, Total));
	return true;
}

void UOutbreakSquadCommandComponent::CancelCommands()
{
	if (UOutbreakSquadComponent* Squad = GetSquad())
	{
		PruneSelection();
		Squad->CancelCommand(Selection.ToRecipients());
		SetFeedback(LOCTEXT("Cancelled", "Orders cancelled."));
	}
}

void UOutbreakSquadCommandComponent::CycleFormation()
{
	if (UOutbreakSquadComponent* Squad = GetSquad())
	{
		Squad->CycleFormation();
		SetFeedback(FText::Format(LOCTEXT("FormationCycle", "Formation: {0}."),
			OutbreakSquadPositioning::GetFormationDisplayName(Squad->GetFormation())));
	}
}

void UOutbreakSquadCommandComponent::SetFormation(EOutbreakFormation Formation)
{
	if (UOutbreakSquadComponent* Squad = GetSquad())
	{
		Squad->SetFormation(Formation);
		SetFeedback(FText::Format(LOCTEXT("Formation", "Formation: {0}."), OutbreakSquadPositioning::GetFormationDisplayName(Formation)));
	}
}

void UOutbreakSquadCommandComponent::ToggleStealthMode()
{
	if (UOutbreakSquadComponent* Squad = GetSquad())
	{
		const bool bEnable = !Squad->IsStealthMode();
		Squad->SetStealthMode(bEnable);
		SetFeedback(bEnable ? LOCTEXT("StealthOn", "Stealth: on. Hold fire, stay low.") : LOCTEXT("StealthOff", "Stealth: off. Weapons free."));
	}
}

FText UOutbreakSquadCommandComponent::BuildEnemyAwarenessText() const
{
	int32 Counts[static_cast<int32>(EOutbreakEnemyState::Retreating) + 1] = {};
	for (TActorIterator<AOutbreakEnemyCharacter> It(GetWorld()); It; ++It)
	{
		if (!It->IsDead())
		{
			++Counts[static_cast<int32>(It->GetEnemyState())];
		}
	}

	TArray<FText> Parts;
	auto AddPart = [&Parts, &Counts](EOutbreakEnemyState State, const FText& Label)
	{
		const int32 Count = Counts[static_cast<int32>(State)];
		if (Count > 0)
		{
			Parts.Add(FText::Format(LOCTEXT("AwarenessPart", "{0} {1}"), Count, Label));
		}
	};
	AddPart(EOutbreakEnemyState::Suspicious, LOCTEXT("AwareSuspicious", "suspicious"));
	AddPart(EOutbreakEnemyState::Investigating, LOCTEXT("AwareInvestigating", "investigating"));
	AddPart(EOutbreakEnemyState::Searching, LOCTEXT("AwareSearching", "searching"));
	AddPart(EOutbreakEnemyState::Combat, LOCTEXT("AwareCombat", "in combat"));
	AddPart(EOutbreakEnemyState::Retreating, LOCTEXT("AwareRetreating", "retreating"));

	return Parts.IsEmpty()
		? LOCTEXT("AwarenessNone", "Enemies: unaware")
		: FText::Format(LOCTEXT("Awareness", "Enemies: {0}"), FText::Join(FText::FromString(TEXT(", ")), Parts));
}

FText FOutbreakSquadMenuEntry::GetLabel() const
{
	if (bIsStealthToggle)
	{
		return LOCTEXT("MenuStealth", "Stealth on / off");
	}
	return bIsFormation
		? FText::Format(LOCTEXT("MenuFormation", "Formation: {0}"), OutbreakSquadPositioning::GetFormationDisplayName(Formation))
		: OutbreakSquadCommands::GetCommandDisplayName(Command);
}

TArray<FOutbreakSquadMenuEntry> UOutbreakSquadCommandComponent::GetMenuEntries()
{
	TArray<FOutbreakSquadMenuEntry> Entries;
	for (int32 Value = static_cast<int32>(EOutbreakSquadCommandType::Follow); Value <= static_cast<int32>(EOutbreakSquadCommandType::Retreat); ++Value)
	{
		FOutbreakSquadMenuEntry& Entry = Entries.AddDefaulted_GetRef();
		Entry.Command = static_cast<EOutbreakSquadCommandType>(Value);
	}
	Entries.AddDefaulted_GetRef().bIsStealthToggle = true;
	for (int32 Value = 0; Value <= static_cast<int32>(EOutbreakFormation::Loose); ++Value)
	{
		FOutbreakSquadMenuEntry& Entry = Entries.AddDefaulted_GetRef();
		Entry.bIsFormation = true;
		Entry.Formation = static_cast<EOutbreakFormation>(Value);
	}
	return Entries;
}

void UOutbreakSquadCommandComponent::ToggleMenu()
{
	bMenuOpen = !bMenuOpen;
	MenuIndex = 0;
}

void UOutbreakSquadCommandComponent::CloseMenu()
{
	bMenuOpen = false;
}

void UOutbreakSquadCommandComponent::MenuNext()
{
	if (bMenuOpen)
	{
		MenuIndex = (MenuIndex + 1) % GetMenuEntries().Num();
	}
}

void UOutbreakSquadCommandComponent::MenuPrevious()
{
	if (bMenuOpen)
	{
		const int32 Count = GetMenuEntries().Num();
		MenuIndex = (MenuIndex - 1 + Count) % Count;
	}
}

bool UOutbreakSquadCommandComponent::MenuConfirm()
{
	if (!bMenuOpen)
	{
		return false;
	}

	const TArray<FOutbreakSquadMenuEntry> Entries = GetMenuEntries();
	bMenuOpen = false;
	if (!Entries.IsValidIndex(MenuIndex) || !GetSquad())
	{
		return false;
	}

	const FOutbreakSquadMenuEntry& Entry = Entries[MenuIndex];
	if (Entry.bIsStealthToggle)
	{
		ToggleStealthMode();
		return true;
	}
	if (Entry.bIsFormation)
	{
		SetFormation(Entry.Formation);
		return true;
	}
	return IssueCommand(Entry.Command);
}

FText UOutbreakSquadCommandComponent::BuildMenuText() const
{
	if (!bMenuOpen)
	{
		return FText::GetEmpty();
	}

	TArray<FText> Lines;
	Lines.Add(LOCTEXT("MenuTitle", "SQUAD ORDERS"));
	const TArray<FOutbreakSquadMenuEntry> Entries = GetMenuEntries();
	for (int32 Index = 0; Index < Entries.Num(); ++Index)
	{
		Lines.Add(FText::Format(LOCTEXT("MenuLine", "{0} {1}"),
			Index == MenuIndex ? LOCTEXT("MenuMarker", ">") : LOCTEXT("MenuNoMarker", " "), Entries[Index].GetLabel()));
	}
	return FText::Join(FText::FromString(TEXT("\n")), Lines);
}

FText UOutbreakSquadCommandComponent::FormatMemberLine(int32 Slot, const UOutbreakSquadMemberComponent* Member, bool bSelected, float DistanceMeters)
{
	if (!Member)
	{
		return FText::GetEmpty();
	}

	FText Condition;
	switch (Member->GetStatus())
	{
	case EOutbreakSquadMemberStatus::Dead:
		Condition = LOCTEXT("Dead", "KIA");
		break;
	case EOutbreakSquadMemberStatus::Incapacitated:
	{
		// With the bleed-out countdown, so the player knows how long is left to revive them.
		const AActor* Owner = Member->GetOwner();
		const UOutbreakHealthComponent* Health = Owner ? Owner->FindComponentByClass<UOutbreakHealthComponent>() : nullptr;
		const float Remaining = Health ? Health->GetBleedOutTimeRemaining() : -1.0f;
		Condition = Remaining >= 0.0f
			? FText::Format(LOCTEXT("DownTimed", "DOWN {0}s"), FMath::CeilToInt(Remaining))
			: LOCTEXT("Down", "DOWN");
		break;
	}
	default:
		Condition = FText::Format(LOCTEXT("HealthPercent", "{0}%"), FMath::RoundToInt(Member->GetHealthFraction() * 100.0f));
		break;
	}

	const FText Order = OutbreakSquadCommands::GetCommandDisplayName(Member->GetActiveCommandType());
	const FText Activity = OutbreakTeammateBehavior::GetBehaviorDisplayName(Member->GetCurrentBehavior());
	const FText Marker = bSelected ? LOCTEXT("SelectedMarker", ">") : LOCTEXT("UnselectedMarker", " ");

	return FText::Format(LOCTEXT("MemberLine", "{0} {1} {2}  {3} ({4})  {5}  {6} m"),
		Marker, Slot, Member->GetDisplayName(), Order, Activity, Condition, FMath::RoundToInt(DistanceMeters));
}

FText UOutbreakSquadCommandComponent::BuildSquadStatusText() const
{
	const UOutbreakSquadComponent* Squad = GetSquad();
	if (!Squad)
	{
		return FText::GetEmpty();
	}

	TArray<FText> Lines;
	FText Formation = OutbreakSquadPositioning::GetFormationDisplayName(Squad->GetFormation());
	if (Squad->IsStealthMode())
	{
		Formation = Squad->IsCompromised() ? LOCTEXT("StealthCompromised", "STEALTH (compromised)") : LOCTEXT("StealthQuiet", "STEALTH");
	}
	Lines.Add(Selection.IsAllSelected()
		? FText::Format(LOCTEXT("HeaderAll", "Squad {0}/{1} alive - {2} - all selected"), Squad->GetAliveMemberCount(), Squad->GetMemberCount(), Formation)
		: FText::Format(LOCTEXT("HeaderSome", "Squad {0}/{1} alive - {2} - {3} selected"), Squad->GetAliveMemberCount(), Squad->GetMemberCount(),
			Formation, Selection.GetSelectedMemberIds().Num()));

	const TArray<UOutbreakSquadMemberComponent*> Teammates = Squad->GetTeammates();
	for (int32 Index = 0; Index < Teammates.Num(); ++Index)
	{
		const UOutbreakSquadMemberComponent* Teammate = Teammates[Index];
		const float DistanceMeters = FMath::Max(0.0f, Squad->GetDistanceToPlayer(Teammate)) / 100.0f;
		Lines.Add(FormatMemberLine(Index + 1, Teammate, Selection.IsSelected(Teammate->GetMemberId()), DistanceMeters));
	}

	Lines.Add(BuildEnemyAwarenessText());
	if (!LastFeedback.IsEmpty())
	{
		Lines.Add(LastFeedback);
	}
	return FText::Join(FText::FromString(TEXT("\n")), Lines);
}

#undef LOCTEXT_NAMESPACE
