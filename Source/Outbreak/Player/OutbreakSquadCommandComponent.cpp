#include "Player/OutbreakSquadCommandComponent.h"

#include "AI/OutbreakTeammateBehaviorTypes.h"
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
		Condition = LOCTEXT("Down", "DOWN");
		break;
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
	Lines.Add(Selection.IsAllSelected()
		? FText::Format(LOCTEXT("HeaderAll", "Squad {0}/{1} alive - all selected"), Squad->GetAliveMemberCount(), Squad->GetMemberCount())
		: FText::Format(LOCTEXT("HeaderSome", "Squad {0}/{1} alive - {2} selected"), Squad->GetAliveMemberCount(), Squad->GetMemberCount(),
			Selection.GetSelectedMemberIds().Num()));

	const TArray<UOutbreakSquadMemberComponent*> Teammates = Squad->GetTeammates();
	for (int32 Index = 0; Index < Teammates.Num(); ++Index)
	{
		const UOutbreakSquadMemberComponent* Teammate = Teammates[Index];
		const float DistanceMeters = FMath::Max(0.0f, Squad->GetDistanceToPlayer(Teammate)) / 100.0f;
		Lines.Add(FormatMemberLine(Index + 1, Teammate, Selection.IsSelected(Teammate->GetMemberId()), DistanceMeters));
	}

	if (!LastFeedback.IsEmpty())
	{
		Lines.Add(LastFeedback);
	}
	return FText::Join(FText::FromString(TEXT("\n")), Lines);
}

#undef LOCTEXT_NAMESPACE
