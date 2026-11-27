#include "Outbreak.h"

#include "Modules/ModuleManager.h"

DEFINE_LOG_CATEGORY(LogOutbreak);

CSV_DEFINE_CATEGORY_MODULE(OUTBREAK_API, Outbreak, true);

IMPLEMENT_PRIMARY_GAME_MODULE(FDefaultGameModuleImpl, Outbreak, "Outbreak");
