#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Combat/OutbreakHealthComponent.h"
#include "EngineUtils.h"
#include "Game/OutbreakPlayerState.h"
#include "Player/OutbreakPlayerCharacter.h"
#include "Squad/OutbreakSquadComponent.h"
#include "Squad/OutbreakSquadConfiguration.h"
#include "Squad/OutbreakSquadMemberComponent.h"
#include "Squad/OutbreakSquadRoleDefinition.h"
#include "Squad/OutbreakSquadSettings.h"
#include "Squad/OutbreakTeammateCharacter.h"
#include "Tests/OutbreakTestWorld.h"
#include "UObject/Package.h"
#include "Weapons/OutbreakAmmoTypeDefinition.h"
#include "Weapons/OutbreakFireBehavior.h"
#include "Weapons/OutbreakWeaponComponent.h"
#include "Weapons/OutbreakWeaponDefinition.h"

namespace OutbreakSquadTests
{
	FOutbreakSquadSizeLimits MakeLimits(int32 Min, int32 Default, int32 Max)
	{
		FOutbreakSquadSizeLimits Limits;
		Limits.MinSquadSize = Min;
		Limits.DefaultSquadSize = Default;
		Limits.MaxSquadSize = Max;
		return Limits;
	}

	UOutbreakSquadConfiguration* MakeConfiguration(int32 SquadSize, int32 MinOverride = 0, int32 MaxOverride = 0)
	{
		UOutbreakSquadConfiguration* Configuration = NewObject<UOutbreakSquadConfiguration>(GetTransientPackage());
		Configuration->SquadSize = SquadSize;
		Configuration->MinSquadSizeOverride = MinOverride;
		Configuration->MaxSquadSizeOverride = MaxOverride;
		return Configuration;
	}

	int32 CountTeammates(UWorld* World)
	{
		int32 Count = 0;
		for (TActorIterator<AOutbreakTeammateCharacter> It(World); It; ++It)
		{
			if (IsValid(*It))
			{
				++Count;
			}
		}
		return Count;
	}

	/** The global limits the tests assume (the shipped defaults). Tests are skipped if the project changed them. */
	bool HasDefaultProjectLimits()
	{
		const FOutbreakSquadSizeLimits& Limits = GetDefault<UOutbreakSquadSettings>()->SizeLimits;
		return Limits.MinSquadSize == 2 && Limits.DefaultSquadSize == 4 && Limits.MaxSquadSize == 6;
	}
}

using namespace OutbreakSquadTests;

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOutbreakSquadLimitRulesTest, "Outbreak.Squad.Rules.Limits",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

