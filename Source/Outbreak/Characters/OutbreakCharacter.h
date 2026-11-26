#pragma once

#include "CoreMinimal.h"
#include "Characters/OutbreakMovementSettings.h"
#include "Combat/OutbreakHealthComponent.h"
#include "Combat/OutbreakTeam.h"
#include "GameFramework/Character.h"
#include "GenericTeamAgentInterface.h"

#include "OutbreakCharacter.generated.h"

class UOutbreakSquadMemberComponent;
class UOutbreakWeaponComponent;

/**
 * Base for every Outbreak soldier: the player (AOutbreakPlayerCharacter) and AI teammates
 * (AOutbreakTeammateCharacter).
 *
 * Owns movement states (walk, sprint, crouch) and the components every soldier has: health,
 * weapons and squad membership. Reacts to its own health: when downed it stops moving and its
 * weapons are disabled. Player-only concerns (camera, input, interaction) live in the subclass.
 */
UCLASS(Abstract)
class OUTBREAK_API AOutbreakCharacter : public ACharacter, public IGenericTeamAgentInterface
{
	GENERATED_BODY()

public:

	AOutbreakCharacter();

	//~ Begin AActor / ACharacter interface
	virtual void PostInitializeComponents() override;
	virtual void OnStartCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust) override;
	virtual void OnEndCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust) override;
	//~ End AActor / ACharacter interface

	//~ Begin IGenericTeamAgentInterface interface
	virtual FGenericTeamId GetGenericTeamId() const override;
	//~ End IGenericTeamAgentInterface interface

	/** Starts sprinting if allowed. Returns true if the character is sprinting afterwards. */
	UFUNCTION(BlueprintCallable, Category = "Movement")
	bool StartSprinting();

	UFUNCTION(BlueprintCallable, Category = "Movement")
	void StopSprinting();

	UFUNCTION(BlueprintPure, Category = "Movement")
	bool IsSprinting() const { return bIsSprinting; }

	/** Sprinting is not allowed while crouched, while a crouch is pending, or while downed. */
	UFUNCTION(BlueprintPure, Category = "Movement")
	bool CanSprint() const;

	/** Stops sprinting and requests a crouch. */
	UFUNCTION(BlueprintCallable, Category = "Movement")
	void BeginCrouch();

	/** Requests standing up. */
	UFUNCTION(BlueprintCallable, Category = "Movement")
	void EndCrouch();

	/** True when crouched or when a crouch has been requested but not yet applied by movement. */
	UFUNCTION(BlueprintPure, Category = "Movement")
	bool IsCrouchedOrCrouchRequested() const;

	/** Pushes MovementSettings to the movement component. Called automatically on initialization. */
	UFUNCTION(BlueprintCallable, Category = "Movement")
	void ApplyMovementSettings();

	const FOutbreakMovementSettings& GetMovementSettings() const { return MovementSettings; }

	UOutbreakHealthComponent* GetHealthComponent() const { return HealthComponent; }

	UOutbreakWeaponComponent* GetWeaponComponent() const { return WeaponComponent; }

	UOutbreakSquadMemberComponent* GetSquadMemberComponent() const { return SquadMemberComponent; }

protected:

	/** Called on every machine when health decreases. Delta is negative. */
	virtual void HandleDamaged(float Delta) {}

	/** Called on every machine when the character becomes incapacitated or dies. */
	virtual void HandleDowned(EOutbreakHealthState NewState);

	/** Called after BaseEyeHeight changes (crouch, settings). The player moves its camera here. */
	virtual void HandleEyeHeightChanged() {}

	/** Blueprint notification for HandleDowned. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Combat")
	void OnDowned(EOutbreakHealthState NewState);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
	TObjectPtr<UOutbreakHealthComponent> HealthComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
	TObjectPtr<UOutbreakWeaponComponent> WeaponComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Squad")
	TObjectPtr<UOutbreakSquadMemberComponent> SquadMemberComponent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Team")
	EOutbreakTeam Team = EOutbreakTeam::Player;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement")
	FOutbreakMovementSettings MovementSettings;

private:

	UFUNCTION()
	void OnHealthChanged(UOutbreakHealthComponent* InHealthComponent, float NewHealth, float Delta);

	UFUNCTION()
	void OnHealthStateChanged(UOutbreakHealthComponent* InHealthComponent, EOutbreakHealthState NewState, EOutbreakHealthState OldState);

	/** Sets BaseEyeHeight for the current crouch state. */
	void UpdateEyeHeight();

	bool bIsSprinting = false;
};
