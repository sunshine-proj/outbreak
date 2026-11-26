#include "Squad/OutbreakSquadSettings.h"

#include "Squad/OutbreakTeammateCharacter.h"

UOutbreakSquadSettings::UOutbreakSquadSettings()
{
	DefaultTeammateClass = AOutbreakTeammateCharacter::StaticClass();
}
