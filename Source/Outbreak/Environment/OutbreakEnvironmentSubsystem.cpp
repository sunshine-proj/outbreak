#include "Environment/OutbreakEnvironmentSubsystem.h"

#include "Engine/World.h"
#include "EngineUtils.h"
#include "Stealth/OutbreakStealthSettings.h"
#include "Outbreak.h"

// Phase 15 profiling: `stat Outbreak`, Insights and the Outbreak CSV category.
DECLARE_CYCLE_STAT(TEXT("Environment zone query"), STAT_OutbreakEnvironmentQuery, STATGROUP_Outbreak);

void UOutbreakEnvironmentSubsystem::RefreshCacheIfNeeded()
{
	const UWorld* World = GetWorld();
	const double Now = World ? World->GetTimeSeconds() : 0.0;
	// Zones are placed in the level and rarely change; the lighting cache interval suits them too.
	if (Now - LastRefreshTime < GetDefault<UOutbreakStealthSettings>()->LightCacheRefreshInterval)
	{
		return;
	}
	LastRefreshTime = Now;

	Volumes.Reset();
	for (TActorIterator<AOutbreakEnvironmentVolume> It(World); It; ++It)
	{
		if (IsValid(*It))
		{
			Volumes.Add(*It);
		}
	}
}

FOutbreakEnvironmentPerception UOutbreakEnvironmentSubsystem::GetPerceptionAt(const FVector& Location)
{
	OUTBREAK_SCOPE_CYCLE_COUNTER(STAT_OutbreakEnvironmentQuery);

	RefreshCacheIfNeeded();

	const AOutbreakEnvironmentVolume* Best = nullptr;
	for (const TWeakObjectPtr<AOutbreakEnvironmentVolume>& Entry : Volumes)
	{
		const AOutbreakEnvironmentVolume* Volume = Entry.Get();
		if (Volume && Volume->ContainsPoint(Location) && (!Best || Volume->GetPriority() > Best->GetPriority()))
		{
			Best = Volume;
		}
	}
	return Best ? Best->GetPerception() : FOutbreakEnvironmentPerception();
}

float UOutbreakEnvironmentSubsystem::GetSightRangeMultiplier(const FVector& ViewerLocation, const FVector& TargetLocation)
{
	return FMath::Min(GetPerceptionAt(ViewerLocation).SightRangeMultiplier, GetPerceptionAt(TargetLocation).SightRangeMultiplier);
}
