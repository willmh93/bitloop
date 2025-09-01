#pragma once
#include <bitloop/platform/platform_macros.h>

BL_PUSH_PRECISE
#include "nlohmann/json.hpp"
BL_POP_PRECISE
#include <regex>

namespace JSON
{
    using namespace nlohmann;

    template<typename T>
    std::string markCleanFloat(T value, int max_decimals = 6, T precision = 0.0f) {
        return "CLEANFLOAT(" + TextUtil::floatToCleanString<T>(value, max_decimals, precision) + ")";
    }


    inline std::string unquoteCleanFloats(const std::string& json)
    {
        static const std::regex cleanFloatRegex("\"CLEANFLOAT\\(([^\\)]+)\\)\"");
        return std::regex_replace(json, cleanFloatRegex, "$1");
    }
}
