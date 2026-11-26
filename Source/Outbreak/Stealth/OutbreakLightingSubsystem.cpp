#include "Stealth/OutbreakLightingSubsystem.h"

#include "CollisionQueryParams.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/LocalLightComponent.h"
#include "Components/SpotLightComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Stealth/OutbreakLightingVolume.h"
#include "Stealth/OutbreakStealthSettings.h"
#include "Stealth/OutbreakVisibility.h"
#include "UObject/UObjectIterator.h"

void UOutbreakLightingSubsystem::RefreshCacheIfNeeded()
{
	const UWorld* World = GetWorld();
	const double Now = World ? World->GetTimeSeconds() : 0.0;
	if (Now - LastRefreshTime < GetDefault<UOutbreakStealthSettings>()->LightCacheRefreshInterval)
	{
		return;
	}
	LastRefreshTime = Now;

	LocalLights.Reset();
	DirectionalLights.Reset();
	Volumes.Reset();

	for (TObjectIterator<ULocalLightComponent> It; It; ++It)
	{
		if (It->GetWorld() == World && IsValid(*It))
		{
			LocalLights.Add(*It);
		}
	}
	for (TObjectIterator<UDirectionalLightComponent> It; It; ++It)
	{
		if (It->GetWorld() == World && IsValid(*It))
		{
			DirectionalLights.Add(*It);
		}
	}
	for (TActorIterator<AOutbreakLightingVolume> It(World); It; ++It)
	{
		Volumes.Add(*It);
	}
}

bool UOutbreakLightingSubsystem::IsShadowed(const FVector& From, const FVector& To, const AActor* IgnoredActor) const
{
	FCollisionQueryParams Params(SCENE_QUERY_STAT(OutbreakLightShadow), false, IgnoredActor);
	FHitResult Hit;
	return GetWorld()->LineTraceSingleByChannel(Hit, From, To, ECC_Visibility, Params);
}

float UOutbreakLightingSubsystem::GetLightLevelAt(const FVector& Location, const AActor* IgnoredActor)
{
	RefreshCacheIfNeeded();
	const UOutbreakStealthSettings* Settings = GetDefault<UOutbreakStealthSettings>();

	// Designer volumes override everything.
	const AOutbreakLightingVolume* Override = nullptr;
	for (const TWeakObjectPtr<AOutbreakLightingVolume>& Entry : Volumes)
	{
		const AOutbreakLightingVolume* Volume = Entry.Get();
		if (Volume && Volume->ContainsPoint(Location) && (!Override || Volume->GetPriority() > Override->GetPriority()))
		{
			Override = Volume;
		}
	}
	if (Override)
	{
		return Override->GetLightLevel();
	}

	float Level = Settings->AmbientLightLevel;

	for (const TWeakObjectPtr<ULocalLightComponent>& Entry : LocalLights)
	{
		const ULocalLightComponent* Light = Entry.Get();
		if (!Light || !Light->IsVisible() || Light->Intensity <= 0.0f)
		{
			continue;
		}
		const FVector LightLocation = Light->GetComponentLocation();
		const float Distance = FVector::Dist(LightLocation, Location);
		if (Distance >= Light->AttenuationRadius)
		{
			continue;
		}

		float ConeFactor = 1.0f;
		if (const USpotLightComponent* Spot = Cast<USpotLightComponent>(Light))
		{
			const FVector ToTarget = (Location - LightLocation).GetSafeNormal();
			const float Angle = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(FVector::DotProduct(Spot->GetForwardVector(), ToTarget), -1.0f, 1.0f)));
			ConeFactor = OutbreakVisibility::ComputeSpotConeFactor(Angle, Spot->InnerConeAngle, Spot->OuterConeAngle);
			if (ConeFactor <= 0.0f)
			{
				continue;
			}
		}

		Level += OutbreakVisibility::ComputeLocalLightContribution(Distance, Light->AttenuationRadius, ConeFactor,
			IsShadowed(LightLocation, Location, IgnoredActor), Settings->LocalLightContribution);
	}

	for (const TWeakObjectPtr<UDirectionalLightComponent>& Entry : DirectionalLights)
	{
		const UDirectionalLightComponent* Light = Entry.Get();
		if (!Light || !Light->IsVisible() || Light->Intensity <= 0.0f)
		{
			continue;
		}
		const FVector TowardLight = -Light->GetDirection();
		if (!IsShadowed(Location + TowardLight * Settings->DirectionalShadowTraceDistance, Location, IgnoredActor))
		{
			Level += Settings->DirectionalLightContribution;
		}
	}

	return FMath::Clamp(Level, 0.0f, 1.0f);
}
