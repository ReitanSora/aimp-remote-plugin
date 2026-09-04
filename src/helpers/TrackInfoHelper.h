#include "../core/Plugin.h"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

json GetTrackInfo(MyPlugin* plugin);