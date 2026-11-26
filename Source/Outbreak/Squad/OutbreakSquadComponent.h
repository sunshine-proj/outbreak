#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Squad/OutbreakSquadCommandTypes.h"
#include "Squad/OutbreakSquadTypes.h"

#include "OutbreakSquadComponent.generated.h"

class APawn;
class AOutbreakCharacter;
class UOutbreakSquadComponent;
class UOutbreakSquadConfiguration;
class UOutbreakSquadMemberComponent;
struct FOutbreakSquadMemberConfig;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOutbreakSquadMembershipChangedSignature,
	UOutbreakSquadComponent*, Squad, UOutbreakSquadMemberComponent*, Member);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOutbreakSquadMemberStatusEventSignature,
	UOutbreakSquadComponent*, Squad, UOutbreakSquadMemberComponent*, Member,
	EOutbreakSquadMemberStatus, NewStatus, EOutbreakSquadMemberStatus, OldStatus);

/**
 * A player's squad: the dynamic collection of member components, the player included.
 *
 * Lives on AOutbreakPlayerState, so the squad outlives the player's pawn and replicates to
 * every client (D-009). Server-authoritative: membership changes only on the authority.
 *
 * Responsible for registration, removal, composition, size limits and member status events.
 * Orders and their distribution arrive in Phase 4; individual AI behaviour stays in AI controllers.
 * Nothing here assumes a particular squad size.
 */
UCLASS(ClassGroup = (Outbreak), meta = (BlueprintSpawnableComponent))
class OUTBREAK_API UOutbreakSquadComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	UOutbreakSquadComponent();

	virtual void InitializeComponent() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// --- Initialization (authority) ---

	/**
	 * Builds the squad: resolves limits and size from the project settings and Configuration
	 * (null = project default configuration, or default-sized generic teammates), registers
	 * PlayerPawn's member component as the player member, then spawns and equips SquadSize - 1
	 * teammates near the player.
	 *
	 * RequestedSizeOverride > 0 replaces the configuration's SquadSize (for test runs).
	 * Returns false if the limits or size are invalid; the player is still registered so the game
	 * remains playable, and the error is logged.
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Squad")
	bool InitializeSquad(const UOutbreakSquadConfiguration* Configuration, APawn* PlayerPawn, int32 RequestedSizeOverride = 0);

	/** True once InitializeSquad has run. */
	UFUNCTION(BlueprintPure, Category = "Squad")
	bool IsSquadInitialized() const { return bSquadInitialized; }

	// --- Membership (authority) ---

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Squad")
	EOutbreakSquadRegistrationResult RegisterMember(UOutbreakSquadMemberComponent* Member);

	/** Registers Member and marks it as the human player's member. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Squad")
	EOutbreakSquadRegistrationResult RegisterPlayerMember(UOutbreakSquadMemberComponent* Member);

	/** Removes Member. The actor is not destroyed. Returns false if it was not a member. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Squad")
	bool RemoveMember(UOutbreakSquadMemberComponent* Member);

	/** Replaces the size limits (normally set by InitializeSquad). Ignored if invalid. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Squad")
	bool SetSizeLimits(const FOutbreakSquadSizeLimits& NewLimits);

	// --- Commands (owning client or authority) ---

	/**
	 * Gives Command to Recipients. On a client this forwards to the server. The server validates
	 * the command for each recipient (ValidateCommandForMember); accepted members replace their
	 * current order. The player member never receives commands; Squad scope means every teammate.
	 */
	UFUNCTION(BlueprintCallable, Category = "Squad|Orders")
	void IssueCommand(const FOutbreakSquadCommand& Command, const FOutbreakCommandRecipients& Recipients);

	/** Cancels the Recipients' active orders; they return to autonomous behaviour. */
	UFUNCTION(BlueprintCallable, Category = "Squad|Orders")
	void CancelCommand(const FOutbreakCommandRecipients& Recipients);

	/** Authority implementation of IssueCommand. Returns one result per resolved recipient. */
	TArray<FOutbreakCommandResult> AuthIssueCommand(const FOutbreakSquadCommand& Command, const FOutbreakCommandRecipients& Recipients);

	/** Authority implementation of CancelCommand. Returns the number of orders cancelled. */
	int32 AuthCancelCommand(const FOutbreakCommandRecipients& Recipients);

	/** Members a recipient set refers to. Squad scope excludes the player; unknown IDs are skipped. */
	TArray<UOutbreakSquadMemberComponent*> ResolveRecipients(const FOutbreakCommandRecipients& Recipients) const;

	/** Why Member would refuse Command, or None. Usable on clients for immediate feedback. */
	EOutbreakCommandRejection ValidateCommandForMember(const FOutbreakSquadCommand& Command, const UOutbreakSquadMemberComponent* Member) const;

	/** Teammates (non-player members) in squad order. Their 1-based position is their HUD slot. */
	UFUNCTION(BlueprintPure, Category = "Squad")
	TArray<UOutbreakSquadMemberComponent*> GetTeammates() const;

	// --- Queries ---

	UFUNCTION(BlueprintPure, Category = "Squad")
	TArray<UOutbreakSquadMemberComponent*> GetMembers() const;

	/** Registered members, the player included, whatever their status. */
	UFUNCTION(BlueprintPure, Category = "Squad")
	int32 GetMemberCount() const { return Members.Num(); }

	/** Members that are not dead (Active or Incapacitated). */
	UFUNCTION(BlueprintPure, Category = "Squad")
	int32 GetAliveMemberCount() const;

	/** Active members other than the player: the ones that can take orders. */
	UFUNCTION(BlueprintPure, Category = "Squad")
	TArray<UOutbreakSquadMemberComponent*> GetAvailableTeammates() const;

	UFUNCTION(BlueprintPure, Category = "Squad")
	UOutbreakSquadMemberComponent* GetPlayerMember() const { return PlayerMember; }

	UFUNCTION(BlueprintPure, Category = "Squad")
	UOutbreakSquadMemberComponent* FindMemberById(int32 MemberId) const;

	UFUNCTION(BlueprintPure, Category = "Squad")
	bool ContainsMember(const UOutbreakSquadMemberComponent* Member) const;

	UFUNCTION(BlueprintPure, Category = "Squad")
	FOutbreakSquadSizeLimits GetSizeLimits() const { return SizeLimits; }

	UFUNCTION(BlueprintPure, Category = "Squad")
	bool IsFull() const { return Members.Num() >= SizeLimits.MaxSquadSize; }

	/** Composition is valid: player registered, member count within limits, no invalid entries. */
	UFUNCTION(BlueprintPure, Category = "Squad")
	bool IsCompositionValid() const;

	/** Like IsCompositionValid, with a reason for each problem. */
	bool ValidateComposition(TArray<FText>& OutErrors) const;

	/** Distance from the player member to Member, or -1 without a player member. */
	UFUNCTION(BlueprintPure, Category = "Squad")
	float GetDistanceToPlayer(const UOutbreakSquadMemberComponent* Member) const;

	// --- Events (every machine) ---

	UPROPERTY(BlueprintAssignable, Category = "Squad")
	FOutbreakSquadMembershipChangedSignature OnMemberAdded;

	UPROPERTY(BlueprintAssignable, Category = "Squad")
	FOutbreakSquadMembershipChangedSignature OnMemberRemoved;

	/** A member became incapacitated, died or recovered. */
	UPROPERTY(BlueprintAssignable, Category = "Squad")
	FOutbreakSquadMemberStatusEventSignature OnMemberStatusChanged;

