#include "nlohmann/json.hpp"
#include <bitloop/core/debug.h>



// JSON support must go *after* class definition
inline void to_json(nlohmann::json& j, const FiniteDouble& fd) {
    j = static_cast<double>(fd);  // or fd.get();
}

inline void from_json(const nlohmann::json& j, FiniteDouble& fd) {
    fd = j.get<double>();
}
