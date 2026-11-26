#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"

#include "OutbreakSquadComms.generated.h"

class USoundBase;

/** Things teammates say (Phase 13 squad communication). Append new values at the end. */
UENUM(BlueprintType)
enum class EOutbreakCallout : uint8
{
	/** An order was accepted. */
	Acknowledge,
	/** First sight of a hostile. */
	Contact,
	Reloading,
	/** A squadmate went down. Subtitles may use {Name}. */
	ManDown,
	/** A squadmate died. Subtitles may use {Name}. */
	MemberKilled,
	/** The base alarm went up. */
	AlarmRaised,
	ObjectiveComplete
};

/** The lines for one callout: a sound and its subtitle per variant (same index). */
USTRUCT(BlueprintType)
struct OUTBREAK_API FOutbreakVoiceLines
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Voice")
	TArray<TObjectPtr<USoundBase>> Sounds;

	/** "{Name} is down!" — {Name} is the squadmate the callout is about. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Voice")
	TArray<FText> Subtitles;

	int32 GetVariantCount() const { return FMath::Max(Sounds.Num(), Subtitles.Num()); }
};

/**
 * A voice: what one kind of soldier says for each callout. Assigned on squad roles (a gruff
 * support gunner, a calm medic); soldiers without one use the project default voice
 * (Project Settings → Outbreak Squad). Sounds use Unreal Audio (D-021).
 */
UCLASS(BlueprintType, Const)
class OUTBREAK_API UOutbreakVoiceDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Voice")
	TMap<EOutbreakCallout, FOutbreakVoiceLines> Lines;
};

/** Pacing of squad chatter. In Project Settings → Outbreak Squad. */
USTRUCT(BlueprintType)
struct OUTBREAK_API FOutbreakSquadCommsSettings
{
	GENERATED_BODY()

	/** Seconds between any two callouts (urgent ones — man down, killed, alarm — ignore this). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Comms", meta = (ClampMin = "0.0", Units = "s"))
	float MinInterval = 2.0f;

	/** Seconds before the same callout can be heard again. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Comms", meta = (ClampMin = "0.0", Units = "s"))
	float SameCalloutCooldown = 8.0f;

	/** Seconds a subtitle stays on screen. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Comms", meta = (ClampMin = "0.5", Units = "s"))
	float SubtitleDuration = 3.0f;
};

namespace OutbreakSquadComms
{
	/** Urgent callouts skip the minimum interval (they still respect their own cooldown). */
	OUTBREAK_API bool IsUrgent(EOutbreakCallout Callout);

	/** Whether Callout may play at Now, given when any callout and this callout last played. */
	OUTBREAK_API bool IsCalloutAllowed(EOutbreakCallout Callout, double Now, double LastAnyCallout, double LastSameCallout,
		const FOutbreakSquadCommsSettings& Settings);

	/** "Hawk: Viper is down!" — Line with {Name} replaced by SubjectName, prefixed by the speaker. */
	OUTBREAK_API FText FormatSubtitle(const FText& SpeakerName, const FText& Line, const FText& SubjectName);
}
