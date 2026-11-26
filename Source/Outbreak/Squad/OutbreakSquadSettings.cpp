#include "Squad/OutbreakSquadSettings.h"

#include "Squad/OutbreakTeammateCharacter.h"

UOutbreakSquadSettings::UOutbreakSquadSettings()
{
	DefaultTeammateClass = AOutbreakTeammateCharacter::StaticClass();

	auto AddSpacing = [this](EOutbreakFormation Formation, float Spacing)
	{
		FOutbreakFormationSpacing& Entry = FormationSpacings.AddDefaulted_GetRef();
		Entry.Formation = Formation;
		Entry.Spacing = Spacing;
	};
	AddSpacing(EOutbreakFormation::Column, 200.0f);
	AddSpacing(EOutbreakFormation::Wedge, 250.0f);
	AddSpacing(EOutbreakFormation::Line, 250.0f);
	AddSpacing(EOutbreakFormation::Defensive, 250.0f);
	AddSpacing(EOutbreakFormation::Loose, 250.0f);
	AddSpacing(EOutbreakFormation::Stealth, 150.0f);
}

float UOutbreakSquadSettings::GetFormationSpacing(EOutbreakFormation Formation) const
{
	for (const FOutbreakFormationSpacing& Entry : FormationSpacings)
	{
		if (Entry.Formation == Formation)
		{
			return FMath::Max(50.0f, Entry.Spacing);
		}
	}
	return DefaultFormationSpacing;
}
