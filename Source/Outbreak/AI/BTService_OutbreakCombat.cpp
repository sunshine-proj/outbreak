#include "AI/BTService_OutbreakCombat.h"

#include "AI/OutbreakAIAgent.h"
#include "AIController.h"
#include "AI/OutbreakAICombatRules.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "GameFramework/Pawn.h"
#include "Weapons/OutbreakWeaponComponent.h"
#include "Weapons/OutbreakWeaponDefinition.h"
#include "Outbreak.h"

// Phase 15 profiling: `stat Outbreak`, Insights and the Outbreak CSV category.
DECLARE_CYCLE_STAT(TEXT("AI combat service"), STAT_OutbreakCombatService, STATGROUP_Outbreak);

UBTService_OutbreakCombat::UBTService_OutbreakCombat()
{
	NodeName = TEXT("Outbreak: Combat");
	Interval = 0.15f;
	RandomDeviation = 0.03f;
	bNotifyTick = true;
	bNotifyCeaseRelevant = true;
}

void UBTService_OutbreakCombat::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	OUTBREAK_SCOPE_CYCLE_COUNTER(STAT_OutbreakCombatService);

	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

	AAIController* Controller = OwnerComp.GetAIOwner();
	const IOutbreakAIAgent* Agent = Cast<IOutbreakAIAgent>(Controller);
	APawn* Pawn = Controller ? Controller->GetPawn() : nullptr;
	UOutbreakWeaponComponent* Weapons = Agent ? Agent->GetAgentWeaponComponent() : nullptr;
	if (!Pawn || !Weapons)
	{
		return;
	}

	AActor* Target = Agent->GetCombatTarget();
	const bool bTargetVisible = Target && Agent->IsActorVisible(Target);

	// Aim: Gameplay focus outranks the movement focus, so the agent faces the target while moving.
	if (bTargetVisible)
	{
		Controller->SetFocus(Target, EAIFocusPriority::Gameplay);
	}
	else
	{
		Controller->ClearFocus(EAIFocusPriority::Gameplay);
	}

	const UOutbreakWeaponDefinition* Weapon = Weapons->GetActiveWeaponDefinition();

	// Reload.
	if (Weapon && OutbreakAICombat::ShouldReload(Weapons->GetAmmoInMagazine(), Weapon->MagazineCapacity,
		bTargetVisible, Agent->GetIdleReloadFraction(), Weapons->CanReload()))
	{
		Weapons->StopFire();
		Weapons->StartReload();
		return;
	}

	// Fire.
	bool bShouldFire = false;
	if (bTargetVisible)
	{
		FVector ViewLocation;
		FRotator ViewRotation;
		Pawn->GetActorEyesViewPoint(ViewLocation, ViewRotation);
		const FVector TargetLocation = Target->GetActorLocation();

		const bool bAimed = OutbreakAICombat::IsAimedAt(ViewLocation, ViewRotation.Vector(), TargetLocation, Agent->GetAimTolerance());
		bShouldFire = OutbreakAICombat::ShouldFire(bTargetVisible, FVector::Dist(ViewLocation, TargetLocation),
			Agent->GetEngagementRange(), bAimed, Weapons->CanFire());
	}

	if (bShouldFire && !Agent->IsFireAllowed())
	{
		// Holding fire (squad stealth): keep the target covered without giving the squad away.
		bShouldFire = false;
	}

	if (bShouldFire)
	{
		// Automatic modes keep firing from one StartFire; semi-automatic modes fire once per call,
		// limited by the weapon's fire rate.
		const FOutbreakFireModeDefinition* FireMode = Weapons->GetActiveFireMode();
		if (!Weapons->IsFiring() || (FireMode && !FireMode->bAutomatic))
		{
			Weapons->StartFire();
		}
	}
	else
	{
		Weapons->StopFire();
	}
}

void UBTService_OutbreakCombat::OnCeaseRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* Controller = OwnerComp.GetAIOwner();
	if (const IOutbreakAIAgent* Agent = Cast<IOutbreakAIAgent>(Controller))
	{
		if (UOutbreakWeaponComponent* Weapons = Agent->GetAgentWeaponComponent())
		{
			Weapons->StopFire();
		}
	}
	if (Controller)
	{
		Controller->ClearFocus(EAIFocusPriority::Gameplay);
	}

	Super::OnCeaseRelevant(OwnerComp, NodeMemory);
}
