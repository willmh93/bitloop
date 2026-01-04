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

    /// JSON pre/post processing helpers, e.g.
    /// 
    /// info["quality"] = JSON::markCleanFloat(quality, 3);
    /// std::string txt = JSON::unmarkCleanFloats(info.dump());
    ///
    /// "quality" is no longer a string and is guaranteed to have 3 decimal places

    // returns float as a string with guaranteed precision
    template<typename T>
    std::string markCleanFloat(T value, int max_decimals = 6, T precision = 0.0f) {
        return "CLEANFLOAT(" + text::floatToCleanString<T>(value, max_decimals, precision, true, true) + ")";
    }

    // strip out all CLEANFLOAT("XXX") and replace with numeric value
    inline std::string unmarkCleanFloats(const std::string& json)
    {
        static const std::regex cleanFloatRegex("\"CLEANFLOAT\\(([^\\)]+)\\)\"");
        return std::regex_replace(json, cleanFloatRegex, "$1");
    }

    // helpers for aggressive compression (use carefully, results in invalid json unless key quotes/leading zeros readded)
    // breaks if keys are empty strings, contain spaces/punctuation, etc. (but works fine for simple key names)
    std::string json_add_leading_zeros(const std::string& s);
    std::string json_remove_key_quotes(const std::string& s);
    std::string json_add_key_quotes(const std::string& s);
}

BL_END_NS;
