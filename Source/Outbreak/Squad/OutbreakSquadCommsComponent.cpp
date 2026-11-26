#include "Squad/OutbreakSquadCommsComponent.h"

#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Kismet/GameplayStatics.h"
#include "Missions/OutbreakAlarmSubsystem.h"
#include "Missions/OutbreakMissionSubsystem.h"
#include "Outbreak.h"
#include "Sound/SoundBase.h"
#include "Squad/OutbreakSquadComponent.h"
#include "Squad/OutbreakSquadMemberComponent.h"
#include "Squad/OutbreakSquadRoleDefinition.h"
#include "Squad/OutbreakSquadSettings.h"
#include "Weapons/OutbreakWeaponComponent.h"

#define LOCTEXT_NAMESPACE "OutbreakSquadComms"

UOutbreakSquadCommsComponent::UOutbreakSquadCommsComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UOutbreakSquadCommsComponent::BeginPlay()
{
	Super::BeginPlay();
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		BindEvents();
	}
}

UOutbreakSquadComponent* UOutbreakSquadCommsComponent::GetSquad() const
{
	return GetOwner() ? GetOwner()->FindComponentByClass<UOutbreakSquadComponent>() : nullptr;
}

double UOutbreakSquadCommsComponent::GetTimeSeconds() const
{
	const UWorld* World = GetWorld();
	return World ? World->GetTimeSeconds() : 0.0;
}

void UOutbreakSquadCommsComponent::BindEvents()
{
	UOutbreakSquadComponent* Squad = GetSquad();
	if (bBound || !Squad)
	{
		return;
	}
	bBound = true;

	Squad->OnMemberAdded.AddUniqueDynamic(this, &ThisClass::HandleMemberAdded);
	Squad->OnMemberStatusChanged.AddUniqueDynamic(this, &ThisClass::HandleMemberStatusChanged);
	Squad->OnCommandAccepted.AddUniqueDynamic(this, &ThisClass::HandleCommandAccepted);
	for (UOutbreakSquadMemberComponent* Member : Squad->GetMembers())
	{
		BindMemberWeapon(Member);
	}

	if (UWorld* World = GetWorld())
	{
		if (UOutbreakAlarmSubsystem* Alarm = World->GetSubsystem<UOutbreakAlarmSubsystem>())
		{
			Alarm->OnAlarmRaised.AddUniqueDynamic(this, &ThisClass::HandleAlarmRaised);
		}
		if (UOutbreakMissionSubsystem* Mission = World->GetSubsystem<UOutbreakMissionSubsystem>())
		{
			Mission->OnObjectiveStateChanged.AddUniqueDynamic(this, &ThisClass::HandleObjectiveStateChanged);
		}
	}
}

void UOutbreakSquadCommsComponent::BindMemberWeapon(UOutbreakSquadMemberComponent* Member)
{
	if (Member && !Member->IsPlayerMember())
	{
		if (UOutbreakWeaponComponent* Weapons = Member->GetWeaponComponent())
		{
			Weapons->OnWeaponAction.AddUniqueDynamic(this, &ThisClass::HandleWeaponAction);
		}
	}
}

UOutbreakSquadCommsComponent* UOutbreakSquadCommsComponent::FindFor(const UOutbreakSquadMemberComponent* Member)
{
	const UOutbreakSquadComponent* Squad = Member ? Member->GetSquad() : nullptr;
	const AActor* SquadOwner = Squad ? Squad->GetOwner() : nullptr;
	return SquadOwner ? SquadOwner->FindComponentByClass<UOutbreakSquadCommsComponent>() : nullptr;
}

const UOutbreakVoiceDefinition* UOutbreakSquadCommsComponent::GetVoiceOf(const UOutbreakSquadMemberComponent* Member)
{
	const UOutbreakSquadRoleDefinition* Role = Member ? Member->GetRole() : nullptr;
	if (Role && Role->Voice)
	{
		return Role->Voice;
	}
	const UOutbreakSquadSettings* Settings = GetDefault<UOutbreakSquadSettings>();
	return Settings->DefaultVoice.IsNull() ? nullptr : Settings->DefaultVoice.LoadSynchronous();
}

UOutbreakSquadMemberComponent* UOutbreakSquadCommsComponent::PickSpeaker(UOutbreakSquadMemberComponent* Preferred,
	const UOutbreakSquadMemberComponent* Subject) const
{
	auto CanSpeak = [Subject](const UOutbreakSquadMemberComponent* Member)
	{
		return Member && Member != Subject && !Member->IsPlayerMember() && Member->GetStatus() == EOutbreakSquadMemberStatus::Active;
	};
	if (CanSpeak(Preferred))
	{
		return Preferred;
	}
	const UOutbreakSquadComponent* Squad = GetSquad();
	if (!Squad)
	{
		return nullptr;
	}
	for (UOutbreakSquadMemberComponent* Member : Squad->GetAvailableTeammates())
	{
		if (CanSpeak(Member))
		{
			return Member;
		}
	}
	return nullptr;
}

