#include "Environment/OutbreakTimeOfDay.h"

#include "Components/DirectionalLightComponent.h"
#include "Components/LightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/SkyLight.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Net/UnrealNetwork.h"
#include "Outbreak.h"
#include "TimerManager.h"

namespace OutbreakTimeOfDay
{
	namespace
	{
		float DayLength(float SunriseHour, float SunsetHour)
		{
			const float Length = WrapHour(SunsetHour - SunriseHour);
			return Length > 0.0f ? Length : 12.0f;
		}
	}

	float WrapHour(float Hour)
	{
		const float Wrapped = FMath::Fmod(Hour, 24.0f);
		return Wrapped < 0.0f ? Wrapped + 24.0f : Wrapped;
	}

	float ComputeSunElevation(float Hour, float SunriseHour, float SunsetHour, float MaxElevation)
	{
		const float Day = DayLength(SunriseHour, SunsetHour);
		const float Night = 24.0f - Day;
		const float SinceSunrise = WrapHour(Hour - SunriseHour);
		if (SinceSunrise <= Day)
		{
			return MaxElevation * FMath::Sin(PI * SinceSunrise / Day);
		}
		return -MaxElevation * FMath::Sin(PI * (SinceSunrise - Day) / Night);
	}

	float ComputeDaylight(float SunElevation)
	{
		return FMath::SmoothStep(-6.0f, 10.0f, SunElevation);
	}

	float ComputeSunYaw(float Hour, float SunriseHour, float SunsetHour, float SunriseYaw)
	{
		const float Day = DayLength(SunriseHour, SunsetHour);
		const float SinceSunrise = WrapHour(Hour - SunriseHour);
		const float Turn = SinceSunrise <= Day
			? 180.0f * SinceSunrise / Day
			: 180.0f + 180.0f * (SinceSunrise - Day) / (24.0f - Day);
		return SunriseYaw + Turn;
	}
}

AOutbreakTimeOfDay::AOutbreakTimeOfDay()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	bAlwaysRelevant = true;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

#if WITH_EDITORONLY_DATA
	// The lighting model looks it up at runtime, so it must always be loaded.
	bIsSpatiallyLoaded = false;
#endif
}

void AOutbreakTimeOfDay::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AOutbreakTimeOfDay, Hour);
}

AOutbreakTimeOfDay* AOutbreakTimeOfDay::Find(const UWorld* World)
{
	if (!World)
	{
		return nullptr;
	}
	for (TActorIterator<AOutbreakTimeOfDay> It(World); It; ++It)
	{
		if (IsValid(*It))
		{
			return *It;
		}
	}
	return nullptr;
}

void AOutbreakTimeOfDay::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		Hour = OutbreakTimeOfDay::WrapHour(StartHour);
		if (bAdvanceTime)
		{
			GetWorldTimerManager().SetTimer(AdvanceTimer, this, &ThisClass::AdvanceTime, UpdateInterval, /*bLoop=*/ true);
		}
	}
	ApplyLighting();
}

void AOutbreakTimeOfDay::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(AdvanceTimer);
	Super::EndPlay(EndPlayReason);
}

void AOutbreakTimeOfDay::SetHour(float NewHour)
{
	if (!HasAuthority())
	{
		return;
	}
	Hour = OutbreakTimeOfDay::WrapHour(NewHour);
	ApplyLighting();
}

void AOutbreakTimeOfDay::AdvanceTime()
{
	SetHour(Hour + UpdateInterval * 24.0f / (DayLengthMinutes * 60.0f));
}

void AOutbreakTimeOfDay::OnRep_Hour()
{
	ApplyLighting();
}

void AOutbreakTimeOfDay::SetLights(ADirectionalLight* InSun, ADirectionalLight* InMoon, ASkyLight* InSkyLight)
{
	SunLight = InSun;
	MoonLight = InMoon;
	SkyLight = InSkyLight;
	ApplyLighting();
}

float AOutbreakTimeOfDay::GetDaylight() const
{
	return OutbreakTimeOfDay::ComputeDaylight(OutbreakTimeOfDay::ComputeSunElevation(Hour, SunriseHour, SunsetHour, MaxSunElevation));
}

float AOutbreakTimeOfDay::GetGameplayAmbientLightLevel() const
{
	return FMath::Lerp(NightAmbientLightLevel, DayAmbientLightLevel, GetDaylight());
}

float AOutbreakTimeOfDay::GetDirectionalLightFactor(const UDirectionalLightComponent* Light) const
{
	if (!Light)
	{
		return 1.0f;
	}
	if (SunLight && Light == SunLight->GetLightComponent())
	{
		return GetDaylight();
	}
	if (MoonLight && Light == MoonLight->GetLightComponent())
	{
		return (1.0f - GetDaylight()) * MoonlightFactor;
	}
	return 1.0f;
}

void AOutbreakTimeOfDay::ApplyLighting()
{
	const float Elevation = OutbreakTimeOfDay::ComputeSunElevation(Hour, SunriseHour, SunsetHour, MaxSunElevation);
	const float Daylight = OutbreakTimeOfDay::ComputeDaylight(Elevation);
	const float SunYaw = OutbreakTimeOfDay::ComputeSunYaw(Hour, SunriseHour, SunsetHour, SunriseYaw);

	// A light shines along its forward vector: from the sun's position towards the ground.
	auto ApplyDirectional = [this](ADirectionalLight* Light, const FRotator& Rotation, float Intensity)
	{
		if (!Light || !Light->GetLightComponent())
		{
			return;
		}
		Light->GetLightComponent()->SetIntensity(Intensity);
		if (Light->GetRootComponent() && Light->GetRootComponent()->Mobility == EComponentMobility::Movable)
		{
			Light->SetActorRotation(Rotation);
		}
		else if (!bWarnedAboutMobility)
		{
			bWarnedAboutMobility = true;
			UE_LOG(LogOutbreak, Warning, TEXT("%s: %s is not Movable, so it cannot follow the time of day."), *GetName(), *Light->GetName());
		}
	};

	ApplyDirectional(SunLight, FRotator(-Elevation, SunYaw + 180.0f, 0.0f), SunIntensity * Daylight);
	ApplyDirectional(MoonLight, FRotator(Elevation, SunYaw, 0.0f), MoonIntensity * (1.0f - Daylight));

	if (SkyLight && SkyLight->GetLightComponent())
	{
		SkyLight->GetLightComponent()->SetIntensity(FMath::Lerp(SkyLightNightIntensity, SkyLightDayIntensity, Daylight));
	}
}
