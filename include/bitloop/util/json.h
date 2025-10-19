#pragma once
#include <bitloop/core/debug.h>
#include <bitloop/platform/platform_macros.h>
#include <bitloop/util/text_util.h>

BL_PUSH_PRECISE
#include "nlohmann/json.hpp"
BL_POP_PRECISE
#include <regex>

BL_BEGIN_NS;

namespace JSON
{
    using namespace nlohmann;

    template<typename T>
    std::string markCleanFloat(T value, int max_decimals = 6, T precision = 0.0f) {
        return "CLEANFLOAT(" + TextUtil::floatToCleanString<T>(value, max_decimals, precision) + ")";
    }

    inline std::string unmarkCleanFloats(const std::string& json)
    {
        static const std::regex cleanFloatRegex("\"CLEANFLOAT\\(([^\\)]+)\\)\"");
        return std::regex_replace(json, cleanFloatRegex, "$1");
    }

    std::string json_add_leading_zeros(const std::string& s);
    std::string json_remove_key_quotes(const std::string& s);
    std::string json_add_key_quotes(const std::string& s);
}

BL_END_NS;
