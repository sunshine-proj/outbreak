#include "Missions/OutbreakObjectiveQueries.h"

#include "Combat/OutbreakHealthComponent.h"
#include "Combat/OutbreakTeam.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"

#define LOCTEXT_NAMESPACE "OutbreakObjectives"

namespace OutbreakObjectiveQueries
{
	int32 CountHostilesNear(const UWorld* World, const FVector& Location, float Radius)
	{
		if (!World)
		{
			return 0;
		}
		const FGenericTeamId PlayerTeam = OutbreakTeam::ToGenericTeamId(EOutbreakTeam::Player);
		const float RadiusSquared = FMath::Square(Radius);
		int32 Count = 0;
		for (TActorIterator<APawn> It(World); It; ++It)
		{
			const APawn* Pawn = *It;
			if (!IsValid(Pawn)
				|| OutbreakTeam::GetAttitude(PlayerTeam, FGenericTeamId::GetTeamIdentifier(Pawn)) != ETeamAttitude::Hostile
				|| FVector::DistSquared(Pawn->GetActorLocation(), Location) > RadiusSquared)
			{
				continue;
			}
			const UOutbreakHealthComponent* Health = Pawn->FindComponentByClass<UOutbreakHealthComponent>();
			Count += !Health || Health->IsAlive() ? 1 : 0;
		}
		return Count;
	}

	bool IsDestroyed(const AActor* Actor)
	{
		if (!IsValid(Actor))
		{
			return true;
		}
		const UOutbreakHealthComponent* Health = Actor->FindComponentByClass<UOutbreakHealthComponent>();
		return Health && Health->GetHealthState() == EOutbreakHealthState::Dead;
	}

	TArray<AActor*> FindActorsWithTag(const UWorld* World, FName Tag)
	{
		TArray<AActor*> Result;
		if (!World || Tag.IsNone())
		{
			return Result;
		}
		for (TActorIterator<AActor> It(World); It; ++It)
		{
			if (IsValid(*It) && It->ActorHasTag(Tag))
			{
				Result.Add(*It);
			}
		}
		return Result;
	}

	FText FormatTime(float Seconds)
	{
		const int32 Total = FMath::Max(0, FMath::CeilToInt(Seconds));
		FNumberFormattingOptions TwoDigits;
		TwoDigits.MinimumIntegralDigits = 2;
		return FText::Format(LOCTEXT("TimeFormat", "{0}:{1}"), FText::AsNumber(Total / 60), FText::AsNumber(Total % 60, &TwoDigits));
	}
}

#undef LOCTEXT_NAMESPACE
