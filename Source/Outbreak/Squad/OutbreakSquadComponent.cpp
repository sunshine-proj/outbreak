#include "Squad/OutbreakSquadComponent.h"

#include "Characters/OutbreakCharacter.h"
#include "Combat/OutbreakHealthComponent.h"
#include "Combat/OutbreakTeam.h"
#include "Engine/World.h"
#include "Inventory/OutbreakInventoryComponent.h"
#include "Inventory/OutbreakLoadout.h"
#include "GameFramework/Pawn.h"
#include "Net/UnrealNetwork.h"
#include "Outbreak.h"
#include "TimerManager.h"
#include "Squad/OutbreakSquadConfiguration.h"
#include "Squad/OutbreakSquadMemberComponent.h"
#include "Squad/OutbreakSquadRoleDefinition.h"
#include "Squad/OutbreakSquadSelection.h"
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

	DOREPLIFETIME(UOutbreakSquadComponent, AvailableLoadouts);
	DOREPLIFETIME(UOutbreakSquadComponent, bLoadoutSelectionOpen);

	DOREPLIFETIME(UOutbreakSquadComponent, Members);
	DOREPLIFETIME(UOutbreakSquadComponent, PlayerMember);
	DOREPLIFETIME(UOutbreakSquadComponent, SizeLimits);
	DOREPLIFETIME(UOutbreakSquadComponent, Formation);
	DOREPLIFETIME(UOutbreakSquadComponent, bStealthMode);
	DOREPLIFETIME(UOutbreakSquadComponent, bCompromised);
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
	return InitializeSquadInternal(Configuration, PlayerPawn, RequestedSizeOverride, nullptr);
}

bool UOutbreakSquadComponent::InitializeSquadWithSelection(const UOutbreakSquadConfiguration* Configuration, APawn* PlayerPawn,
	const FOutbreakDeploymentSelection& Selection)
{
	return InitializeSquadInternal(Configuration, PlayerPawn, 0, &Selection);
}

