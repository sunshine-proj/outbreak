#include "Game/OutbreakPlayerState.h"

#include "Squad/OutbreakSquadComponent.h"

AOutbreakPlayerState::AOutbreakPlayerState()
{
	SquadComponent = CreateDefaultSubobject<UOutbreakSquadComponent>(TEXT("SquadComponent"));
}
