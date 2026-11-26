#pragma once

#include "CoreMinimal.h"

#include "OutbreakSquadTypes.generated.h"

/** Squad size range. Sizes always include the player. */
USTRUCT(BlueprintType)
struct OUTBREAK_API FOutbreakSquadSizeLimits
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Squad", meta = (ClampMin = "1"))
	int32 MinSquadSize = 2;

	/** Size used when a squad configuration does not request one. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Squad", meta = (ClampMin = "1"))
	int32 DefaultSquadSize = 4;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Squad", meta = (ClampMin = "1"))
	int32 MaxSquadSize = 6;

	/** 1 <= Min <= Default <= Max. */
	bool IsValid() const;

	bool Contains(int32 SquadSize) const { return SquadSize >= MinSquadSize && SquadSize <= MaxSquadSize; }
};

/** A squad member's condition, derived from its health. */
UENUM(BlueprintType)
enum class EOutbreakSquadMemberStatus : uint8
{
	Active,
	Incapacitated,
	Dead
};

UENUM(BlueprintType)
enum class EOutbreakSquadRegistrationResult : uint8
{
	Registered,
	AlreadyMember,
	/** The squad already has MaxSquadSize members. */
	SquadFull,
	/** The member belongs to a different squad; remove it there first. */
	InOtherSquad,
	/** Null member, or called without authority. */
	Invalid
};

/**
 * Squad size rules (ROADMAP D-008).
 *
 * Global limits come from project settings (UOutbreakSquadSettings). A squad configuration may
 * narrow them per mission but never widen them: overrides outside the global range are clamped
 * into it. A requested size outside the resolved range is rejected.
 */
namespace OutbreakSquadRules
{
	/**
	 * Combines global limits with optional per-configuration overrides (0 = inherit).
	 * Fails if the global limits are invalid or the overrides leave an empty range.
	 * The default size is clamped into the resolved range.
	 */
	OUTBREAK_API bool ResolveLimits(const FOutbreakSquadSizeLimits& GlobalLimits, int32 MinOverride, int32 MaxOverride,
		FOutbreakSquadSizeLimits& OutLimits, FText& OutError);

	/**
	 * Resolves the squad size to create. RequestedSize 0 means "use Limits.DefaultSquadSize".
	 * Fails if RequestedSize is outside Limits.
	 */
	OUTBREAK_API bool ResolveSquadSize(const FOutbreakSquadSizeLimits& Limits, int32 RequestedSize,
		int32& OutSquadSize, FText& OutError);
}
