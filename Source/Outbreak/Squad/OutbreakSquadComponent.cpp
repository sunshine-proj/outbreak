#include "Squad/OutbreakSquadComponent.h"

#include "Characters/OutbreakCharacter.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Net/UnrealNetwork.h"
#include "Outbreak.h"
#include "Squad/OutbreakSquadConfiguration.h"
#include "Squad/OutbreakSquadMemberComponent.h"
#include "Squad/OutbreakSquadRoleDefinition.h"
#include "Squad/OutbreakSquadSettings.h"
#include "Squad/OutbreakTeammateCharacter.h"
#include "Weapons/OutbreakWeaponComponent.h"
#include "Weapons/OutbreakWeaponDefinition.h"

#define LOCTEXT_NAMESPACE "OutbreakSquad"

UOutbreakSquadComponent::UOutbreakSquadComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	bWantsInitializeComponent = true;
	SetIsReplicatedByDefault(true);
}

void UOutbreakSquadComponent::InitializeComponent()
{
	Super::InitializeComponent();

	// Start from the project limits so registration is bounded even before InitializeSquad.
	const FOutbreakSquadSizeLimits& GlobalLimits = GetDefault<UOutbreakSquadSettings>()->SizeLimits;
	if (GlobalLimits.IsValid())
	{
		SizeLimits = GlobalLimits;
	}
	else
	{
		UE_LOG(LogOutbreak, Error, TEXT("Project squad limits are invalid (min %d, default %d, max %d); using built-in defaults."),
			GlobalLimits.MinSquadSize, GlobalLimits.DefaultSquadSize, GlobalLimits.MaxSquadSize);
	}
}

void UOutbreakSquadComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UOutbreakSquadComponent, Members);
	DOREPLIFETIME(UOutbreakSquadComponent, PlayerMember);
	DOREPLIFETIME(UOutbreakSquadComponent, SizeLimits);
}

bool UOutbreakSquadComponent::HasOwnerAuthority() const
{
	const AActor* Owner = GetOwner();
	return Owner && Owner->HasAuthority();
}

// --- Initialization ------------------------------------------------------------------------------

bool UOutbreakSquadComponent::InitializeSquad(const UOutbreakSquadConfiguration* Configuration, APawn* PlayerPawn,
	int32 RequestedSizeOverride)
{
	if (!HasOwnerAuthority() || bSquadInitialized)
	{
		return false;
	}
	bSquadInitialized = true;

	const UOutbreakSquadSettings* Settings = GetDefault<UOutbreakSquadSettings>();
	if (!Configuration && !Settings->DefaultSquadConfiguration.IsNull())
	{
		Configuration = Settings->DefaultSquadConfiguration.LoadSynchronous();
	}

	// The player always counts as a member, even if the configuration is rejected below.
	UOutbreakSquadMemberComponent* PlayerMemberComponent =
		PlayerPawn ? PlayerPawn->FindComponentByClass<UOutbreakSquadMemberComponent>() : nullptr;
	if (!PlayerMemberComponent)
	{
		UE_LOG(LogOutbreak, Error, TEXT("%s: player pawn %s has no UOutbreakSquadMemberComponent; squad not created."),
			*GetNameSafe(GetOwner()), *GetNameSafe(PlayerPawn));
		return false;
	}

	FText Error;
	TArray<FText> ConfigErrors;
	if (Configuration && !Configuration->Validate(ConfigErrors))
	{
		Error = FText::Join(FText::FromString(TEXT(" ")), ConfigErrors);
	}

	FOutbreakSquadSizeLimits ResolvedLimits;
	int32 SquadSize = 0;
	const bool bValid = Error.IsEmpty()
		&& OutbreakSquadRules::ResolveLimits(Settings->SizeLimits,
			Configuration ? Configuration->MinSquadSizeOverride : 0,
			Configuration ? Configuration->MaxSquadSizeOverride : 0,
			ResolvedLimits, Error)
		&& OutbreakSquadRules::ResolveSquadSize(ResolvedLimits,
			RequestedSizeOverride > 0 ? RequestedSizeOverride : (Configuration ? Configuration->SquadSize : 0),
			SquadSize, Error);

	if (bValid)
	{
		SizeLimits = ResolvedLimits;
	}

	RegisterPlayerMember(PlayerMemberComponent);

	if (!bValid)
	{
		UE_LOG(LogOutbreak, Error, TEXT("%s: squad configuration %s rejected: %s Only the player is in the squad."),
			*GetNameSafe(GetOwner()), *GetNameSafe(Configuration), *Error.ToString());
		return false;
	}

	const FOutbreakSquadMemberConfig EmptyConfig;
	for (int32 TeammateIndex = 0; TeammateIndex < SquadSize - 1; ++TeammateIndex)
	{
		const FOutbreakSquadMemberConfig& MemberConfig = !Configuration
			? EmptyConfig
			: (Configuration->Teammates.IsValidIndex(TeammateIndex) ? Configuration->Teammates[TeammateIndex] : Configuration->FillerTeammate);

		AOutbreakCharacter* Teammate = SpawnTeammate(MemberConfig, PlayerPawn, TeammateIndex);
		if (!Teammate)
		{
			continue;
		}

		ApplyMemberConfig(Teammate, MemberConfig);
		const EOutbreakSquadRegistrationResult Result = RegisterMember(Teammate->GetSquadMemberComponent());
		if (Result != EOutbreakSquadRegistrationResult::Registered)
		{
			UE_LOG(LogOutbreak, Warning, TEXT("%s: teammate %s not registered (%s); destroying it."),
				*GetNameSafe(GetOwner()), *Teammate->GetName(), *UEnum::GetValueAsString(Result));
			Teammate->Destroy();
		}
	}

	UE_LOG(LogOutbreak, Log, TEXT("%s: squad created with %d of %d members (limits %d-%d)."), *GetNameSafe(GetOwner()),
		Members.Num(), SquadSize, SizeLimits.MinSquadSize, SizeLimits.MaxSquadSize);
	return Members.Num() == SquadSize;
}