private:

	UFUNCTION(Server, Reliable)
	void ServerIssueCommand(const FOutbreakSquadCommand& Command, const FOutbreakCommandRecipients& Recipients);

	UFUNCTION(Server, Reliable)
	void ServerCancelCommand(const FOutbreakCommandRecipients& Recipients);

	EOutbreakSquadRegistrationResult RegisterMemberInternal(UOutbreakSquadMemberComponent* Member, bool bIsPlayer);
	AOutbreakCharacter* SpawnTeammate(const FOutbreakSquadMemberConfig& MemberConfig, const APawn* PlayerPawn, int32 TeammateIndex);
	void ApplyMemberConfig(AOutbreakCharacter* Teammate, const FOutbreakSquadMemberConfig& MemberConfig) const;
	bool HasOwnerAuthority() const;

	void BindMember(UOutbreakSquadMemberComponent* Member);
	void UnbindMember(UOutbreakSquadMemberComponent* Member);

	UFUNCTION()
	void HandleMemberStatusChanged(UOutbreakSquadMemberComponent* Member, EOutbreakSquadMemberStatus NewStatus, EOutbreakSquadMemberStatus OldStatus);

	UFUNCTION()
	void OnRep_Members();

	UPROPERTY(ReplicatedUsing = OnRep_Members)
	TArray<TObjectPtr<UOutbreakSquadMemberComponent>> Members;

	UPROPERTY(Replicated)
	TObjectPtr<UOutbreakSquadMemberComponent> PlayerMember;

	UPROPERTY(Replicated)
	FOutbreakSquadSizeLimits SizeLimits;

	/** Client copy of Members before the last replication, to detect additions and removals. */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UOutbreakSquadMemberComponent>> PreviousMembers;

	int32 NextMemberId = 1;
	bool bSquadInitialized = false;
};
