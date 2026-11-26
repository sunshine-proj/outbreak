#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/EngineTypes.h"
#include "Squad/OutbreakSquadCommandTypes.h"

#include "OutbreakSquadCommandComponent.generated.h"

class UOutbreakSquadComponent;
class UOutbreakSquadMemberComponent;

/**
 * The local player's squad command interface (on AOutbreakPlayerController).
 *
 * Keeps the member selection, builds commands from what the player is looking at (Move
 * destination, Attack target), sends them to the squad, and formats the squad status shown on the
 * HUD. Local only: the squad component validates and distributes commands on the server.
 *
 * Teammate "slots" are 1-based positions in squad order, matching the HUD list and the
 * select-member key values.
 */
UCLASS(ClassGroup = (Outbreak), meta = (BlueprintSpawnableComponent))
class OUTBREAK_API UOutbreakSquadCommandComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	UOutbreakSquadCommandComponent();

	UFUNCTION(BlueprintCallable, Category = "Squad|Commands")
	void SelectAll();

	/** Selects only the teammate in Slot (1-based). Ignored if the slot is empty. */
	UFUNCTION(BlueprintCallable, Category = "Squad|Commands")
	void SelectSlot(int32 Slot);

	/** Adds or removes the teammate in Slot from the selection. */
	UFUNCTION(BlueprintCallable, Category = "Squad|Commands")
	void ToggleSlot(int32 Slot);

	/**
	 * Orders the selected teammates. Move uses the point under the crosshair; Attack uses the
	 * hostile actor under the crosshair. Returns false (with feedback) if no valid target exists.
	 */
	UFUNCTION(BlueprintCallable, Category = "Squad|Commands")
	bool IssueCommand(EOutbreakSquadCommandType CommandType);

	/** Cancels the selected teammates' orders. */
	UFUNCTION(BlueprintCallable, Category = "Squad|Commands")
	void CancelCommands();

	const FOutbreakSquadSelection& GetSelection() const { return Selection; }

	UFUNCTION(BlueprintPure, Category = "Squad|Commands")
	UOutbreakSquadComponent* GetSquad() const;

	/** Multi-line squad panel: selection, one line per teammate, and the last order feedback. */
	UFUNCTION(BlueprintPure, Category = "Squad|Commands")
	FText BuildSquadStatusText() const;

	/** One HUD line, e.g. "> 2 Hawk  Move (Moving)  85%  12 m". */
	static FText FormatMemberLine(int32 Slot, const UOutbreakSquadMemberComponent* Member, bool bSelected, float DistanceMeters);

protected:

	/** Length of the crosshair trace used for Move and Attack targets. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Squad|Commands", meta = (ClampMin = "100.0", Units = "cm"))
	float CommandTraceRange = 10000.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Squad|Commands")
	TEnumAsByte<ECollisionChannel> CommandTraceChannel = ECC_Visibility;

private:

	UOutbreakSquadMemberComponent* GetTeammateInSlot(int32 Slot) const;
	bool TraceCommandTarget(FVector& OutLocation, AActor*& OutActor) const;
	void PruneSelection();
	void SetFeedback(const FText& Feedback);

	FOutbreakSquadSelection Selection;
	FText LastFeedback;
};
