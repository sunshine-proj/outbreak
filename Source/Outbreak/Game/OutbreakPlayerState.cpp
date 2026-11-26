#include "Game/OutbreakPlayerState.h"

#include "Squad/OutbreakSquadCommsComponent.h"
#include "Squad/OutbreakSquadComponent.h"

AOutbreakPlayerState::AOutbreakPlayerState()
{
	SquadComponent = CreateDefaultSubobject<UOutbreakSquadComponent>(TEXT("SquadComponent"));
	SquadCommsComponent = CreateDefaultSubobject<UOutbreakSquadCommsComponent>(TEXT("SquadCommsComponent"));
}