AOutbreakCharacter* UOutbreakSquadComponent::SpawnTeammate(const FOutbreakSquadMemberConfig& MemberConfig,
	const APawn* PlayerPawn, int32 TeammateIndex)
{
	UWorld* World = GetWorld();
	if (!World || !PlayerPawn)
	{
		return nullptr;
	}

	const UOutbreakSquadSettings* Settings = GetDefault<UOutbreakSquadSettings>();
	TSubclassOf<AOutbreakTeammateCharacter> TeammateClass = MemberConfig.CharacterClass;
	if (!TeammateClass)
	{
		TeammateClass = Settings->DefaultTeammateClass.LoadSynchronous();
	}
	if (!TeammateClass)
	{
		TeammateClass = AOutbreakTeammateCharacter::StaticClass();
	}

	// Two columns behind the player. Proper formations arrive in Phase 7.
	const float Spacing = Settings->InitialSpawnSpacing;
	const int32 Row = TeammateIndex / 2 + 1;
	const float Side = (TeammateIndex % 2 == 0) ? -0.5f : 0.5f;
	const FVector Offset = PlayerPawn->GetActorForwardVector() * (-Spacing * Row)
		+ PlayerPawn->GetActorRightVector() * (Spacing * Side);
	const FTransform SpawnTransform(PlayerPawn->GetActorRotation(), PlayerPawn->GetActorLocation() + Offset);

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	SpawnParameters.Owner = GetOwner();

	AOutbreakTeammateCharacter* Teammate = World->SpawnActor<AOutbreakTeammateCharacter>(TeammateClass, SpawnTransform, SpawnParameters);
	if (!Teammate)
	{
		UE_LOG(LogOutbreak, Warning, TEXT("%s: failed to spawn teammate %d of class %s."),
			*GetNameSafe(GetOwner()), TeammateIndex, *GetNameSafe(TeammateClass.Get()));
	}
	return Teammate;
}

void UOutbreakSquadComponent::ApplyMemberConfig(AOutbreakCharacter* Teammate, const FOutbreakSquadMemberConfig& MemberConfig) const
{
	UOutbreakSquadMemberComponent* Member = Teammate->GetSquadMemberComponent();
	UOutbreakWeaponComponent* Weapons = Teammate->GetWeaponComponent();
	const UOutbreakSquadRoleDefinition* Role = MemberConfig.Role;

	Member->SetRole(MemberConfig.Role);
	Member->SetDisplayName(!MemberConfig.DisplayName.IsEmpty()
		? MemberConfig.DisplayName
		: FText::Format(LOCTEXT("DefaultTeammateName", "Soldier {0}"), NextMemberId));

	// Member overrides win over role defaults; each list is replaced as a whole, not merged.
	const TArray<FOutbreakAmmoAmount>& Ammo = !MemberConfig.AmmoReserves.IsEmpty() || !Role
		? MemberConfig.AmmoReserves : Role->DefaultAmmoReserves;
	for (const FOutbreakAmmoAmount& Reserve : Ammo)
	{
		Weapons->AddAmmo(Reserve.AmmoType, Reserve.Count);
	}

	const TArray<TObjectPtr<UOutbreakWeaponDefinition>>& WeaponList = !MemberConfig.Weapons.IsEmpty() || !Role
		? MemberConfig.Weapons : Role->DefaultWeapons;
	for (UOutbreakWeaponDefinition* Weapon : WeaponList)
	{
		Weapons->AddWeapon(Weapon);
	}
}

