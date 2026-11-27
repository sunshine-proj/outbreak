#include "Game/OutbreakGameUserSettings.h"

#include "Engine/Engine.h"
#include "Game/OutbreakAudioSettings.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundClass.h"
#include "Sound/SoundMix.h"
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
	MasterVolume = EffectsVolume = VoiceVolume = MusicVolume = 1.0f;
}

void UOutbreakGameUserSettings::ValidateSettings()
{
	Super::ValidateSettings();
	// Load validation: values from an edited or damaged ini are brought back into range.
	const float LoadedSensitivity = LookSensitivity;
	const float LoadedFieldOfView = FieldOfView;
	LookSensitivity = ClampLookSensitivity(LookSensitivity);
	FieldOfView = ClampFieldOfView(FieldOfView);
	for (float* Volume : { &MasterVolume, &EffectsVolume, &VoiceVolume, &MusicVolume })
	{
		*Volume = ClampVolume(*Volume);
	}
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
	for (float* Volume : { &MasterVolume, &EffectsVolume, &VoiceVolume, &MusicVolume })
	{
		*Volume = ClampVolume(*Volume);
	}
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

float UOutbreakGameUserSettings::ClampVolume(float Value)
{
	return FMath::IsFinite(Value) ? FMath::Clamp(Value, 0.0f, 1.0f) : 1.0f;
}

float* UOutbreakGameUserSettings::GetVolumeField(EOutbreakVolumeChannel Channel)
{
	switch (Channel)
	{
	case EOutbreakVolumeChannel::Effects:
		return &EffectsVolume;
	case EOutbreakVolumeChannel::Voice:
		return &VoiceVolume;
	case EOutbreakVolumeChannel::Music:
		return &MusicVolume;
	case EOutbreakVolumeChannel::Master:
	default:
		return &MasterVolume;
	}
}

float UOutbreakGameUserSettings::GetVolume(EOutbreakVolumeChannel Channel) const
{
	return *const_cast<UOutbreakGameUserSettings*>(this)->GetVolumeField(Channel);
}

void UOutbreakGameUserSettings::SetVolume(EOutbreakVolumeChannel Channel, float Volume)
{
	*GetVolumeField(Channel) = ClampVolume(Volume);
}

float UOutbreakGameUserSettings::ComputeClassVolume(float InMasterVolume, float ChannelVolume, EOutbreakVolumeChannel Channel)
{
	// Each class gets its effective volume directly (no reliance on mix inheritance between
	// parent and child adjusters), so the result does not depend on the class hierarchy.
	const float Master = ClampVolume(InMasterVolume);
	return Channel == EOutbreakVolumeChannel::Master ? Master : Master * ClampVolume(ChannelVolume);
}

void UOutbreakGameUserSettings::ApplyAudioVolumes(const UObject* WorldContext) const
{
	const UOutbreakAudioSettings* Audio = GetDefault<UOutbreakAudioSettings>();
	USoundMix* Mix = Audio->VolumeSoundMix.IsNull() ? nullptr : Audio->VolumeSoundMix.LoadSynchronous();
	if (!WorldContext || !Mix)
	{
		return;
	}

	const TPair<EOutbreakVolumeChannel, const TSoftObjectPtr<USoundClass>*> Classes[] = {
		{ EOutbreakVolumeChannel::Master, &Audio->MasterSoundClass },
		{ EOutbreakVolumeChannel::Effects, &Audio->EffectsSoundClass },
		{ EOutbreakVolumeChannel::Voice, &Audio->VoiceSoundClass },
		{ EOutbreakVolumeChannel::Music, &Audio->MusicSoundClass },
	};
	for (const auto& Entry : Classes)
	{
		USoundClass* SoundClass = Entry.Value->IsNull() ? nullptr : Entry.Value->LoadSynchronous();
		if (!SoundClass)
		{
			continue;
		}
		const float Volume = ComputeClassVolume(MasterVolume, GetVolume(Entry.Key), Entry.Key);
		// The master class applies to itself only; its children get Master x channel above.
		const bool bApplyToChildren = Entry.Key != EOutbreakVolumeChannel::Master;
		UGameplayStatics::SetSoundMixClassOverride(WorldContext, Mix, SoundClass, Volume, 1.0f, 0.0f, bApplyToChildren);
	}
	UGameplayStatics::PushSoundMixModifier(WorldContext, Mix);
}
