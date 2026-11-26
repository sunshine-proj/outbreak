#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "Squad/OutbreakSquadComms.h"
#include "Squad/OutbreakSquadTypes.h"

#include "OutbreakSquadSettings.generated.h"

class AOutbreakTeammateCharacter;
class UOutbreakSquadConfiguration;
class UOutbreakVoiceDefinition;

/**
 * Project-wide squad settings: Project Settings → Game → Outbreak Squad, stored in
 * Config/DefaultGame.ini. These are the global limits every squad configuration must respect.
 */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Outbreak Squad"))
class OUTBREAK_API UOutbreakSquadSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:

	UOutbreakSquadSettings();

	virtual FName GetCategoryName() const override { return TEXT("Game"); }

	/** Global limits. Squad sizes include the player. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Limits")
	FOutbreakSquadSizeLimits SizeLimits;

	/** Used when the game mode has no squad configuration. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Defaults")
	TSoftObjectPtr<UOutbreakSquadConfiguration> DefaultSquadConfiguration;

	/** Spawned for teammates whose configuration does not name a character class. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Defaults")
	TSoftClassPtr<AOutbreakTeammateCharacter> DefaultTeammateClass;

	/** Distance between teammates when the squad is first spawned. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Spawning", meta = (ClampMin = "50.0", Units = "cm"))
	float InitialSpawnSpacing = 150.0f;

	/** Slot spacing per formation. Formations without an entry use DefaultFormationSpacing. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Formations")
	TArray<FOutbreakFormationSpacing> FormationSpacings;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Formations", meta = (ClampMin = "50.0", Units = "cm"))
	float DefaultFormationSpacing = 200.0f;

	float GetFormationSpacing(EOutbreakFormation Formation) const;

	/** In stealth mode, seconds without new compromising events before the squad goes quiet again. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Stealth", meta = (ClampMin = "0.0", Units = "s"))
	float CompromiseDuration = 15.0f;

	/** Voice for teammates whose role has none (Phase 13). */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Communication")
	TSoftObjectPtr<UOutbreakVoiceDefinition> DefaultVoice;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Communication")
	FOutbreakSquadCommsSettings Comms;
};
