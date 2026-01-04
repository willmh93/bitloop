#pragma once

#include <bitloop/core/types.h>

// Physics types

BL_BEGIN_NS

struct MassForceParticle : public DVec2
{
    f64 r;
    f64 fx;
    f64 fy;
    f64 vx;
    f64 vy;
    f64 mass;
};

BL_END_NS;
