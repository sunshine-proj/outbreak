#pragma once

#include "CoreMinimal.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Engine/Engine.h"
#include "Engine/World.h"

/**
 * Creates a minimal transient game world for automation tests and destroys it on scope exit.
 *
 * Actors are initialized for play, so spawned actors run InitializeComponent and
 * PostInitializeComponents. The world has no game mode and does not begin play: BeginPlay is not
 * called and timers never fire. Tests that need a running game should use a map-based
 * functional test instead.
 *
 * The world runs as NM_Standalone, so every actor has authority and Server/Multicast RPCs run locally.
 */
class FOutbreakTestWorld
{
public:

	FOutbreakTestWorld()
	{
		World = UWorld::CreateWorld(EWorldType::Game, /*bInformEngineOfWorld=*/ false, TEXT("OutbreakTestWorld"));

		FWorldContext& WorldContext = GEngine->CreateNewWorldContext(EWorldType::Game);
		WorldContext.SetCurrentWorld(World);

		World->InitializeActorsForPlay(FURL());
	}

	~FOutbreakTestWorld()
	{
		if (World)
		{
			GEngine->DestroyWorldContext(World);
			World->DestroyWorld(/*bInformEngineOfWorld=*/ false);
			World->RemoveFromRoot();
		}
	}

	FOutbreakTestWorld(const FOutbreakTestWorld&) = delete;
	FOutbreakTestWorld& operator=(const FOutbreakTestWorld&) = delete;

	UWorld* Get() const { return World; }

	template <typename ActorType>
	ActorType* Spawn(const FVector& Location = FVector::ZeroVector)
	{
		FActorSpawnParameters SpawnParameters;
		SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		return World->SpawnActor<ActorType>(ActorType::StaticClass(), Location, FRotator::ZeroRotator, SpawnParameters);
	}

private:

	UWorld* World = nullptr;
};

#endif // WITH_DEV_AUTOMATION_TESTS
