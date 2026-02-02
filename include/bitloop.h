#pragma once

#include <bitloop/platform/platform.h>

// core
#include <bitloop/core/scene.h>
#include <bitloop/core/project.h>
#include <bitloop/core/viewport.h>
#include <bitloop/core/main_window.h>

#include <bitloop/core/physics2d.h>

#include <bitloop/nanovgx/nano_canvas.h>
#include <bitloop/nanovgx/nano_bitmap.h>
#include <bitloop/nanovgx/nano_shader_surface.h>

// imgui
#include <bitloop/imguix/imguix.h>

// util
#include <bitloop/util/compression.h>
#include <bitloop/util/constexpr_dispatch.h>
#include <bitloop/util/math_util.h>
#include <bitloop/util/text_util.h>
#include <bitloop/util/json.h>

int bitloop_main(int, char* []);
