#include "Squad/OutbreakSquadComms.h"

#define LOCTEXT_NAMESPACE "OutbreakSquadComms"

namespace OutbreakSquadComms
{
	bool IsUrgent(EOutbreakCallout Callout)
	{
		return Callout == EOutbreakCallout::ManDown || Callout == EOutbreakCallout::MemberKilled || Callout == EOutbreakCallout::AlarmRaised;
	}

	bool IsCalloutAllowed(EOutbreakCallout Callout, double Now, double LastAnyCallout, double LastSameCallout,
		const FOutbreakSquadCommsSettings& Settings)
	{
		if (Now - LastSameCallout < Settings.SameCalloutCooldown)
		{
			return false;
		}
		return IsUrgent(Callout) || Now - LastAnyCallout >= Settings.MinInterval;
	}

	FText FormatSubtitle(const FText& SpeakerName, const FText& Line, const FText& SubjectName)
	{
		FFormatNamedArguments Arguments;
		Arguments.Add(TEXT("Name"), SubjectName);
		return FText::Format(LOCTEXT("Subtitle", "{0}: {1}"), SpeakerName, FText::Format(Line, Arguments));
	}
}

#undef LOCTEXT_NAMESPACE
