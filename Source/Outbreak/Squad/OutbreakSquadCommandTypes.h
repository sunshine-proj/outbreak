#pragma once

#include "CoreMinimal.h"

#include "OutbreakSquadCommandTypes.generated.h"

class AActor;
class UOutbreakSquadMemberComponent;

/** Basic squad commands (D-003). Defend, Cover and Retreat are Phase 7; Stealth is Phase 8. */
UENUM(BlueprintType)
enum class EOutbreakSquadCommandType : uint8
{
	/** No order: the member behaves autonomously. */
	None,
	/** Stay with the player, even under threat. */
	Follow,
	/** Stay at the position held when ordered. */
	Hold,
	/** Go to TargetLocation, then hold there. */
	Move,
	/** Return to the player; completes on arrival. */
	Regroup,
	/** Engage TargetActor; completes when it is dead or gone. */
	Attack
};

UENUM(BlueprintType)
enum class EOutbreakSquadCommandStatus : uint8
{
	None,
	Active,
	Completed,
	/** Replaced or cancelled by the player. */
	Cancelled,
	/** Could not be carried out (for example the member went down). */
	Failed
};

UENUM(BlueprintType)
enum class EOutbreakCommandScope : uint8
{
	/** Every teammate. */
	Squad,
	/** One member: MemberIds[0]. */
	Individual,
	/** The members in MemberIds. */
	Selected
};

/** Why a member refused a command. */
UENUM(BlueprintType)
enum class EOutbreakCommandRejection : uint8
{
	None,
	NotInSquad,
	/** Commands are for teammates, not the player. */
	PlayerMember,
	/** Incapacitated or dead. */
	Unavailable,
	MissingLocation,
	InvalidTarget,
	TargetNotHostile,
	NoCommand
};

/** One order. Structured data rather than strings (AGENTS.md §5.2). */
USTRUCT(BlueprintType)
struct OUTBREAK_API FOutbreakSquadCommand
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Command")
	EOutbreakSquadCommandType Type = EOutbreakSquadCommandType::None;

	/** Move destination or Hold position. Hold without a location holds where the member stands. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Command")
	FVector TargetLocation = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Command")
	bool bHasTargetLocation = false;

	/** Attack target. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Command")
	TObjectPtr<AActor> TargetActor;

	static FOutbreakSquadCommand Make(EOutbreakSquadCommandType InType);
	static FOutbreakSquadCommand MakeMove(const FVector& Location);
	static FOutbreakSquadCommand MakeHold(const FVector& Location);
	static FOutbreakSquadCommand MakeAttack(AActor* Target);

	bool RequiresLocation() const { return Type == EOutbreakSquadCommandType::Move; }
	bool RequiresTarget() const { return Type == EOutbreakSquadCommandType::Attack; }
};

/** Who a command is for. */
USTRUCT(BlueprintType)
struct OUTBREAK_API FOutbreakCommandRecipients
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Command")
	EOutbreakCommandScope Scope = EOutbreakCommandScope::Squad;

	/** Member IDs (UOutbreakSquadMemberComponent::GetMemberId) for Individual and Selected. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Command")
	TArray<int32> MemberIds;

	static FOutbreakCommandRecipients ForSquad();
	static FOutbreakCommandRecipients ForMember(int32 MemberId);
	static FOutbreakCommandRecipients ForMembers(const TArray<int32>& InMemberIds);
};

/** The outcome of a command for one member. */
USTRUCT(BlueprintType)
struct OUTBREAK_API FOutbreakCommandResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Command")
	TObjectPtr<UOutbreakSquadMemberComponent> Member;

	UPROPERTY(BlueprintReadOnly, Category = "Command")
	EOutbreakCommandRejection Rejection = EOutbreakCommandRejection::None;

	bool WasAccepted() const { return Rejection == EOutbreakCommandRejection::None; }
};

/**
 * The player's current choice of command recipients, by member ID: either the whole squad or an
 * explicit set. Local UI state; it does not replicate.
 */
struct OUTBREAK_API FOutbreakSquadSelection
{
	void SelectAll();
	void SelectOnly(int32 MemberId);

	/** Adds or removes MemberId. Toggling from "all" starts a new set; an empty set means "all". */
	void Toggle(int32 MemberId);

	/** Drops IDs that are no longer in ValidMemberIds. */
	void Prune(const TArray<int32>& ValidMemberIds);

	bool IsAllSelected() const { return bAllSelected; }
	bool IsSelected(int32 MemberId) const { return bAllSelected || SelectedMemberIds.Contains(MemberId); }
	const TArray<int32>& GetSelectedMemberIds() const { return SelectedMemberIds; }

	FOutbreakCommandRecipients ToRecipients() const;

private:
	bool bAllSelected = true;
	TArray<int32> SelectedMemberIds;
};

namespace OutbreakSquadCommands
{
	OUTBREAK_API FText GetCommandDisplayName(EOutbreakSquadCommandType Type);
	OUTBREAK_API FText GetRejectionText(EOutbreakCommandRejection Rejection);
}