/** Configurations narrow the project limits but cannot widen them (D-008). */
bool FOutbreakSquadLimitRulesTest::RunTest(const FString& Parameters)
{
	const FOutbreakSquadSizeLimits Global = MakeLimits(2, 4, 6);
	FOutbreakSquadSizeLimits Resolved;
	FText Error;

	TestTrue(TEXT("No overrides"), OutbreakSquadRules::ResolveLimits(Global, 0, 0, Resolved, Error));
	TestEqual(TEXT("Inherits min"), Resolved.MinSquadSize, 2);
	TestEqual(TEXT("Inherits default"), Resolved.DefaultSquadSize, 4);
	TestEqual(TEXT("Inherits max"), Resolved.MaxSquadSize, 6);

	TestTrue(TEXT("Narrowing"), OutbreakSquadRules::ResolveLimits(Global, 3, 5, Resolved, Error));
	TestEqual(TEXT("Narrowed min"), Resolved.MinSquadSize, 3);
	TestEqual(TEXT("Narrowed max"), Resolved.MaxSquadSize, 5);

	TestTrue(TEXT("Widening is clamped"), OutbreakSquadRules::ResolveLimits(Global, 1, 10, Resolved, Error));
	TestEqual(TEXT("Min cannot go below project min"), Resolved.MinSquadSize, 2);
	TestEqual(TEXT("Max cannot go above project max"), Resolved.MaxSquadSize, 6);

	TestTrue(TEXT("Default follows a raised min"), OutbreakSquadRules::ResolveLimits(Global, 5, 0, Resolved, Error));
	TestEqual(TEXT("Default clamped up to min"), Resolved.DefaultSquadSize, 5);

	TestTrue(TEXT("Default follows a lowered max"), OutbreakSquadRules::ResolveLimits(Global, 0, 3, Resolved, Error));
	TestEqual(TEXT("Default clamped down to max"), Resolved.DefaultSquadSize, 3);

	TestFalse(TEXT("Inverted overrides rejected"), OutbreakSquadRules::ResolveLimits(Global, 5, 3, Resolved, Error));
	TestFalse(TEXT("Error explains rejection"), Error.IsEmpty());

	TestFalse(TEXT("Invalid global limits rejected"), OutbreakSquadRules::ResolveLimits(MakeLimits(4, 2, 6), 0, 0, Resolved, Error));

	// Raising the project maximum needs no code change.
	TestTrue(TEXT("Larger project maximum"), OutbreakSquadRules::ResolveLimits(MakeLimits(2, 4, 10), 0, 0, Resolved, Error));
	TestEqual(TEXT("Maximum of ten"), Resolved.MaxSquadSize, 10);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOutbreakSquadSizeRulesTest, "Outbreak.Squad.Rules.Size",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FOutbreakSquadSizeRulesTest::RunTest(const FString& Parameters)
{
	const FOutbreakSquadSizeLimits Limits = MakeLimits(2, 4, 6);
	int32 Size = 0;
	FText Error;

	TestTrue(TEXT("0 resolves to default"), OutbreakSquadRules::ResolveSquadSize(Limits, 0, Size, Error));
	TestEqual(TEXT("Default size is four"), Size, 4);

	for (int32 Requested = 2; Requested <= 6; ++Requested)
	{
		TestTrue(FString::Printf(TEXT("Size %d allowed"), Requested), OutbreakSquadRules::ResolveSquadSize(Limits, Requested, Size, Error));
		TestEqual(FString::Printf(TEXT("Size %d resolved"), Requested), Size, Requested);
	}

	TestFalse(TEXT("Size 1 rejected"), OutbreakSquadRules::ResolveSquadSize(Limits, 1, Size, Error));
	TestFalse(TEXT("Size 7 rejected"), OutbreakSquadRules::ResolveSquadSize(Limits, 7, Size, Error));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOutbreakSquadRegistrationTest, "Outbreak.Squad.Component.RegistrationAndRemoval",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FOutbreakSquadRegistrationTest::RunTest(const FString& Parameters)
{
	FOutbreakTestWorld TestWorld;
	AOutbreakPlayerState* PlayerState = TestWorld.Spawn<AOutbreakPlayerState>();
	AOutbreakPlayerCharacter* Player = TestWorld.Spawn<AOutbreakPlayerCharacter>();
	if (!TestNotNull(TEXT("Player state"), PlayerState) || !TestNotNull(TEXT("Player"), Player))
	{
		return false;
	}

	UOutbreakSquadComponent* Squad = PlayerState->GetSquadComponent();
	TestTrue(TEXT("Limits set"), Squad->SetSizeLimits(MakeLimits(2, 4, 6)));
	TestFalse(TEXT("Invalid limits refused"), Squad->SetSizeLimits(MakeLimits(5, 4, 3)));

	UOutbreakSquadMemberComponent* PlayerMember = Player->GetSquadMemberComponent();
	TestTrue(TEXT("Player registered"),
		Squad->RegisterPlayerMember(PlayerMember) == EOutbreakSquadRegistrationResult::Registered);
	TestTrue(TEXT("Player member recorded"), Squad->GetPlayerMember() == PlayerMember);
	TestTrue(TEXT("Player knows it is the player member"), PlayerMember->IsPlayerMember());
	TestEqual(TEXT("Player counted"), Squad->GetMemberCount(), 1);
	TestFalse(TEXT("One member is below the minimum"), Squad->IsCompositionValid());

	TArray<AOutbreakTeammateCharacter*> Teammates;
	for (int32 Index = 0; Index < 6; ++Index)
	{
		Teammates.Add(TestWorld.Spawn<AOutbreakTeammateCharacter>(FVector(200.0f * (Index + 1), 0.0f, 0.0f)));
	}

	for (int32 Index = 0; Index < 5; ++Index)
	{
		TestTrue(FString::Printf(TEXT("Teammate %d registered"), Index),
			Squad->RegisterMember(Teammates[Index]->GetSquadMemberComponent()) == EOutbreakSquadRegistrationResult::Registered);
	}
	TestEqual(TEXT("Six members"), Squad->GetMemberCount(), 6);
	TestTrue(TEXT("Full at max"), Squad->IsFull());
	TestTrue(TEXT("Valid composition"), Squad->IsCompositionValid());

	TestTrue(TEXT("Seventh member refused"),
		Squad->RegisterMember(Teammates[5]->GetSquadMemberComponent()) == EOutbreakSquadRegistrationResult::SquadFull);
	TestTrue(TEXT("Duplicate refused"),
		Squad->RegisterMember(Teammates[0]->GetSquadMemberComponent()) == EOutbreakSquadRegistrationResult::AlreadyMember);
	TestTrue(TEXT("Null refused"), Squad->RegisterMember(nullptr) == EOutbreakSquadRegistrationResult::Invalid);

	TSet<int32> Ids;
	for (const UOutbreakSquadMemberComponent* Member : Squad->GetMembers())
	{
		Ids.Add(Member->GetMemberId());
		TestTrue(TEXT("Member points back at the squad"), Member->GetSquad() == Squad);
	}
	TestEqual(TEXT("Member IDs are unique"), Ids.Num(), 6);
	TestTrue(TEXT("Find by ID"), Squad->FindMemberById(PlayerMember->GetMemberId()) == PlayerMember);

	UOutbreakSquadMemberComponent* Removed = Teammates[2]->GetSquadMemberComponent();
	TestTrue(TEXT("Removal succeeds"), Squad->RemoveMember(Removed));
	TestFalse(TEXT("Removed member no longer in squad"), Squad->ContainsMember(Removed));
	TestNull(TEXT("Removed member has no squad"), Removed->GetSquad());
	TestEqual(TEXT("Removed member lost its ID"), Removed->GetMemberId(), static_cast<int32>(INDEX_NONE));
	TestFalse(TEXT("Second removal fails"), Squad->RemoveMember(Removed));
	TestEqual(TEXT("Five members"), Squad->GetMemberCount(), 5);

	TestTrue(TEXT("Freed slot can be filled"),
		Squad->RegisterMember(Teammates[5]->GetSquadMemberComponent()) == EOutbreakSquadRegistrationResult::Registered);

	// A member of one squad cannot join another.
	AOutbreakPlayerState* OtherPlayerState = TestWorld.Spawn<AOutbreakPlayerState>();
	TestTrue(TEXT("Member of another squad refused"),
		OtherPlayerState->GetSquadComponent()->RegisterMember(Teammates[0]->GetSquadMemberComponent())
			== EOutbreakSquadRegistrationResult::InOtherSquad);

	// Destroying a teammate removes it from the squad.
	const int32 CountBeforeDestroy = Squad->GetMemberCount();
	Teammates[1]->Destroy();
	TestEqual(TEXT("Destroyed member removed"), Squad->GetMemberCount(), CountBeforeDestroy - 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOutbreakSquadMemberDeathTest, "Outbreak.Squad.Component.MemberDeath",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

/** Dead members stay registered, stop counting as alive and are no longer available for orders. */
bool FOutbreakSquadMemberDeathTest::RunTest(const FString& Parameters)
{
	FOutbreakTestWorld TestWorld;
	AOutbreakPlayerState* PlayerState = TestWorld.Spawn<AOutbreakPlayerState>();
	AOutbreakPlayerCharacter* Player = TestWorld.Spawn<AOutbreakPlayerCharacter>();
	AOutbreakTeammateCharacter* Alpha = TestWorld.Spawn<AOutbreakTeammateCharacter>(FVector(200.0f, 0.0f, 0.0f));
	AOutbreakTeammateCharacter* Bravo = TestWorld.Spawn<AOutbreakTeammateCharacter>(FVector(400.0f, 0.0f, 0.0f));
	if (!PlayerState || !Player || !Alpha || !Bravo)
	{
		AddError(TEXT("Spawning failed"));
		return false;
	}

	UOutbreakSquadComponent* Squad = PlayerState->GetSquadComponent();
	Squad->RegisterPlayerMember(Player->GetSquadMemberComponent());
	Squad->RegisterMember(Alpha->GetSquadMemberComponent());
	Squad->RegisterMember(Bravo->GetSquadMemberComponent());

	TestEqual(TEXT("All alive"), Squad->GetAliveMemberCount(), 3);
	TestEqual(TEXT("Two available teammates"), Squad->GetAvailableTeammates().Num(), 2);

	UOutbreakSquadMemberComponent* AlphaMember = Alpha->GetSquadMemberComponent();
	Alpha->GetHealthComponent()->SetCanBeIncapacitated(true);
	Alpha->GetHealthComponent()->ApplyDamage(10000.0f, nullptr);
	TestTrue(TEXT("Incapacitated status"), AlphaMember->GetStatus() == EOutbreakSquadMemberStatus::Incapacitated);
	TestEqual(TEXT("Incapacitated still alive"), Squad->GetAliveMemberCount(), 3);
	TestFalse(TEXT("Incapacitated not available"), AlphaMember->IsAvailable());
	TestEqual(TEXT("Health fraction zero"), AlphaMember->GetHealthFraction(), 0.0f);

	Alpha->GetHealthComponent()->ApplyDamage(1.0f, nullptr);
	TestTrue(TEXT("Dead status"), AlphaMember->GetStatus() == EOutbreakSquadMemberStatus::Dead);
	TestEqual(TEXT("Alive count drops"), Squad->GetAliveMemberCount(), 2);
	TestTrue(TEXT("Dead member stays registered"), Squad->ContainsMember(AlphaMember));
	TestEqual(TEXT("One available teammate"), Squad->GetAvailableTeammates().Num(), 1);
	TestFalse(TEXT("Dead teammate's weapons disabled"), Alpha->GetWeaponComponent()->AreWeaponsEnabled());

	Player->GetHealthComponent()->ApplyDamage(10000.0f, nullptr);
	TestTrue(TEXT("Dead player is still the player member"), Squad->GetPlayerMember() == Player->GetSquadMemberComponent());
	TestEqual(TEXT("Alive count after player death"), Squad->GetAliveMemberCount(), 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOutbreakSquadInitializationMatrixTest, "Outbreak.Squad.Initialization.SizeMatrix",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

/** The required test matrix (SKILLS.md Skill 12): squads of 2 to 6, player included. */
bool FOutbreakSquadInitializationMatrixTest::RunTest(const FString& Parameters)
{
	if (!HasDefaultProjectLimits())
	{
		AddWarning(TEXT("Project squad limits differ from 2/4/6; size matrix skipped."));
		return true;
	}

	for (int32 SquadSize = 2; SquadSize <= 6; ++SquadSize)
	{
		FOutbreakTestWorld TestWorld;
		AOutbreakPlayerState* PlayerState = TestWorld.Spawn<AOutbreakPlayerState>();
		AOutbreakPlayerCharacter* Player = TestWorld.Spawn<AOutbreakPlayerCharacter>();
		UOutbreakSquadComponent* Squad = PlayerState->GetSquadComponent();

		const bool bCreated = Squad->InitializeSquad(MakeConfiguration(SquadSize), Player);
		TestTrue(FString::Printf(TEXT("Squad of %d created"), SquadSize), bCreated);
		TestEqual(FString::Printf(TEXT("Squad of %d has %d members"), SquadSize, SquadSize), Squad->GetMemberCount(), SquadSize);
		TestEqual(FString::Printf(TEXT("Squad of %d spawned %d teammates"), SquadSize, SquadSize - 1),
			CountTeammates(TestWorld.Get()), SquadSize - 1);
		TestTrue(FString::Printf(TEXT("Squad of %d includes the player"), SquadSize),
			Squad->GetPlayerMember() == Player->GetSquadMemberComponent());
		TestTrue(FString::Printf(TEXT("Squad of %d is valid"), SquadSize), Squad->IsCompositionValid());
		TestFalse(FString::Printf(TEXT("Squad of %d cannot be initialized twice"), SquadSize),
			Squad->InitializeSquad(MakeConfiguration(SquadSize), Player));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOutbreakSquadInitializationRulesTest, "Outbreak.Squad.Initialization.DefaultsAndRejection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FOutbreakSquadInitializationRulesTest::RunTest(const FString& Parameters)
{
	if (!HasDefaultProjectLimits())
	{
		AddWarning(TEXT("Project squad limits differ from 2/4/6; test skipped."));
		return true;
	}

	{
		FOutbreakTestWorld TestWorld;
		UOutbreakSquadComponent* Squad = TestWorld.Spawn<AOutbreakPlayerState>()->GetSquadComponent();
		AOutbreakPlayerCharacter* Player = TestWorld.Spawn<AOutbreakPlayerCharacter>();
		TestTrue(TEXT("Configuration without size uses default"), Squad->InitializeSquad(MakeConfiguration(0), Player));
		TestEqual(TEXT("Default squad of four"), Squad->GetMemberCount(), 4);
	}
	{
		FOutbreakTestWorld TestWorld;
		UOutbreakSquadComponent* Squad = TestWorld.Spawn<AOutbreakPlayerState>()->GetSquadComponent();
		AOutbreakPlayerCharacter* Player = TestWorld.Spawn<AOutbreakPlayerCharacter>();
		TestFalse(TEXT("Size above project max rejected"), Squad->InitializeSquad(MakeConfiguration(7), Player));
		TestEqual(TEXT("Only the player after rejection"), Squad->GetMemberCount(), 1);
		TestEqual(TEXT("No teammates spawned"), CountTeammates(TestWorld.Get()), 0);
	}
	{
		FOutbreakTestWorld TestWorld;
		UOutbreakSquadComponent* Squad = TestWorld.Spawn<AOutbreakPlayerState>()->GetSquadComponent();
		AOutbreakPlayerCharacter* Player = TestWorld.Spawn<AOutbreakPlayerCharacter>();
		TestFalse(TEXT("Size above mission max rejected"), Squad->InitializeSquad(MakeConfiguration(5, 0, 3), Player));
	}
	{
		FOutbreakTestWorld TestWorld;
		UOutbreakSquadComponent* Squad = TestWorld.Spawn<AOutbreakPlayerState>()->GetSquadComponent();
		AOutbreakPlayerCharacter* Player = TestWorld.Spawn<AOutbreakPlayerCharacter>();
		TestTrue(TEXT("Mission narrowing applied"), Squad->InitializeSquad(MakeConfiguration(0, 0, 3), Player));
		TestEqual(TEXT("Default clamped to mission max"), Squad->GetMemberCount(), 3);
		TestEqual(TEXT("Mission max replicated in limits"), Squad->GetSizeLimits().MaxSquadSize, 3);
	}
	{
		FOutbreakTestWorld TestWorld;
		UOutbreakSquadComponent* Squad = TestWorld.Spawn<AOutbreakPlayerState>()->GetSquadComponent();
		AOutbreakPlayerCharacter* Player = TestWorld.Spawn<AOutbreakPlayerCharacter>();
		TestTrue(TEXT("Size override replaces configuration size"), Squad->InitializeSquad(MakeConfiguration(2), Player, 5));
		TestEqual(TEXT("Override size used"), Squad->GetMemberCount(), 5);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOutbreakSquadMemberConfigTest, "Outbreak.Squad.Initialization.MemberConfiguration",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

/** Teammates get names, roles and equipment from their configuration, falling back to role defaults. */
bool FOutbreakSquadMemberConfigTest::RunTest(const FString& Parameters)
{
	if (!HasDefaultProjectLimits())
	{
		AddWarning(TEXT("Project squad limits differ from 2/4/6; test skipped."));
		return true;
	}

	UOutbreakAmmoTypeDefinition* Ammo = NewObject<UOutbreakAmmoTypeDefinition>(GetTransientPackage());
	auto MakeWeapon = [Ammo]()
	{
		UOutbreakWeaponDefinition* Weapon = NewObject<UOutbreakWeaponDefinition>(GetTransientPackage());
		FOutbreakFireModeDefinition& Mode = Weapon->FireModes.AddDefaulted_GetRef();
		Mode.FireBehavior = NewObject<UOutbreakHitscanFireBehavior>(Weapon);
		Weapon->CompatibleAmmoTypes.Add(Ammo);
		return Weapon;
	};
	UOutbreakWeaponDefinition* RoleRifle = MakeWeapon();
	UOutbreakWeaponDefinition* SniperRifle = MakeWeapon();

	UOutbreakSquadRoleDefinition* Rifleman = NewObject<UOutbreakSquadRoleDefinition>(GetTransientPackage());
	Rifleman->DefaultWeapons.Add(RoleRifle);
	FOutbreakAmmoAmount& RoleAmmo = Rifleman->DefaultAmmoReserves.AddDefaulted_GetRef();
	RoleAmmo.AmmoType = Ammo;
	RoleAmmo.Count = 90;

	UOutbreakSquadConfiguration* Configuration = MakeConfiguration(4);
	FOutbreakSquadMemberConfig& Named = Configuration->Teammates.AddDefaulted_GetRef();
	Named.DisplayName = FText::FromString(TEXT("Hawk"));
	Named.Role = Rifleman;
	FOutbreakSquadMemberConfig& Override = Configuration->Teammates.AddDefaulted_GetRef();
	Override.Role = Rifleman;
	Override.Weapons.Add(SniperRifle);
	Configuration->FillerTeammate.Role = Rifleman;

	FOutbreakTestWorld TestWorld;
	UOutbreakSquadComponent* Squad = TestWorld.Spawn<AOutbreakPlayerState>()->GetSquadComponent();
	AOutbreakPlayerCharacter* Player = TestWorld.Spawn<AOutbreakPlayerCharacter>();
	TestTrue(TEXT("Squad created"), Squad->InitializeSquad(Configuration, Player));

	const TArray<UOutbreakSquadMemberComponent*> Members = Squad->GetMembers();
	if (!TestEqual(TEXT("Four members"), Members.Num(), 4))
	{
		return false;
	}

	// Members[0] is the player; teammates follow in configuration order.
	UOutbreakSquadMemberComponent* HawkMember = Members[1];
	TestEqual(TEXT("Configured name"), HawkMember->GetDisplayName().ToString(), FString(TEXT("Hawk")));
	TestTrue(TEXT("Configured role"), HawkMember->GetRole() == Rifleman);
	TestTrue(TEXT("Role weapon given"), HawkMember->GetWeaponComponent()->GetActiveWeaponDefinition() == RoleRifle);
	TestEqual(TEXT("Role ammunition given"), HawkMember->GetWeaponComponent()->GetReserveAmmo(Ammo), 90);

	TestTrue(TEXT("Member weapon overrides role"), Members[2]->GetWeaponComponent()->GetActiveWeaponDefinition() == SniperRifle);
	TestEqual(TEXT("Override teammate has one weapon"), Members[2]->GetWeaponComponent()->GetWeaponCount(), 1);

	TestFalse(TEXT("Filler teammate has a generated name"), Members[3]->GetDisplayName().IsEmpty());
	TestTrue(TEXT("Filler teammate uses filler role"), Members[3]->GetRole() == Rifleman);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
