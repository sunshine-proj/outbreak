#include "Squad/OutbreakSquadCommandTypes.h"

#define LOCTEXT_NAMESPACE "OutbreakSquadCommands"

FOutbreakSquadCommand FOutbreakSquadCommand::Make(EOutbreakSquadCommandType InType)
{
	FOutbreakSquadCommand Command;
	Command.Type = InType;
	return Command;
}

FOutbreakSquadCommand FOutbreakSquadCommand::MakeMove(const FVector& Location)
{
	FOutbreakSquadCommand Command = Make(EOutbreakSquadCommandType::Move);
	Command.TargetLocation = Location;
	Command.bHasTargetLocation = true;
	return Command;
}

FOutbreakSquadCommand FOutbreakSquadCommand::MakeHold(const FVector& Location)
{
	FOutbreakSquadCommand Command = Make(EOutbreakSquadCommandType::Hold);
	Command.TargetLocation = Location;
	Command.bHasTargetLocation = true;
	return Command;
}

FOutbreakSquadCommand FOutbreakSquadCommand::MakeAttack(AActor* Target)
{
	FOutbreakSquadCommand Command = Make(EOutbreakSquadCommandType::Attack);
	Command.TargetActor = Target;
	return Command;
}

FOutbreakCommandRecipients FOutbreakCommandRecipients::ForSquad()
{
	return FOutbreakCommandRecipients();
}

FOutbreakCommandRecipients FOutbreakCommandRecipients::ForMember(int32 MemberId)
{
	FOutbreakCommandRecipients Recipients;
	Recipients.Scope = EOutbreakCommandScope::Individual;
	Recipients.MemberIds.Add(MemberId);
	return Recipients;
}

FOutbreakCommandRecipients FOutbreakCommandRecipients::ForMembers(const TArray<int32>& InMemberIds)
{
	FOutbreakCommandRecipients Recipients;
	Recipients.Scope = EOutbreakCommandScope::Selected;
	Recipients.MemberIds = InMemberIds;
	return Recipients;
}

void FOutbreakSquadSelection::SelectAll()
{
	bAllSelected = true;
	SelectedMemberIds.Reset();
}

void FOutbreakSquadSelection::SelectOnly(int32 MemberId)
{
	bAllSelected = false;
	SelectedMemberIds.Reset();
	SelectedMemberIds.Add(MemberId);
}

void FOutbreakSquadSelection::Toggle(int32 MemberId)
{
	if (bAllSelected)
	{
		SelectOnly(MemberId);
		return;
	}

	if (SelectedMemberIds.Remove(MemberId) == 0)
	{
		SelectedMemberIds.Add(MemberId);
	}
	if (SelectedMemberIds.IsEmpty())
	{
		SelectAll();
	}
}

void FOutbreakSquadSelection::Prune(const TArray<int32>& ValidMemberIds)
{
	if (bAllSelected)
	{
		return;
	}

	SelectedMemberIds.RemoveAll([&ValidMemberIds](int32 Id) { return !ValidMemberIds.Contains(Id); });
	if (SelectedMemberIds.IsEmpty())
	{
		SelectAll();
	}
}

FOutbreakCommandRecipients FOutbreakSquadSelection::ToRecipients() const
{
	if (bAllSelected)
	{
		return FOutbreakCommandRecipients::ForSquad();
	}
	return SelectedMemberIds.Num() == 1
		? FOutbreakCommandRecipients::ForMember(SelectedMemberIds[0])
		: FOutbreakCommandRecipients::ForMembers(SelectedMemberIds);
}

namespace OutbreakSquadCommands
{
	FText GetCommandDisplayName(EOutbreakSquadCommandType Type)
	{
		switch (Type)
		{
		case EOutbreakSquadCommandType::Follow:  return LOCTEXT("Follow", "Follow");
		case EOutbreakSquadCommandType::Hold:    return LOCTEXT("Hold", "Hold");
		case EOutbreakSquadCommandType::Move:    return LOCTEXT("Move", "Move");
		case EOutbreakSquadCommandType::Regroup: return LOCTEXT("Regroup", "Regroup");
		case EOutbreakSquadCommandType::Attack:  return LOCTEXT("Attack", "Attack");
		default:                                 return LOCTEXT("None", "Autonomous");
		}
	}

	FText GetRejectionText(EOutbreakCommandRejection Rejection)
	{
		switch (Rejection)
		{
		case EOutbreakCommandRejection::None:             return LOCTEXT("Accepted", "accepted");
		case EOutbreakCommandRejection::NotInSquad:       return LOCTEXT("NotInSquad", "not in the squad");
		case EOutbreakCommandRejection::PlayerMember:     return LOCTEXT("PlayerMember", "the player cannot be ordered");
		case EOutbreakCommandRejection::Unavailable:      return LOCTEXT("Unavailable", "unavailable (down or dead)");
		case EOutbreakCommandRejection::MissingLocation:  return LOCTEXT("MissingLocation", "no destination");
		case EOutbreakCommandRejection::InvalidTarget:    return LOCTEXT("InvalidTarget", "no valid target");
		case EOutbreakCommandRejection::TargetNotHostile: return LOCTEXT("TargetNotHostile", "target is not hostile");
		default:                                          return LOCTEXT("NoCommand", "no command");
		}
	}
}

#undef LOCTEXT_NAMESPACE
