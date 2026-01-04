#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include <bitloop.h>

using Catch::Approx;
using namespace bl;

TEST_CASE("roundUp/roundDown")
{
    std::cout << math::roundUp(17, 8) << "\n";
    std::cout << 17.0 / 8.0;

    REQUIRE(math::roundDown(17, 8) == 16);
    REQUIRE(math::roundUp(17, 8) == 24);

    REQUIRE(math::roundDown(15.65784, 0.025) == 15.65);
    REQUIRE(math::roundUp(15.65784, 0.025) == 15.675);

    REQUIRE(math::roundDown(15.65784, 0.01) == 15.657);
    REQUIRE(math::roundUp(15.65784, 0.01) == 15.658);

    
    //REQUIRE(math::roundDown(17.9, 0.5) == Approx(17.5));
    //REQUIRE(math::roundUp(17.1, 0.5) == Approx(17.5));
}

TEST_CASE("divisible handles integers and floats with tolerance")
{
    REQUIRE(math::divisible(16, 8));
    REQUIRE_FALSE(math::divisible(17, 8));

    REQUIRE(math::divisible(1.0, 0.1));      // 0.1 * 10
    REQUIRE(math::divisible(0.3, 0.1));      // tolerant of fp error
}

TEST_CASE("wrap clamps into [min,max) cyclically")
{
    REQUIRE(math::wrap(370.0, 0.0, 360.0) == Approx(10.0));
    REQUIRE(math::wrap(-10.0, 0.0, 360.0) == Approx(350.0));
}

TEST_CASE("Angle conversions are consistent")
{
    double d = 180.0;
    double r = math::toRadians(d);
    REQUIRE(math::toDegrees(r) == Approx(d));
}

TEST_CASE("closestAngleDifference / wrapRadians / wrapRadians2PI")
{
    double a = math::toRadians(350.0);
    double b = math::toRadians(10.0);
    double diff = math::closestAngleDifference(a, b);
    REQUIRE(diff == Approx(math::toRadians(20.0) * -1.0).margin(1e-12)); // shortest signed path from 350→10 is -20°

    REQUIRE(math::wrapRadians(10.0) == Approx(10.0 - 2 * M_PI));
    REQUIRE(math::wrapRadians2PI(7.0) == Approx(7.0 - 2 * M_PI));
}

TEST_CASE("avgAngle wraps correctly across 0/2π")
{
    double a = math::toRadians(350.0);
    double b = math::toRadians(10.0);
    REQUIRE(math::avgAngle(a, b) == Approx(0.0).margin(1e-12));
}

TEST_CASE("rotateOffset and reverseRotateOffset are inverses")
{
    DVec2 r = math::rotateOffset(1.0, 0.0, M_PI / 2);
    REQUIRE(r.x == Approx(0.0).margin(1e-12));
    REQUIRE(r.y == Approx(1.0).margin(1e-12));

    DVec2 back = math::reverseRotateOffset(r.x, r.y, M_PI / 2);
    REQUIRE(back.x == Approx(1.0).margin(1e-12));
    REQUIRE(back.y == Approx(0.0).margin(1e-12));
}

TEST_CASE("countDigits and countWholeDigits")
{
    REQUIRE(math::countDigits(0) == 1);
    REQUIRE(math::countDigits(9) == 1);
    REQUIRE(math::countDigits(10) == 2);
    REQUIRE(math::countDigits(-123) == 3);

    REQUIRE(math::countWholeDigits(0.5) == 1);
    REQUIRE(math::countWholeDigits(9.9) == 1);
    REQUIRE(math::countWholeDigits(10.0) == 2);
}
