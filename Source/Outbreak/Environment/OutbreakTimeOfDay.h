#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "OutbreakTimeOfDay.generated.h"

class ADirectionalLight;
class ASkyLight;
class UDirectionalLightComponent;

/** Day/night curve (Phase 12). Pure, so it is testable without lights. Hours are 0..24. */
namespace OutbreakTimeOfDay
{
	/** Hour wrapped into [0, 24). */
	OUTBREAK_API float WrapHour(float Hour);

	/**
	 * Sun elevation in degrees: 0 at sunrise and sunset, MaxElevation halfway between, and the
	 * mirror image below the horizon at night (lowest halfway through the night).
	 */
	OUTBREAK_API float ComputeSunElevation(float Hour, float SunriseHour, float SunsetHour, float MaxElevation);

	/** Daylight 0 (night) .. 1 (day): a smooth ramp from 6° below the horizon to 10° above it (twilight). */
	OUTBREAK_API float ComputeDaylight(float SunElevation);

	/** Sun yaw: SunriseYaw at sunrise, turning 180° by sunset. Night continues the turn. */
	OUTBREAK_API float ComputeSunYaw(float Hour, float SunriseHour, float SunsetHour, float SunriseYaw);
}

/**
 * Basic day/night support (Phase 12). Place one per level and point it at the level's sun (and
 * optionally moon and sky light), all Movable. It turns the sun, sets light intensities, and tells
 * the stealth lighting model (UOutbreakLightingSubsystem) how bright ambient and directional light
 * are, so night missions are darker for enemies too.
 *
 * The hour is a level setting, can be set per mission (UOutbreakMissionDefinition::StartHour), and
 * advances only when bAdvanceTime is on. Server-authoritative; the hour replicates and every machine
 * applies the lighting itself. Updates on a timer, never per frame.
 */
UCLASS()
class OUTBREAK_API AOutbreakTimeOfDay : public AActor
{
	GENERATED_BODY()

public:

	AOutbreakTimeOfDay();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** Authority: jumps to Hour (wrapped) and applies the lighting. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Time of Day")
	void SetHour(float NewHour);

	UFUNCTION(BlueprintPure, Category = "Time of Day")
	float GetHour() const { return Hour; }

	/** 0 at night, 1 in daylight. */
	UFUNCTION(BlueprintPure, Category = "Time of Day")
	float GetDaylight() const;

	/** Gameplay ambient light level at this hour (replaces the stealth setting's Ambient Light Level). */
	float GetGameplayAmbientLightLevel() const;

	/**
	 * Scale for a directional light's gameplay contribution: daylight for the sun, moonlight at
	 * night for the moon, 1 for directional lights this actor does not control.
	 */
	float GetDirectionalLightFactor(const UDirectionalLightComponent* Light) const;

	/** Points this actor at the lights to control (normally set in the level). */
	void SetLights(ADirectionalLight* InSun, ADirectionalLight* InMoon, ASkyLight* InSkyLight);

	/** Sets rotations and intensities for the current hour on this machine. */
	void ApplyLighting();

	/** The first time-of-day actor in World, or null. */
	static AOutbreakTimeOfDay* Find(const UWorld* World);

protected:

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Lights")
	TObjectPtr<ADirectionalLight> SunLight;

	/** Optional. Shines from opposite the sun at night. */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Lights")
	TObjectPtr<ADirectionalLight> MoonLight;

	/** Optional. Its intensity follows daylight. */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Lights")
	TObjectPtr<ASkyLight> SkyLight;

	/** Hour at level start (0..24). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Time", meta = (ClampMin = "0.0", ClampMax = "24.0"))
	float StartHour = 14.0f;

	/** Let time pass. Off = the hour stays where the level or mission set it. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Time")
	bool bAdvanceTime = false;

	/** Real minutes for a full 24-hour day while time advances. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Time", meta = (ClampMin = "1.0", Units = "min", EditCondition = "bAdvanceTime"))
	float DayLengthMinutes = 48.0f;

	/** Seconds between lighting updates while time advances. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Time", meta = (ClampMin = "0.1", Units = "s", EditCondition = "bAdvanceTime"))
	float UpdateInterval = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sun", meta = (ClampMin = "0.0", ClampMax = "24.0"))
	float SunriseHour = 6.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sun", meta = (ClampMin = "0.0", ClampMax = "24.0"))
	float SunsetHour = 18.0f;

	/** Highest sun elevation, at midday. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sun", meta = (ClampMin = "5.0", ClampMax = "90.0", Units = "deg"))
	float MaxSunElevation = 60.0f;

	/** Compass direction of sunrise (world yaw). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sun", meta = (Units = "deg"))
	float SunriseYaw = 90.0f;

	/** Sun intensity in full daylight (lux for the default physical units). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sun", meta = (ClampMin = "0.0"))
	float SunIntensity = 10.0f;

	/** Moon intensity in full night. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sun", meta = (ClampMin = "0.0"))
	float MoonIntensity = 0.3f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sun", meta = (ClampMin = "0.0"))
	float SkyLightDayIntensity = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sun", meta = (ClampMin = "0.0"))
	float SkyLightNightIntensity = 0.08f;

	// --- Gameplay (stealth) light, 0..1 ---

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gameplay Light", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float DayAmbientLightLevel = 0.25f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gameplay Light", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float NightAmbientLightLevel = 0.03f;

	/** Moonlight's share of the stealth setting's Directional Light Contribution (unshadowed, at night). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gameplay Light", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float MoonlightFactor = 0.2f;

private:

	void AdvanceTime();

	UFUNCTION()
	void OnRep_Hour();

	UPROPERTY(ReplicatedUsing = OnRep_Hour)
	float Hour = 14.0f;

	FTimerHandle AdvanceTimer;
	bool bWarnedAboutMobility = false;
};
