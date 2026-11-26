#include "AI/OutbreakMedicalAssist.h"

namespace OutbreakMedicalAssist
{
	int32 ChoosePatient(const FVector& SelfLocation, TConstArrayView<FOutbreakAssistPatient> Patients,
		float SearchRadius, bool bCanHeal, float HealFraction)
	{
		int32 Best = INDEX_NONE;
		bool bBestDowned = false;
		float BestDistance = TNumericLimits<float>::Max();

		for (int32 Index = 0; Index < Patients.Num(); ++Index)
		{
			const FOutbreakAssistPatient& Patient = Patients[Index];
			const bool bNeedsHeal = bCanHeal && !Patient.bDowned && Patient.HealthFraction > 0.0f && Patient.HealthFraction < HealFraction;
			if (!Patient.bDowned && !bNeedsHeal)
			{
				continue;
			}

			const float Distance = FVector::Dist(SelfLocation, Patient.Location);
			if (Distance > SearchRadius || Patient.bBeingRevivedByOther || Patient.NearestOtherHelperDistance < Distance)
			{
				continue;
			}

			// Downed beats hurt; then nearer beats farther.
			const bool bBetter = Best == INDEX_NONE
				|| (Patient.bDowned && !bBestDowned)
				|| (Patient.bDowned == bBestDowned && Distance < BestDistance);
			if (bBetter)
			{
				Best = Index;
				bBestDowned = Patient.bDowned;
				BestDistance = Distance;
			}
		}
		return Best;
	}
}
