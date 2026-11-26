#pragma once

#include "CoreMinimal.h"

#include "OutbreakMovementSettings.generated.h"

/**
 * Movement tuning for Outbreak characters.
 *
 * Shared by the player and (from Phase 3) AI teammates so both move with the same rules.
 * Edited on the character's Blueprint defaults; values are applied to the CharacterMovementComponent
 * when the character initializes its components.
 *
 * Heights are measured from the capsule centre, in centimetres.
 */
USTRUCT(BlueprintType)
struct OUTBREAK_API FOutbreakMovementSettings
{
	GENERATED_BODY()

	/** Normal (non-sprinting, standing) ground speed. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement", meta = (ClampMin = "0.0", Units = "cm/s"))
	float WalkSpeed = 300.0f;

	/** Ground speed while sprinting. Expected to be greater than WalkSpeed. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement", meta = (ClampMin = "0.0", Units = "cm/s"))
	float SprintSpeed = 600.0f;

	/** Ground speed while crouched. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement", meta = (ClampMin = "0.0", Units = "cm/s"))
	float CrouchSpeed = 150.0f;

	/** Initial upward velocity of a jump. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement", meta = (ClampMin = "0.0", Units = "cm/s"))
	float JumpZVelocity = 420.0f;

	/** Lateral control while airborne (0 = none, 1 = full). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float AirControl = 0.2f;

	/** Capsule half height while crouched. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement", meta = (ClampMin = "1.0", Units = "cm"))
	float CrouchedHalfHeight = 55.0f;

	/** Camera height above the capsule centre while standing. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera", meta = (Units = "cm"))
	float StandingEyeHeight = 64.0f;

	/** Camera height above the capsule centre while crouched. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera", meta = (Units = "cm"))
	float CrouchedEyeHeight = 45.0f;
};
