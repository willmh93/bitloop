#pragma once

BL_BEGIN_NS

class Viewport;

struct MouseInfo
{
    Viewport* viewport = nullptr;
    f64 client_x = 0;
    f64 client_y = 0;
    f64 stage_x = 0;
    f64 stage_y = 0;
    f128 world_x = 0;
    f128 world_y = 0;
    int scroll_delta = 0;
    bool pressed = false;
};

struct FingerInfo
{
    Viewport* ctx_owner = nullptr;
    i64 fingerId;
    f64 x, y;
};

BL_END_NS
