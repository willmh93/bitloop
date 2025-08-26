#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include <bitloop/utility/math_util.h>

TEST_CASE("Vec2 length is correct")
{
    BL::FVec2 v{3.0f, 4.0f};
    REQUIRE(v.magnitude() == Catch::Approx(5.0f));
}