bool UOutbreakSquadComponent::InitializeSquadInternal(const UOutbreakSquadConfiguration* Configuration, APawn* PlayerPawn,
	int32 RequestedSizeOverride, const FOutbreakDeploymentSelection* RequestedSelection)
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
	const bool bLimitsValid = Error.IsEmpty()
		&& OutbreakSquadRules::ResolveLimits(Settings->SizeLimits,
			Configuration ? Configuration->MinSquadSizeOverride : 0,
			Configuration ? Configuration->MaxSquadSizeOverride : 0,
			ResolvedLimits, Error);

	// A briefing selection replaces the configuration's size and composition (Phase 11).
	FOutbreakDeploymentSelection Selection;
	if (RequestedSelection && bLimitsValid)
	{
		TArray<FText> SelectionErrors;
		Selection = *RequestedSelection;
		if (!OutbreakSquadSelection::Validate(Configuration, ResolvedLimits, Selection, SelectionErrors))
		{
			UE_LOG(LogOutbreak, Warning, TEXT("%s: squad selection rejected (%s); using the default squad."), *GetNameSafe(GetOwner()),
				*FText::Join(FText::FromString(TEXT(" ")), SelectionErrors).ToString());
			Selection = OutbreakSquadSelection::MakeDefault(Configuration, ResolvedLimits);
		}
	}
	const bool bUseSelection = RequestedSelection && bLimitsValid;

	int32 SquadSize = 0;
	const bool bValid = bLimitsValid
		&& OutbreakSquadRules::ResolveSquadSize(ResolvedLimits,
			bUseSelection ? Selection.SquadSize
				: (RequestedSizeOverride > 0 ? RequestedSizeOverride : (Configuration ? Configuration->SquadSize : 0)),
			SquadSize, Error);

	if (bValid)
	{
		SizeLimits = ResolvedLimits;
	}
	if (Configuration)
	{
		AuthSetFormation(Configuration->InitialFormation);
		AvailableLoadouts = Configuration->AvailableLoadouts;
	}

	RegisterPlayerMember(PlayerMemberComponent);

	// The player's equipment from the mission, replacing the Blueprint defaults.
	if (Configuration && Configuration->PlayerLoadout)
	{
		if (UOutbreakInventoryComponent* PlayerInventory = PlayerPawn->FindComponentByClass<UOutbreakInventoryComponent>())
		{
			PlayerInventory->ApplyLoadout(Configuration->PlayerLoadout->Loadout);
		}
	}

	if (!bValid)
	{
		UE_LOG(LogOutbreak, Error, TEXT("%s: squad configuration %s rejected: %s Only the player is in the squad."),
			*GetNameSafe(GetOwner()), *GetNameSafe(Configuration), *Error.ToString());
		return false;
	}

	// Member per selection slot: [0] the player, then each teammate slot (null where spawning failed).
	TArray<UOutbreakSquadMemberComponent*> SlotMembers;
	SlotMembers.Add(PlayerMemberComponent);

	const FOutbreakSquadMemberConfig EmptyConfig;
	for (int32 TeammateIndex = 0; TeammateIndex < SquadSize - 1; ++TeammateIndex)
	{
		const FOutbreakSquadMemberConfig& MemberConfig = bUseSelection
			? OutbreakSquadSelection::GetTeammateConfig(Configuration, Selection, TeammateIndex)
			: (!Configuration
				? EmptyConfig
				: (Configuration->Teammates.IsValidIndex(TeammateIndex) ? Configuration->Teammates[TeammateIndex] : Configuration->FillerTeammate));

		SlotMembers.Add(nullptr);
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
			continue;
		}
		SlotMembers.Last() = Teammate->GetSquadMemberComponent();
	}

	// Equipment selection from the briefing, through the same checks as SelectMemberLoadout.
	if (bUseSelection)
	{
		for (int32 MemberIndex = 0; MemberIndex < SlotMembers.Num(); ++MemberIndex)
		{
			UOutbreakLoadoutDefinition* Loadout = OutbreakSquadSelection::GetLoadout(Selection, MemberIndex);
			if (Loadout && SlotMembers[MemberIndex] && !AuthSelectMemberLoadout(SlotMembers[MemberIndex], Loadout))
			{
				UE_LOG(LogOutbreak, Warning, TEXT("%s: selected loadout %s could not be given to %s; it keeps its default equipment."),
					*GetNameSafe(GetOwner()), *Loadout->GetName(), *SlotMembers[MemberIndex]->GetDisplayName().ToString());
			}
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

	UOutbreakInventoryComponent* Inventory = Teammate->GetInventoryComponent();
	if (Inventory && Role && Role->CarryCapacity > 0.0f)
	{
		Inventory->SetMaxCarryWeight(Role->CarryCapacity);
	}

	// A complete loadout (member's, else role's) replaces the individual equipment lists.
	const UOutbreakLoadoutDefinition* Loadout = MemberConfig.Loadout ? MemberConfig.Loadout.Get() : (Role ? Role->DefaultLoadout.Get() : nullptr);
	if (Loadout && Inventory)
	{
		Inventory->ApplyLoadout(Loadout->Loadout);
		return;
	}

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

	const TArray<FOutbreakItemAmount>& ItemList = !MemberConfig.Items.IsEmpty() || !Role ? MemberConfig.Items : Role->DefaultItems;
	for (const FOutbreakItemAmount& Entry : ItemList)
	{
		if (Inventory && Inventory->AddItem(Entry.Item, Entry.Count) < Entry.Count)
		{
			UE_LOG(LogOutbreak, Warning, TEXT("%s: not all of %s fit into %s's carrying capacity."),
				*GetNameSafe(GetOwner()), *GetNameSafe(Entry.Item.Get()), *Teammate->GetName());
		}
	}
}

// --- Equipment selection -------------------------------------------------------------------------

TArray<UOutbreakLoadoutDefinition*> UOutbreakSquadComponent::GetAvailableLoadouts() const
{
	TArray<UOutbreakLoadoutDefinition*> Result;
	for (UOutbreakLoadoutDefinition* Loadout : AvailableLoadouts)
	{
		if (Loadout)
		{
			Result.Add(Loadout);
		}
	}
	return Result;
}

bool UOutbreakSquadComponent::SelectMemberLoadout(UOutbreakSquadMemberComponent* Member, UOutbreakLoadoutDefinition* Loadout)
{
	if (HasOwnerAuthority())
	{
		return AuthSelectMemberLoadout(Member, Loadout);
	}
	if (!bLoadoutSelectionOpen || !Member || !AvailableLoadouts.Contains(Loadout))
	{
		return false;
	}
	ServerSelectMemberLoadout(Member, Loadout);
	return true;
}

void UOutbreakSquadComponent::ServerSelectMemberLoadout_Implementation(UOutbreakSquadMemberComponent* Member,
	UOutbreakLoadoutDefinition* Loadout)
{
	AuthSelectMemberLoadout(Member, Loadout);
}

bool UOutbreakSquadComponent::AuthSelectMemberLoadout(UOutbreakSquadMemberComponent* Member, UOutbreakLoadoutDefinition* Loadout)
{
	if (!bLoadoutSelectionOpen || !Member || !Loadout || !Members.Contains(Member) || !AvailableLoadouts.Contains(Loadout)
		|| Member->GetStatus() != EOutbreakSquadMemberStatus::Active)
	{
		return false;
	}
	UOutbreakInventoryComponent* Inventory = Member->GetOwner()->FindComponentByClass<UOutbreakInventoryComponent>();
	if (!Inventory)
	{
		return false;
	}

	TArray<FText> Errors;
	if (!OutbreakLoadout::Validate(Loadout->Loadout, Inventory->GetMaxCarryWeight(), Errors))
	{
		UE_LOG(LogOutbreak, Warning, TEXT("%s: loadout %s rejected for %s: %s"), *GetNameSafe(GetOwner()), *Loadout->GetName(),
			*Member->GetDisplayName().ToString(), *FText::Join(FText::FromString(TEXT(" ")), Errors).ToString());
		return false;
	}
	Inventory->ApplyLoadout(Loadout->Loadout);
	return true;
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
	if (UOutbreakWeaponComponent* MemberWeapons = Member->GetWeaponComponent())
	{
		MemberWeapons->OnShotFired.AddUniqueDynamic(this, &ThisClass::HandleMemberShotFired);
	}
}

void UOutbreakSquadComponent::UnbindMember(UOutbreakSquadMemberComponent* Member)
{
	Member->OnStatusChanged.RemoveDynamic(this, &ThisClass::HandleMemberStatusChanged);
	if (UOutbreakWeaponComponent* MemberWeapons = Member->GetWeaponComponent())
	{
		MemberWeapons->OnShotFired.RemoveDynamic(this, &ThisClass::HandleMemberShotFired);
	}
}

void UOutbreakSquadComponent::HandleMemberShotFired(UOutbreakWeaponComponent* WeaponComponent)
{
	// A shot from anyone in the squad gives it away.
	if (bStealthMode)
	{
		MarkCompromised();
	}
}

void UOutbreakSquadComponent::SetStealthMode(bool bEnabled)
{
	if (HasOwnerAuthority())
	{
		AuthSetStealthMode(bEnabled);
	}
	else
	{
		ServerSetStealthMode(bEnabled);
	}
}

void UOutbreakSquadComponent::ServerSetStealthMode_Implementation(bool bEnabled)
{
	AuthSetStealthMode(bEnabled);
}

void UOutbreakSquadComponent::AuthSetStealthMode(bool bEnabled)
{
	if (bStealthMode == bEnabled)
	{
		return;
	}

	bStealthMode = bEnabled;
	if (bEnabled)
	{
		FormationBeforeStealth = Formation;
		AuthSetFormation(EOutbreakFormation::Stealth);
	}
	else
	{
		AuthSetFormation(FormationBeforeStealth);
	}
	ClearCompromised();

	UE_LOG(LogOutbreak, Log, TEXT("%s: stealth mode %s."), *GetNameSafe(GetOwner()), bEnabled ? TEXT("on") : TEXT("off"));
	OnStealthChanged.Broadcast(this);
}

void UOutbreakSquadComponent::MarkCompromised()
{
	if (!HasOwnerAuthority() || !bStealthMode)
	{
		return;
	}

	if (!bCompromised)
	{
		bCompromised = true;
		UE_LOG(LogOutbreak, Log, TEXT("%s: squad compromised."), *GetNameSafe(GetOwner()));
		OnStealthChanged.Broadcast(this);
	}

	const float Duration = GetDefault<UOutbreakSquadSettings>()->CompromiseDuration;
	if (UWorld* World = GetWorld(); World && Duration > 0.0f)
	{
		World->GetTimerManager().SetTimer(CompromiseTimer, this, &ThisClass::ClearCompromised, Duration, false);
	}
}

void UOutbreakSquadComponent::ClearCompromised()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(CompromiseTimer);
	}
	if (bCompromised)
	{
		bCompromised = false;
		OnStealthChanged.Broadcast(this);
	}
}

