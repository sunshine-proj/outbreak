#include "Missions/OutbreakAlarmSpeaker.h"

#include "Components/AudioComponent.h"
#include "Engine/World.h"
#include "Game/OutbreakGameState.h"
#include "Missions/OutbreakAlarmSubsystem.h"
#include "Sound/SoundBase.h"

AOutbreakAlarmSpeaker::AOutbreakAlarmSpeaker()
{
	PrimaryActorTick.bCanEverTick = false;

	Siren = CreateDefaultSubobject<UAudioComponent>(TEXT("Siren"));
	Siren->bAutoActivate = false;
	RootComponent = Siren;
}

void AOutbreakAlarmSpeaker::BeginPlay()
{
	Super::BeginPlay();

	Siren->SetSound(AlarmSound);
	if (AOutbreakGameState* GameState = GetWorld()->GetGameState<AOutbreakGameState>())
	{
		GameState->OnAlarmChanged.AddUniqueDynamic(this, &ThisClass::HandleAlarmChanged);
		if (GameState->IsAlarmRaised())
		{
			HandleAlarmChanged(true);
		}
	}
}

void AOutbreakAlarmSpeaker::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (AOutbreakGameState* GameState = GetWorld() ? GetWorld()->GetGameState<AOutbreakGameState>() : nullptr)
	{
		GameState->OnAlarmChanged.RemoveDynamic(this, &ThisClass::HandleAlarmChanged);
	}
	Super::EndPlay(EndPlayReason);
}

bool AOutbreakAlarmSpeaker::IsSounding() const
{
	return Siren->IsPlaying();
}

void AOutbreakAlarmSpeaker::HandleAlarmChanged(bool bAlarmRaised)
{
	if (GetNetMode() != NM_DedicatedServer && AlarmSound)
	{
		if (bAlarmRaised)
		{
			Siren->Play();
		}
		else
		{
			Siren->FadeOut(1.0f, 0.0f);
		}
	}
	OnAlarmStateChanged(bAlarmRaised);
}

void AOutbreakAlarmSpeaker::SetSwitchedOn_Implementation(bool bOn, AActor* SwitchedBy)
{
	// An alarm panel: activating it silences the alarm.
	UOutbreakAlarmSubsystem* Alarm = GetWorld() ? GetWorld()->GetSubsystem<UOutbreakAlarmSubsystem>() : nullptr;
	if (bOn && Alarm && HasAuthority())
	{
		Alarm->ResetAlarm();
	}
}
