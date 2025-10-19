#pragma once

#include <bitloop/platform/platform.h>

// core
#include <bitloop/core/scene.h>
#include <bitloop/core/project.h>
#include <bitloop/core/viewport.h>
#include <bitloop/core/main_window.h>

// imgui
#include <bitloop/imguix/imgui_custom.h>

// util
#include <bitloop/util/compression.h>
#include <bitloop/util/constexpr_dispatch.h>
#include <bitloop/util/math_util.h>
#include <bitloop/util/text_util.h>
#include <bitloop/util/json.h>

int bitloop_main(int, char* []);
