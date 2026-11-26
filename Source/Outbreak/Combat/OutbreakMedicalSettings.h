#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"

#include "OutbreakMedicalSettings.generated.h"

/**
 * Project-wide incapacitation and revive rules (Project Settings → Game → Outbreak Medical).
 * Medical items carry their own heal amounts; teammate AI thresholds are on UOutbreakTeammateAIConfig.
 * "Deliberately simple" (PRD §9): down at zero health, bleed out unless revived, limited revives.
 */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Outbreak Medical"))
class OUTBREAK_API UOutbreakMedicalSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:

	virtual FName GetCategoryName() const override { return TEXT("Game"); }

	/** Squad soldiers (player and teammates) are incapacitated at zero health instead of dying. */
	UPROPERTY(Config, EditAnywhere, Category = "Incapacitation")
	bool bSquadCanBeIncapacitated = true;

	/** Seconds an incapacitated soldier lasts before dying. 0 = until revived or finished off. */
	UPROPERTY(Config, EditAnywhere, Category = "Incapacitation", meta = (ClampMin = "0.0", Units = "s"))
	float BleedOutTime = 30.0f;

	/** Revives each soldier can receive per mission; once used up, zero health kills. 0 = unlimited. */
	UPROPERTY(Config, EditAnywhere, Category = "Incapacitation", meta = (ClampMin = "0"))
	int32 MaxRevivesPerSoldier = 2;

	/** Seconds a revive takes (before the reviver's medical multiplier). */
	UPROPERTY(Config, EditAnywhere, Category = "Revive", meta = (ClampMin = "0.0", Units = "s"))
	float ReviveDuration = 4.0f;

	/** The reviver must stay within this distance of the downed soldier. */
	UPROPERTY(Config, EditAnywhere, Category = "Revive", meta = (ClampMin = "50.0", Units = "cm"))
	float ReviveRange = 200.0f;

	/** Health after a revive, as a fraction of maximum health. */
	UPROPERTY(Config, EditAnywhere, Category = "Revive", meta = (ClampMin = "0.01", ClampMax = "1.0"))
	float ReviveHealthFraction = 0.3f;

	/** Items are used on another soldier within this distance. */
	UPROPERTY(Config, EditAnywhere, Category = "Healing", meta = (ClampMin = "50.0", Units = "cm"))
	float MedicalUseRange = 250.0f;
};
