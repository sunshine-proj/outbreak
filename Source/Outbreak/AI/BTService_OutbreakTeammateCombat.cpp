#include "AI/BTService_OutbreakTeammateCombat.h"

#include "AI/OutbreakSquadAIController.h"
#include "AI/OutbreakTeammateAIConfig.h"
#include "AI/OutbreakTeammateCombatRules.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "GameFramework/Pawn.h"
#include "Weapons/OutbreakWeaponComponent.h"
#include "Weapons/OutbreakWeaponDefinition.h"

UBTService_OutbreakTeammateCombat::UBTService_OutbreakTeammateCombat()
{
	NodeName = TEXT("Outbreak: Teammate Combat");
	Interval = 0.15f;
	RandomDeviation = 0.03f;
	bNotifyTick = true;
	bNotifyCeaseRelevant = true;
}

void UBTService_OutbreakTeammateCombat::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

	AOutbreakSquadAIController* Controller = Cast<AOutbreakSquadAIController>(OwnerComp.GetAIOwner());
	APawn* Pawn = Controller ? Controller->GetPawn() : nullptr;
	UOutbreakWeaponComponent* Weapons = Controller ? Controller->GetWeaponComponent() : nullptr;
	if (!Pawn || !Weapons)
	{
		return;
	}

	AActor* Target = Controller->GetCombatTarget();
	const bool bTargetVisible = Target && Controller->IsActorVisible(Target);

	// Aim: Gameplay focus outranks the movement focus, so the teammate faces the target while moving.
	if (bTargetVisible)
	{
		Controller->SetFocus(Target, EAIFocusPriority::Gameplay);
	}
	else
	{
		Controller->ClearFocus(EAIFocusPriority::Gameplay);
	}

	const UOutbreakTeammateAIConfig* Config = Controller->GetAIConfig();
	const UOutbreakWeaponDefinition* Weapon = Weapons->GetActiveWeaponDefinition();

	// Reload.
	if (Weapon && OutbreakTeammateCombat::ShouldReload(Weapons->GetAmmoInMagazine(), Weapon->MagazineCapacity,
		bTargetVisible, Config->IdleReloadFraction, Weapons->CanReload()))
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

		const bool bAimed = OutbreakTeammateCombat::IsAimedAt(ViewLocation, ViewRotation.Vector(), TargetLocation, Config->AimTolerance);
		bShouldFire = OutbreakTeammateCombat::ShouldFire(bTargetVisible, FVector::Dist(ViewLocation, TargetLocation),
			Controller->GetEngagementRange(), bAimed, Weapons->CanFire());
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

void UBTService_OutbreakTeammateCombat::OnCeaseRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	if (AOutbreakSquadAIController* Controller = Cast<AOutbreakSquadAIController>(OwnerComp.GetAIOwner()))
	{
		if (UOutbreakWeaponComponent* Weapons = Controller->GetWeaponComponent())
		{
			Weapons->StopFire();
		}
		Controller->ClearFocus(EAIFocusPriority::Gameplay);
	}

	Super::OnCeaseRelevant(OwnerComp, NodeMemory);
}
