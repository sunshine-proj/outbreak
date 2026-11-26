#pragma once

#include "CoreMinimal.h"

/** A squadmate who may need help, as seen by one AI teammate (Phase 10). */
struct FOutbreakAssistPatient
{
	FVector Location = FVector::ZeroVector;

	/** Incapacitated (needs a revive). */
	bool bDowned = false;

	float HealthFraction = 1.0f;

	/** Someone else is already reviving them. */
	bool bBeingRevivedByOther = false;

	/** Distance from the patient to the nearest other teammate able to help. Max = none. */
	float NearestOtherHelperDistance = TNumericLimits<float>::Max();
};

namespace OutbreakMedicalAssist
{
	/**
	 * Which patient this teammate should help, or INDEX_NONE. Downed squadmates come first (nearest
	 * first), then, if bCanHeal (carries a usable medical item), squadmates below HealFraction.
	 * Skips patients beyond SearchRadius, already being revived, or with another helper nearer, so
	 * one teammate goes to each patient.
	 */
	OUTBREAK_API int32 ChoosePatient(const FVector& SelfLocation, TConstArrayView<FOutbreakAssistPatient> Patients,
		float SearchRadius, bool bCanHeal, float HealFraction);
}
