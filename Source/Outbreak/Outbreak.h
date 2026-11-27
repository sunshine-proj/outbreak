#pragma once

#include "CoreMinimal.h"
#include "ProfilingDebugging/CsvProfiler.h"
#include "Stats/Stats.h"

/** General log category for the Outbreak game module. Add per-system categories when a system needs its own verbosity control. */
OUTBREAK_API DECLARE_LOG_CATEGORY_EXTERN(LogOutbreak, Log, All);

/**
 * Profiling (Phase 15). `stat Outbreak` shows these cycle counters and counts in game; Unreal
 * Insights shows them as CPU timing events; `csvprofile start` / `csvprofile stop` records them
 * in the "Outbreak" CSV category for comparing runs. See Docs/Phase15-EditorSetup.md.
 */
DECLARE_STATS_GROUP(TEXT("Outbreak"), STATGROUP_Outbreak, STATCAT_Advanced);
CSV_DECLARE_CATEGORY_MODULE_EXTERN(OUTBREAK_API, Outbreak);

/** Times the enclosing scope in `stat Outbreak`, Insights and the Outbreak CSV category. StatId comes from DECLARE_CYCLE_STAT. */
#define OUTBREAK_SCOPE_CYCLE_COUNTER(StatId) \
	SCOPE_CYCLE_COUNTER(StatId); \
	CSV_SCOPED_TIMING_STAT(Outbreak, StatId)
