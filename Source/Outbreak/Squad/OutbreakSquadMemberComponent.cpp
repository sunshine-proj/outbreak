#include "Squad/OutbreakSquadMemberComponent.h"

#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "Net/UnrealNetwork.h"
#include "Squad/OutbreakSquadComponent.h"
#include "Weapons/OutbreakWeaponComponent.h"

UOutbreakSquadMemberComponent::UOutbreakSquadMemberComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	bWantsInitializeComponent = true;
	SetIsReplicatedByDefault(true);
}

void UOutbreakSquadMemberComponent::InitializeComponent()
{
	Super::InitializeComponent();

	if (AActor* Owner = GetOwner())
	{
		HealthComponent = Owner->FindComponentByClass<UOutbreakHealthComponent>();
		WeaponComponent = Owner->FindComponentByClass<UOutbreakWeaponComponent>();
	}

	if (HealthComponent)
	{
		HealthComponent->OnHealthStateChanged.AddUniqueDynamic(this, &ThisClass::HandleHealthStateChanged);
	}
}

void UOutbreakSquadMemberComponent::UninitializeComponent()
{
	// A destroyed member leaves its squad so no dangling entry remains. UninitializeComponent runs
	// on destruction even for actors that never began play.
	if (IsValid(Squad) && GetOwner() && GetOwner()->HasAuthority())
	{
		Squad->RemoveMember(this);
	}

	if (HealthComponent)
	{
		HealthComponent->OnHealthStateChanged.RemoveDynamic(this, &ThisClass::HandleHealthStateChanged);
	}

	Super::UninitializeComponent();
}

void UOutbreakSquadMemberComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UOutbreakSquadMemberComponent, Squad);
	DOREPLIFETIME(UOutbreakSquadMemberComponent, MemberId);
	DOREPLIFETIME(UOutbreakSquadMemberComponent, DisplayName);
	DOREPLIFETIME(UOutbreakSquadMemberComponent, Role);
}

void UOutbreakSquadMemberComponent::SetDisplayName(const FText& NewDisplayName)
{
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		DisplayName = NewDisplayName;
		OnMemberInfoChanged.Broadcast(this);
	}
}

void UOutbreakSquadMemberComponent::SetRole(UOutbreakSquadRoleDefinition* NewRole)
{
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		Role = NewRole;
		OnMemberInfoChanged.Broadcast(this);
	}
}

void UOutbreakSquadMemberComponent::SetSquadMembership(UOutbreakSquadComponent* NewSquad, int32 NewMemberId)
{
	Squad = NewSquad;
	MemberId = NewSquad ? NewMemberId : INDEX_NONE;
	OnMemberInfoChanged.Broadcast(this);
}

bool UOutbreakSquadMemberComponent::IsPlayerMember() const
{
	if (Squad)
	{
		return Squad->GetPlayerMember() == this;
	}
	const APawn* OwnerPawn = Cast<APawn>(GetOwner());
	return OwnerPawn && OwnerPawn->IsPlayerControlled();
}

EOutbreakSquadMemberStatus UOutbreakSquadMemberComponent::ToMemberStatus(EOutbreakHealthState HealthState)
{
	switch (HealthState)
	{
	case EOutbreakHealthState::Incapacitated:
		return EOutbreakSquadMemberStatus::Incapacitated;
	case EOutbreakHealthState::Dead:
		return EOutbreakSquadMemberStatus::Dead;
	default:
		return EOutbreakSquadMemberStatus::Active;
	}
}

EOutbreakSquadMemberStatus UOutbreakSquadMemberComponent::GetStatus() const
{
	return HealthComponent ? ToMemberStatus(HealthComponent->GetHealthState()) : EOutbreakSquadMemberStatus::Active;
}

bool UOutbreakSquadMemberComponent::IsAvailable() const
{
	return IsInSquad() && GetStatus() == EOutbreakSquadMemberStatus::Active;
}

float UOutbreakSquadMemberComponent::GetHealthFraction() const
{
	if (!HealthComponent || HealthComponent->GetMaxHealth() <= 0.0f)
	{
		return 1.0f;
	}
	return FMath::Clamp(HealthComponent->GetHealth() / HealthComponent->GetMaxHealth(), 0.0f, 1.0f);
}

FVector UOutbreakSquadMemberComponent::GetMemberLocation() const
{
	const AActor* Owner = GetOwner();
	return Owner ? Owner->GetActorLocation() : FVector::ZeroVector;
}

void UOutbreakSquadMemberComponent::HandleHealthStateChanged(UOutbreakHealthComponent* InHealthComponent,
	EOutbreakHealthState NewState, EOutbreakHealthState OldState)
{
	const EOutbreakSquadMemberStatus NewStatus = ToMemberStatus(NewState);
	const EOutbreakSquadMemberStatus OldStatus = ToMemberStatus(OldState);
	if (NewStatus != OldStatus)
	{
		OnStatusChanged.Broadcast(this, NewStatus, OldStatus);
	}
}

void UOutbreakSquadMemberComponent::OnRep_MemberInfo()
{
	OnMemberInfoChanged.Broadcast(this);
}
