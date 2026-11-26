#include "UI/OutbreakHUDText.h"

#define LOCTEXT_NAMESPACE "OutbreakHUDText"

namespace OutbreakHUDText
{
	FText BuildAwarenessText(float Awareness, bool bAlarmRaised)
	{
		constexpr int32 MeterLength = 10;
		FText Line = FText::GetEmpty();
		if (Awareness >= 1.0f)
		{
			Line = LOCTEXT("Detected", "DETECTED");
		}
		else if (Awareness > 0.01f)
		{
			const int32 Filled = FMath::Clamp(FMath::CeilToInt(Awareness * MeterLength), 1, MeterLength - 1);
			const FString Meter = FString::ChrN(Filled, TEXT('#')) + FString::ChrN(MeterLength - Filled, TEXT('-'));
			Line = FText::Format(LOCTEXT("Noticed", "NOTICED [{0}]"), FText::FromString(Meter));
		}

		if (!bAlarmRaised)
		{
			return Line;
		}
		return Line.IsEmpty()
			? LOCTEXT("Alarm", "ALARM RAISED")
			: FText::Format(LOCTEXT("AlarmAndLine", "{0}\nALARM RAISED"), Line);
	}
}

#undef LOCTEXT_NAMESPACE
