#include "nlohmann_json.hpp"
#include <regex>

namespace JSON
{
    using namespace nlohmann;

    inline std::string markCleanFloat(float value, int max_decimals = 6, float precision = 0.0f) {
        return "CLEANFLOAT(" + Helpers::floatToCleanString(value, max_decimals, precision) + ")";
    }

    inline std::string unquoteCleanFloats(const std::string& json)
    {
        static const std::regex cleanFloatRegex("\"CLEANFLOAT\\(([^\\)]+)\\)\"");
        return std::regex_replace(json, cleanFloatRegex, "$1");
    }
}