bool UOutbreakSquadCommsComponent::RequestCallout(EOutbreakCallout Callout, UOutbreakSquadMemberComponent* Speaker,
	UOutbreakSquadMemberComponent* Subject)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return false;
	}

	const FOutbreakSquadCommsSettings& Settings = GetDefault<UOutbreakSquadSettings>()->Comms;
	const double Now = GetTimeSeconds();
	const double* LastSame = LastCalloutTimes.Find(Callout);
	if (!OutbreakSquadComms::IsCalloutAllowed(Callout, Now, LastAnyCalloutTime, LastSame ? *LastSame : -1.0e9, Settings))
	{
		return false;
	}

	UOutbreakSquadMemberComponent* Chosen = PickSpeaker(Speaker, Subject);
	const UOutbreakVoiceDefinition* Voice = GetVoiceOf(Chosen);
	const FOutbreakVoiceLines* Lines = Voice ? Voice->Lines.Find(Callout) : nullptr;
	if (!Chosen || !Lines || Lines->GetVariantCount() == 0)
	{
		return false;
	}

	LastAnyCalloutTime = Now;
	LastCalloutTimes.Add(Callout, Now);
	MulticastCallout(Chosen, Subject, Callout, FMath::RandRange(0, Lines->GetVariantCount() - 1));
	return true;
}

void UOutbreakSquadCommsComponent::MulticastCallout_Implementation(UOutbreakSquadMemberComponent* Speaker,
	UOutbreakSquadMemberComponent* Subject, EOutbreakCallout Callout, int32 Variant)
{
	const UOutbreakVoiceDefinition* Voice = GetVoiceOf(Speaker);
	const FOutbreakVoiceLines* Lines = Voice ? Voice->Lines.Find(Callout) : nullptr;
	if (!Speaker || !Lines)
	{
		return;
	}

	const UWorld* World = GetWorld();
	USoundBase* Sound = Lines->Sounds.IsValidIndex(Variant) ? Lines->Sounds[Variant].Get() : nullptr;
	AActor* SpeakerActor = Speaker->GetOwner();
	if (Sound && SpeakerActor && World && World->GetNetMode() != NM_DedicatedServer)
	{
		UGameplayStatics::SpawnSoundAttached(Sound, SpeakerActor->GetRootComponent());
	}

	LastSubtitle = Lines->Subtitles.IsValidIndex(Variant)
		? OutbreakSquadComms::FormatSubtitle(Speaker->GetDisplayName(), Lines->Subtitles[Variant],
			Subject ? Subject->GetDisplayName() : FText::GetEmpty())
		: FText::GetEmpty();
	OnCallout.Broadcast(Speaker, Callout, LastSubtitle);
}

// --- Event sources -------------------------------------------------------------------------------

void UOutbreakSquadCommsComponent::HandleMemberAdded(UOutbreakSquadComponent* Squad, UOutbreakSquadMemberComponent* Member)
{
	BindMemberWeapon(Member);
}

void UOutbreakSquadCommsComponent::HandleMemberStatusChanged(UOutbreakSquadComponent* Squad, UOutbreakSquadMemberComponent* Member,
	EOutbreakSquadMemberStatus NewStatus, EOutbreakSquadMemberStatus OldStatus)
{
	if (NewStatus == EOutbreakSquadMemberStatus::Incapacitated)
	{
		RequestCallout(EOutbreakCallout::ManDown, nullptr, Member);
	}
	else if (NewStatus == EOutbreakSquadMemberStatus::Dead)
	{
		RequestCallout(EOutbreakCallout::MemberKilled, nullptr, Member);
	}
}

void UOutbreakSquadCommsComponent::HandleCommandAccepted(UOutbreakSquadComponent* Squad, EOutbreakSquadCommandType CommandType,
	UOutbreakSquadMemberComponent* FirstAcceptedMember)
{
	RequestCallout(EOutbreakCallout::Acknowledge, FirstAcceptedMember);
}

void UOutbreakSquadCommsComponent::HandleWeaponAction(UOutbreakWeaponComponent* WeaponComponent, EOutbreakWeaponAction Action,
	UOutbreakWeaponDefinition* Weapon)
{
	if (Action != EOutbreakWeaponAction::ReloadStarted || !WeaponComponent || !WeaponComponent->GetOwner())
	{
		return;
	}
	if (UOutbreakSquadMemberComponent* Member = WeaponComponent->GetOwner()->FindComponentByClass<UOutbreakSquadMemberComponent>())
	{
		RequestCallout(EOutbreakCallout::Reloading, Member);
	}
}

void UOutbreakSquadCommsComponent::HandleAlarmRaised(FVector Location)
{
	RequestCallout(EOutbreakCallout::AlarmRaised);
}

void UOutbreakSquadCommsComponent::HandleObjectiveStateChanged(int32 ObjectiveIndex, EOutbreakObjectiveState NewState)
{
	if (NewState == EOutbreakObjectiveState::Completed)
	{
		RequestCallout(EOutbreakCallout::ObjectiveComplete);
	}
}

#undef LOCTEXT_NAMESPACE
