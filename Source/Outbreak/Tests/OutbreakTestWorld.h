#pragma once

#include "CoreMinimal.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Engine/World.h"

/**
 * Creates a minimal transient game world for automation tests and destroys it on scope exit.
 *
 * The world has no game mode and does not begin play: actors are spawned and their components
 * initialized (PostInitializeComponents runs), but BeginPlay and timers do not. Tests that need
 * a running game should use a map-based functional test instead.
 */
class FOutbreakTestWorld
{
public:

	FOutbreakTestWorld()
	{
		World = UWorld::CreateWorld(EWorldType::Game, /*bInformEngineOfWorld=*/ false, TEXT("OutbreakTestWorld"));
	}

	~FOutbreakTestWorld()
	{
		if (World)
		{
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