// --- Membership ----------------------------------------------------------------------------------

EOutbreakSquadRegistrationResult UOutbreakSquadComponent::RegisterMember(UOutbreakSquadMemberComponent* Member)
{
	return RegisterMemberInternal(Member, /*bIsPlayer=*/ false);
}

EOutbreakSquadRegistrationResult UOutbreakSquadComponent::RegisterPlayerMember(UOutbreakSquadMemberComponent* Member)
{
	return RegisterMemberInternal(Member, /*bIsPlayer=*/ true);
}

EOutbreakSquadRegistrationResult UOutbreakSquadComponent::RegisterMemberInternal(UOutbreakSquadMemberComponent* Member, bool bIsPlayer)
{
	if (!HasOwnerAuthority() || !IsValid(Member))
	{
		return EOutbreakSquadRegistrationResult::Invalid;
	}
	if (Members.Contains(Member))
	{
		return EOutbreakSquadRegistrationResult::AlreadyMember;
	}
	if (Member->GetSquad() && Member->GetSquad() != this)
	{
		return EOutbreakSquadRegistrationResult::InOtherSquad;
	}
	if (IsFull())
	{
		return EOutbreakSquadRegistrationResult::SquadFull;
	}

	Members.Add(Member);
	if (bIsPlayer)
	{
		PlayerMember = Member;
	}
	Member->SetSquadMembership(this, NextMemberId++);
	BindMember(Member);

	UE_LOG(LogOutbreak, Verbose, TEXT("%s: registered %s as member %d (%d/%d)."), *GetNameSafe(GetOwner()),
		*GetNameSafe(Member->GetOwner()), Member->GetMemberId(), Members.Num(), SizeLimits.MaxSquadSize);
	OnMemberAdded.Broadcast(this, Member);
	return EOutbreakSquadRegistrationResult::Registered;
}

bool UOutbreakSquadComponent::RemoveMember(UOutbreakSquadMemberComponent* Member)
{
	if (!HasOwnerAuthority() || !Member || !Members.Contains(Member))
	{
		return false;
	}

	Members.Remove(Member);
	if (PlayerMember == Member)
	{
		PlayerMember = nullptr;
	}
	UnbindMember(Member);
	Member->SetSquadMembership(nullptr, INDEX_NONE);

	OnMemberRemoved.Broadcast(this, Member);
	return true;
}

bool UOutbreakSquadComponent::SetSizeLimits(const FOutbreakSquadSizeLimits& NewLimits)
{
	if (!HasOwnerAuthority() || !NewLimits.IsValid())
	{
		return false;
	}
	SizeLimits = NewLimits;
	return true;
}

void UOutbreakSquadComponent::BindMember(UOutbreakSquadMemberComponent* Member)
{
	Member->OnStatusChanged.AddUniqueDynamic(this, &ThisClass::HandleMemberStatusChanged);
}

void UOutbreakSquadComponent::UnbindMember(UOutbreakSquadMemberComponent* Member)
{
	Member->OnStatusChanged.RemoveDynamic(this, &ThisClass::HandleMemberStatusChanged);
}

void UOutbreakSquadComponent::HandleMemberStatusChanged(UOutbreakSquadMemberComponent* Member,
	EOutbreakSquadMemberStatus NewStatus, EOutbreakSquadMemberStatus OldStatus)
{
	// Dead members stay registered (the HUD shows them as lost) until explicitly removed.
	UE_LOG(LogOutbreak, Log, TEXT("%s: member %s is now %s (%d alive of %d)."), *GetNameSafe(GetOwner()),
		*GetNameSafe(Member ? Member->GetOwner() : nullptr), *UEnum::GetValueAsString(NewStatus),
		GetAliveMemberCount(), Members.Num());
	OnMemberStatusChanged.Broadcast(this, Member, NewStatus, OldStatus);
}

