#include "Game/OutbreakGameUserSettings.h"

#include "Engine/Engine.h"
#include "Outbreak.h"

FOutbreakGameplaySettingsChangedSignature UOutbreakGameUserSettings::OnGameplaySettingsChanged;

UOutbreakGameUserSettings* UOutbreakGameUserSettings::GetOutbreakUserSettings()
{
	UGameUserSettings* Settings = GEngine ? GEngine->GetGameUserSettings() : nullptr;
	UOutbreakGameUserSettings* OutbreakSettings = Cast<UOutbreakGameUserSettings>(Settings);
	if (Settings && !OutbreakSettings)
	{
		static bool bWarned = false;
		if (!bWarned)
		{
			bWarned = true;
			UE_LOG(LogOutbreak, Warning, TEXT("GameUserSettingsClassName is %s, not OutbreakGameUserSettings; gameplay settings are unavailable."),
				*Settings->GetClass()->GetName());
		}
	}
	return OutbreakSettings;
}

float UOutbreakGameUserSettings::ClampLookSensitivity(float Value)
{
	return FMath::IsFinite(Value) ? FMath::Clamp(Value, MinLookSensitivity, MaxLookSensitivity) : 1.0f;
}

float UOutbreakGameUserSettings::ClampFieldOfView(float Value)
{
	if (!FMath::IsFinite(Value) || Value <= 0.0f)
	{
		return 0.0f;
	}
	return FMath::Clamp(Value, MinFieldOfView, MaxFieldOfView);
}

void UOutbreakGameUserSettings::SetToDefaults()
{
	Super::SetToDefaults();
	LookSensitivity = 1.0f;
	bInvertLookY = false;
	FieldOfView = 0.0f;
	bShowSubtitles = true;
}

void UOutbreakGameUserSettings::ValidateSettings()
{
	Super::ValidateSettings();
	// Load validation: values from an edited or damaged ini are brought back into range.
	const float LoadedSensitivity = LookSensitivity;
	const float LoadedFieldOfView = FieldOfView;
	LookSensitivity = ClampLookSensitivity(LookSensitivity);
	FieldOfView = ClampFieldOfView(FieldOfView);
	if (LoadedSensitivity != LookSensitivity || LoadedFieldOfView != FieldOfView)
	{
		UE_LOG(LogOutbreak, Warning, TEXT("Settings out of range were corrected (look sensitivity %f -> %f, field of view %f -> %f)."),
			LoadedSensitivity, LookSensitivity, LoadedFieldOfView, FieldOfView);
	}
}

void UOutbreakGameUserSettings::ApplySettings(bool bCheckForCommandLineOverrides)
{
	LookSensitivity = ClampLookSensitivity(LookSensitivity);
	FieldOfView = ClampFieldOfView(FieldOfView);
	// The engine applies graphics settings and saves the whole object to GameUserSettings.ini.
	Super::ApplySettings(bCheckForCommandLineOverrides);
	OnGameplaySettingsChanged.Broadcast();
}

void UOutbreakGameUserSettings::SetLookSensitivity(float NewSensitivity)
{
	LookSensitivity = ClampLookSensitivity(NewSensitivity);
}

void UOutbreakGameUserSettings::SetFieldOfView(float NewFieldOfView)
{
	FieldOfView = ClampFieldOfView(NewFieldOfView);
}
