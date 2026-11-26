#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "Squad/OutbreakSquadTypes.h"

#include "OutbreakSquadSettings.generated.h"

class AOutbreakTeammateCharacter;
class UOutbreakSquadConfiguration;

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

	/** Distance between teammates when the squad is first spawned (formations come in Phase 7). */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Spawning", meta = (ClampMin = "50.0", Units = "cm"))
	float InitialSpawnSpacing = 150.0f;
};