void UOutbreakSquadComponent::OnRep_Stealth()
{
	OnStealthChanged.Broadcast(this);
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

// --- Commands ------------------------------------------------------------------------------------

void UOutbreakSquadComponent::IssueCommand(const FOutbreakSquadCommand& Command, const FOutbreakCommandRecipients& Recipients)
{
	if (HasOwnerAuthority())
	{
		AuthIssueCommand(Command, Recipients);
	}
	else
	{
		ServerIssueCommand(Command, Recipients);
	}
}

void UOutbreakSquadComponent::ServerIssueCommand_Implementation(const FOutbreakSquadCommand& Command, const FOutbreakCommandRecipients& Recipients)
{
	AuthIssueCommand(Command, Recipients);
}

void UOutbreakSquadComponent::CancelCommand(const FOutbreakCommandRecipients& Recipients)
{
	if (HasOwnerAuthority())
	{
		AuthCancelCommand(Recipients);
	}
	else
	{
		ServerCancelCommand(Recipients);
	}
}

void UOutbreakSquadComponent::ServerCancelCommand_Implementation(const FOutbreakCommandRecipients& Recipients)
{
	AuthCancelCommand(Recipients);
}

TArray<UOutbreakSquadMemberComponent*> UOutbreakSquadComponent::ResolveRecipients(const FOutbreakCommandRecipients& Recipients) const
{
	TArray<UOutbreakSquadMemberComponent*> Result;
	switch (Recipients.Scope)
	{
	case EOutbreakCommandScope::Squad:
		return GetTeammates();

	case EOutbreakCommandScope::Individual:
		if (Recipients.MemberIds.Num() > 0)
		{
			if (UOutbreakSquadMemberComponent* Member = FindMemberById(Recipients.MemberIds[0]))
			{
				Result.Add(Member);
			}
		}
		return Result;

	case EOutbreakCommandScope::Selected:
		for (const int32 MemberId : Recipients.MemberIds)
		{
			if (UOutbreakSquadMemberComponent* Member = FindMemberById(MemberId))
			{
				Result.AddUnique(Member);
			}
		}
		return Result;

	default:
		return Result;
	}
}

EOutbreakCommandRejection UOutbreakSquadComponent::ValidateCommandForMember(const FOutbreakSquadCommand& Command,
	const UOutbreakSquadMemberComponent* Member) const
{
	if (Command.Type == EOutbreakSquadCommandType::None)
	{
		return EOutbreakCommandRejection::NoCommand;
	}
	if (!Member || !ContainsMember(Member))
	{
		return EOutbreakCommandRejection::NotInSquad;
	}
	if (Member == PlayerMember.Get())
	{
		return EOutbreakCommandRejection::PlayerMember;
	}
	if (Member->GetStatus() != EOutbreakSquadMemberStatus::Active)
	{
		return EOutbreakCommandRejection::Unavailable;
	}
	if (Command.RequiresLocation() && !Command.bHasTargetLocation)
	{
		return EOutbreakCommandRejection::MissingLocation;
	}
	if (Command.RequiresTarget())
	{
		const AActor* Target = Command.TargetActor;
		if (!IsValid(Target))
		{
			return EOutbreakCommandRejection::InvalidTarget;
		}
		const UOutbreakHealthComponent* TargetHealth = Target->FindComponentByClass<UOutbreakHealthComponent>();
		if (TargetHealth && !TargetHealth->IsAlive())
		{
			return EOutbreakCommandRejection::InvalidTarget;
		}
		if (!OutbreakTeam::IsHostile(Member->GetOwner(), Target))
		{
			return EOutbreakCommandRejection::TargetNotHostile;
		}
	}
	return EOutbreakCommandRejection::None;
}

TArray<FOutbreakCommandResult> UOutbreakSquadComponent::AuthIssueCommand(const FOutbreakSquadCommand& Command,
	const FOutbreakCommandRecipients& Recipients)
{
	TArray<FOutbreakCommandResult> Results;
	if (!HasOwnerAuthority())
	{
		return Results;
	}

	for (UOutbreakSquadMemberComponent* Member : ResolveRecipients(Recipients))
	{
		FOutbreakCommandResult& Result = Results.AddDefaulted_GetRef();
		Result.Member = Member;
		Result.Rejection = ValidateCommandForMember(Command, Member);

		if (Result.WasAccepted())
		{
			FOutbreakSquadCommand MemberCommand = Command;
			// Hold / Defend / Cover without a position mean "here, where you stand".
			if (MemberCommand.UsesPosition() && !MemberCommand.bHasTargetLocation)
			{
				MemberCommand.TargetLocation = Member->GetMemberLocation();
				MemberCommand.bHasTargetLocation = true;
			}
			Member->AssignCommand(MemberCommand);
		}

		UE_LOG(LogOutbreak, Log, TEXT("%s: %s order for %s: %s."), *GetNameSafe(GetOwner()),
			*OutbreakSquadCommands::GetCommandDisplayName(Command.Type).ToString(),
			*Member->GetDisplayName().ToString(),
			*OutbreakSquadCommands::GetRejectionText(Result.Rejection).ToString());
	}

	const FOutbreakCommandResult* FirstAccepted = Results.FindByPredicate([](const FOutbreakCommandResult& Result) { return Result.WasAccepted(); });
	if (FirstAccepted)
	{
		OnCommandAccepted.Broadcast(this, Command.Type, FirstAccepted->Member);
	}
	return Results;
}

int32 UOutbreakSquadComponent::AuthCancelCommand(const FOutbreakCommandRecipients& Recipients)
{
	if (!HasOwnerAuthority())
	{
		return 0;
	}

	int32 Cancelled = 0;
	for (UOutbreakSquadMemberComponent* Member : ResolveRecipients(Recipients))
	{
		if (Member != PlayerMember && Member->GetCommandStatus() == EOutbreakSquadCommandStatus::Active)
		{
			Member->ClearCommand(EOutbreakSquadCommandStatus::Cancelled);
			++Cancelled;
		}
	}
	return Cancelled;
}

void UOutbreakSquadComponent::SetFormation(EOutbreakFormation NewFormation)
{
	if (HasOwnerAuthority())
	{
		AuthSetFormation(NewFormation);
	}
	else
	{
		ServerSetFormation(NewFormation);
	}
}

void UOutbreakSquadComponent::CycleFormation()
{
	// The Stealth formation belongs to stealth mode; the cycle covers the others.
	const int32 Count = static_cast<int32>(EOutbreakFormation::Loose) + 1;
	const int32 Current = FMath::Min(static_cast<int32>(Formation), Count - 1);
	SetFormation(static_cast<EOutbreakFormation>((Current + 1) % Count));
}

void UOutbreakSquadComponent::ServerSetFormation_Implementation(EOutbreakFormation NewFormation)
{
	AuthSetFormation(NewFormation);
}

void UOutbreakSquadComponent::AuthSetFormation(EOutbreakFormation NewFormation)
{
	if (Formation != NewFormation)
	{
		Formation = NewFormation;
		UE_LOG(LogOutbreak, Log, TEXT("%s: formation %s."), *GetNameSafe(GetOwner()), *UEnum::GetValueAsString(NewFormation));
		OnFormationChanged.Broadcast(this, Formation);
	}
}

void UOutbreakSquadComponent::OnRep_Formation()
{
	OnFormationChanged.Broadcast(this, Formation);
}

TArray<UOutbreakSquadMemberComponent*> UOutbreakSquadComponent::GetTeammates() const
{
	TArray<UOutbreakSquadMemberComponent*> Result;
	for (UOutbreakSquadMemberComponent* Member : Members)
	{
		if (Member && Member != PlayerMember)
		{
			Result.Add(Member);
		}
	}
	return Result;
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