void UOutbreakSquadComponent::OnRep_Members()
{
	for (UOutbreakSquadMemberComponent* Previous : PreviousMembers)
	{
		if (Previous && !Members.Contains(Previous))
		{
			UnbindMember(Previous);
			OnMemberRemoved.Broadcast(this, Previous);
		}
	}
	for (UOutbreakSquadMemberComponent* Current : Members)
	{
		if (Current && !PreviousMembers.Contains(Current))
		{
			BindMember(Current);
			OnMemberAdded.Broadcast(this, Current);
		}
	}
	PreviousMembers = Members;
}

// --- Queries -------------------------------------------------------------------------------------

TArray<UOutbreakSquadMemberComponent*> UOutbreakSquadComponent::GetMembers() const
{
	TArray<UOutbreakSquadMemberComponent*> Result;
	Result.Reserve(Members.Num());
	for (UOutbreakSquadMemberComponent* Member : Members)
	{
		if (Member)
		{
			Result.Add(Member);
		}
	}
	return Result;
}

int32 UOutbreakSquadComponent::GetAliveMemberCount() const
{
	int32 Count = 0;
	for (const UOutbreakSquadMemberComponent* Member : Members)
	{
		if (Member && Member->GetStatus() != EOutbreakSquadMemberStatus::Dead)
		{
			++Count;
		}
	}
	return Count;
}

TArray<UOutbreakSquadMemberComponent*> UOutbreakSquadComponent::GetAvailableTeammates() const
{
	TArray<UOutbreakSquadMemberComponent*> Result;
	for (UOutbreakSquadMemberComponent* Member : Members)
	{
		if (Member && Member != PlayerMember && Member->IsAvailable())
		{
			Result.Add(Member);
		}
	}
	return Result;
}

UOutbreakSquadMemberComponent* UOutbreakSquadComponent::FindMemberById(int32 MemberId) const
{
	for (UOutbreakSquadMemberComponent* Member : Members)
	{
		if (Member && Member->GetMemberId() == MemberId)
		{
			return Member;
		}
	}
	return nullptr;
}

bool UOutbreakSquadComponent::ContainsMember(const UOutbreakSquadMemberComponent* Member) const
{
	return Member && Members.ContainsByPredicate(
		[Member](const TObjectPtr<UOutbreakSquadMemberComponent>& Entry) { return Entry.Get() == Member; });
}

bool UOutbreakSquadComponent::IsCompositionValid() const
{
	TArray<FText> Errors;
	return ValidateComposition(Errors);
}

bool UOutbreakSquadComponent::ValidateComposition(TArray<FText>& OutErrors) const
{
	const int32 InitialErrorCount = OutErrors.Num();

	if (!PlayerMember || !Members.Contains(PlayerMember))
	{
		OutErrors.Add(LOCTEXT("NoPlayer", "The squad has no player member."));
	}
	if (!SizeLimits.Contains(Members.Num()))
	{
		OutErrors.Add(FText::Format(LOCTEXT("CountOutOfRange", "The squad has {0} members; allowed {1}-{2}."),
			Members.Num(), SizeLimits.MinSquadSize, SizeLimits.MaxSquadSize));
	}

	TSet<int32> SeenIds;
	for (const UOutbreakSquadMemberComponent* Member : Members)
	{
		if (!IsValid(Member) || Member->GetSquad() != this)
		{
			OutErrors.Add(LOCTEXT("InvalidMember", "The squad contains an invalid member."));
			continue;
		}
		bool bAlreadySeen = false;
		SeenIds.Add(Member->GetMemberId(), &bAlreadySeen);
		if (bAlreadySeen)
		{
			OutErrors.Add(FText::Format(LOCTEXT("DuplicateId", "Member ID {0} is used twice."), Member->GetMemberId()));
		}
	}

	return OutErrors.Num() == InitialErrorCount;
}

float UOutbreakSquadComponent::GetDistanceToPlayer(const UOutbreakSquadMemberComponent* Member) const
{
	if (!PlayerMember || !Member)
	{
		return -1.0f;
	}
	return FVector::Dist(PlayerMember->GetMemberLocation(), Member->GetMemberLocation());
}

#undef LOCTEXT_NAMESPACE
