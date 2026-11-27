#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameUserSettings.h"

#include "OutbreakGameUserSettings.generated.h"

DECLARE_MULTICAST_DELEGATE(FOutbreakGameplaySettingsChangedSignature);

/**
 * Player settings (Phase 14 settings persistence, D-023). Extends the engine's user settings, so
 * graphics, resolution and window mode persist as they always do and the gameplay settings below
 * are stored with them in Saved/Config/<Platform>/GameUserSettings.ini. Selected through
 * Config/DefaultEngine.ini (GameUserSettingsClassName).
 *
 * Values are clamped when set and again when loaded (ValidateSettings), so an edited or damaged
 * file cannot produce unusable controls. ApplySettings saves and notifies players.
 */
UCLASS(Config = GameUserSettings, ConfigDoNotCheckDefaults)
class OUTBREAK_API UOutbreakGameUserSettings : public UGameUserSettings
{
	GENERATED_BODY()

public:

	/** The engine's settings object as Outbreak settings; null when the engine uses another class (logged once). */
	UFUNCTION(BlueprintPure, Category = "Settings")
	static UOutbreakGameUserSettings* GetOutbreakUserSettings();

	virtual void SetToDefaults() override;
	virtual void ValidateSettings() override;
	virtual void ApplySettings(bool bCheckForCommandLineOverrides) override;

	UFUNCTION(BlueprintPure, Category = "Settings|Controls")
	float GetLookSensitivity() const { return LookSensitivity; }

	UFUNCTION(BlueprintCallable, Category = "Settings|Controls")
	void SetLookSensitivity(float NewSensitivity);

	UFUNCTION(BlueprintPure, Category = "Settings|Controls")
	bool IsLookYInverted() const { return bInvertLookY; }

	UFUNCTION(BlueprintCallable, Category = "Settings|Controls")
	void SetInvertLookY(bool bInvert) { bInvertLookY = bInvert; }

	/** Degrees; 0 = the character's own default field of view. */
	UFUNCTION(BlueprintPure, Category = "Settings|View")
	float GetFieldOfView() const { return FieldOfView; }

	/** 0 = character default; otherwise clamped to MinFieldOfView..MaxFieldOfView. */
	UFUNCTION(BlueprintCallable, Category = "Settings|View")
	void SetFieldOfView(float NewFieldOfView);

	UFUNCTION(BlueprintPure, Category = "Settings|Audio")
	bool AreSubtitlesShown() const { return bShowSubtitles; }

	UFUNCTION(BlueprintCallable, Category = "Settings|Audio")
	void SetShowSubtitles(bool bShow) { bShowSubtitles = bShow; }

	/** Fires after ApplySettings, so players pick up the new values. */
	static FOutbreakGameplaySettingsChangedSignature OnGameplaySettingsChanged;

	static constexpr float MinLookSensitivity = 0.1f;
	static constexpr float MaxLookSensitivity = 5.0f;
	static constexpr float MinFieldOfView = 60.0f;
	static constexpr float MaxFieldOfView = 120.0f;

	/** The clamping rules, shared by the setters and loading. */
	static float ClampLookSensitivity(float Value);
	static float ClampFieldOfView(float Value);

protected:

	UPROPERTY(Config)
	float LookSensitivity = 1.0f;

	UPROPERTY(Config)
	bool bInvertLookY = false;

	UPROPERTY(Config)
	float FieldOfView = 0.0f;

	UPROPERTY(Config)
	bool bShowSubtitles = true;
};
