#pragma once

#include "CoreMinimal.h"
#include "Combat/OutbreakHealthComponent.h"
#include "Components/ActorComponent.h"
#include "Squad/OutbreakSquadTypes.h"

#include "OutbreakSquadMemberComponent.generated.h"

class UOutbreakSquadComponent;
class UOutbreakSquadMemberComponent;
class UOutbreakSquadRoleDefinition;
class UOutbreakWeaponComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOutbreakSquadMemberStatusChangedSignature,
	UOutbreakSquadMemberComponent*, Member, EOutbreakSquadMemberStatus, NewStatus, EOutbreakSquadMemberStatus, OldStatus);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOutbreakSquadMemberChangedSignature, UOutbreakSquadMemberComponent*, Member);

/**
 * Makes its owner a squad member. Attached to the player and to AI teammates (D-002), so the
 * squad stores members without caring whether a human or the AI controls them.
 *
 * Holds the member's identity (ID, name, role) and derives its status from the owner's health
 * component. Membership itself is managed by UOutbreakSquadComponent: use its RegisterMember and
 * RemoveMember. Identity and squad replicate; status follows the replicated health.
 */
UCLASS(ClassGroup = (Outbreak), meta = (BlueprintSpawnableComponent))
class OUTBREAK_API UOutbreakSquadMemberComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	UOutbreakSquadMemberComponent();

	virtual void InitializeComponent() override;
	virtual void UninitializeComponent() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** Squad-unique ID assigned on registration. INDEX_NONE while not in a squad. */
	UFUNCTION(BlueprintPure, Category = "Squad")
	int32 GetMemberId() const { return MemberId; }

	UFUNCTION(BlueprintPure, Category = "Squad")
	UOutbreakSquadComponent* GetSquad() const { return Squad; }

	UFUNCTION(BlueprintPure, Category = "Squad")
	bool IsInSquad() const { return Squad != nullptr; }

	UFUNCTION(BlueprintPure, Category = "Squad")
	FText GetDisplayName() const { return DisplayName; }

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Squad")
	void SetDisplayName(const FText& NewDisplayName);

	UFUNCTION(BlueprintPure, Category = "Squad")
	UOutbreakSquadRoleDefinition* GetRole() const { return Role; }

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Squad")
	void SetRole(UOutbreakSquadRoleDefinition* NewRole);

	/** True for the human-controlled member. */
	UFUNCTION(BlueprintPure, Category = "Squad")
	bool IsPlayerMember() const;

	UFUNCTION(BlueprintPure, Category = "Squad")
	EOutbreakSquadMemberStatus GetStatus() const;

	/** In a squad and Active: able to take orders (Phase 4). */
	UFUNCTION(BlueprintPure, Category = "Squad")
	bool IsAvailable() const;

	/** 0..1. 1 when the owner has no health component. */
	UFUNCTION(BlueprintPure, Category = "Squad")
	float GetHealthFraction() const;

	UFUNCTION(BlueprintPure, Category = "Squad")
	FVector GetMemberLocation() const;

	UFUNCTION(BlueprintPure, Category = "Squad")
	UOutbreakHealthComponent* GetHealthComponent() const { return HealthComponent; }

	UFUNCTION(BlueprintPure, Category = "Squad")
	UOutbreakWeaponComponent* GetWeaponComponent() const { return WeaponComponent; }

	/** Fires on every machine when the member's status changes. */
	UPROPERTY(BlueprintAssignable, Category = "Squad")
	FOutbreakSquadMemberStatusChangedSignature OnStatusChanged;

	/** Fires on every machine when the squad, ID, name or role changes. */
	UPROPERTY(BlueprintAssignable, Category = "Squad")
	FOutbreakSquadMemberChangedSignature OnMemberInfoChanged;

private:

	friend class UOutbreakSquadComponent;

	/** Called by UOutbreakSquadComponent on registration and removal (authority). */
	void SetSquadMembership(UOutbreakSquadComponent* NewSquad, int32 NewMemberId);

	UFUNCTION()
	void HandleHealthStateChanged(UOutbreakHealthComponent* InHealthComponent, EOutbreakHealthState NewState, EOutbreakHealthState OldState);

	UFUNCTION()
	void OnRep_MemberInfo();

	static EOutbreakSquadMemberStatus ToMemberStatus(EOutbreakHealthState HealthState);

	UPROPERTY(ReplicatedUsing = OnRep_MemberInfo)
	TObjectPtr<UOutbreakSquadComponent> Squad;

	UPROPERTY(ReplicatedUsing = OnRep_MemberInfo)
	int32 MemberId = INDEX_NONE;

	UPROPERTY(ReplicatedUsing = OnRep_MemberInfo)
	FText DisplayName;

	UPROPERTY(ReplicatedUsing = OnRep_MemberInfo)
	TObjectPtr<UOutbreakSquadRoleDefinition> Role;

	UPROPERTY(Transient)
	TObjectPtr<UOutbreakHealthComponent> HealthComponent;

	UPROPERTY(Transient)
	TObjectPtr<UOutbreakWeaponComponent> WeaponComponent;
};
