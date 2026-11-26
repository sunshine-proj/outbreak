#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Missions/OutbreakMissionTypes.h"
#include "Squad/OutbreakSquadCommandTypes.h"
#include "Squad/OutbreakSquadComms.h"
#include "Squad/OutbreakSquadTypes.h"
#include "Weapons/OutbreakWeaponTypes.h"

#include "OutbreakSquadCommsComponent.generated.h"

class UOutbreakSquadComponent;
class UOutbreakSquadMemberComponent;
class UOutbreakVoiceDefinition;
class UOutbreakWeaponComponent;
class UOutbreakWeaponDefinition;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOutbreakCalloutSignature,
	UOutbreakSquadMemberComponent*, Speaker, EOutbreakCallout, Callout, const FText&, Subtitle);

/**
 * Squad communication (Phase 13): teammates call out orders acknowledged, contacts, reloads,
 * squadmates down or killed, the alarm and completed objectives, with a voice line and a subtitle.
 *
 * Lives on AOutbreakPlayerState next to the squad. The server decides what is said (events happen
 * there), paces it (FOutbreakSquadCommsSettings) and multicasts the chosen line, so every machine
 * hears it from the speaker and the owning player sees the subtitle (OnCallout).
 */
UCLASS(ClassGroup = (Outbreak), meta = (BlueprintSpawnableComponent))
class OUTBREAK_API UOutbreakSquadCommsComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	UOutbreakSquadCommsComponent();

	/** Authority: subscribes to the squad, alarm and mission events. Called on BeginPlay. */
	void BindEvents();

	/**
	 * Authority: has Speaker (null = any standing teammate other than Subject) say Callout, about
	 * Subject. Returns false when paced out or nobody can say it.
	 */
	bool RequestCallout(EOutbreakCallout Callout, UOutbreakSquadMemberComponent* Speaker = nullptr,
		UOutbreakSquadMemberComponent* Subject = nullptr);

	/** The comms of the squad Member belongs to, or null. */
	static UOutbreakSquadCommsComponent* FindFor(const UOutbreakSquadMemberComponent* Member);

	/** The voice Member speaks with: its role's, else the project default. */
	static const UOutbreakVoiceDefinition* GetVoiceOf(const UOutbreakSquadMemberComponent* Member);

	/** Last subtitle shown on this machine (debugging and tests). */
	const FText& GetLastSubtitle() const { return LastSubtitle; }

	/** Every machine: someone spoke. */
	UPROPERTY(BlueprintAssignable, Category = "Squad|Comms")
	FOutbreakCalloutSignature OnCallout;

protected:

	virtual void BeginPlay() override;

private:

	UFUNCTION(NetMulticast, Unreliable)
	void MulticastCallout(UOutbreakSquadMemberComponent* Speaker, UOutbreakSquadMemberComponent* Subject, EOutbreakCallout Callout, int32 Variant);

	UOutbreakSquadComponent* GetSquad() const;
	UOutbreakSquadMemberComponent* PickSpeaker(UOutbreakSquadMemberComponent* Preferred, const UOutbreakSquadMemberComponent* Subject) const;
	double GetTimeSeconds() const;
	void BindMemberWeapon(UOutbreakSquadMemberComponent* Member);

	UFUNCTION()
	void HandleMemberAdded(UOutbreakSquadComponent* Squad, UOutbreakSquadMemberComponent* Member);

	UFUNCTION()
	void HandleMemberStatusChanged(UOutbreakSquadComponent* Squad, UOutbreakSquadMemberComponent* Member,
		EOutbreakSquadMemberStatus NewStatus, EOutbreakSquadMemberStatus OldStatus);

	UFUNCTION()
	void HandleCommandAccepted(UOutbreakSquadComponent* Squad, EOutbreakSquadCommandType CommandType, UOutbreakSquadMemberComponent* FirstAcceptedMember);

	UFUNCTION()
	void HandleWeaponAction(UOutbreakWeaponComponent* WeaponComponent, EOutbreakWeaponAction Action, UOutbreakWeaponDefinition* Weapon);

	UFUNCTION()
	void HandleAlarmRaised(FVector Location);

	UFUNCTION()
	void HandleObjectiveStateChanged(int32 ObjectiveIndex, EOutbreakObjectiveState NewState);

	TMap<EOutbreakCallout, double> LastCalloutTimes;
	double LastAnyCalloutTime = -1.0e9;
	FText LastSubtitle;
	bool bBound = false;
};
