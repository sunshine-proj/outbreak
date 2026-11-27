#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "UObject/SoftObjectPtr.h"

#include "OutbreakAudioSettings.generated.h"

class USoundClass;
class USoundMix;

/**
 * Which sound class assets the player's volume settings drive (Phase 15): Project Settings →
 * Game → Outbreak Audio, stored in Config/DefaultGame.ini. Effects, Voice and Music should be
 * children of Master, and every sound should use one of them (Docs/Phase15-EditorSetup.md).
 * Unset entries are skipped, so volumes do nothing until the assets exist.
 */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Outbreak Audio"))
class OUTBREAK_API UOutbreakAudioSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:

	virtual FName GetCategoryName() const override { return TEXT("Game"); }

	/** The mix that carries the volume overrides. It is pushed once settings are applied. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Volume")
	TSoftObjectPtr<USoundMix> VolumeSoundMix;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Volume")
	TSoftObjectPtr<USoundClass> MasterSoundClass;

	/** Weapons, impacts, footsteps, doors, enemies, ambience. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Volume")
	TSoftObjectPtr<USoundClass> EffectsSoundClass;

	/** Squad callouts. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Volume")
	TSoftObjectPtr<USoundClass> VoiceSoundClass;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Volume")
	TSoftObjectPtr<USoundClass> MusicSoundClass;
};
