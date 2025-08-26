find_package(Catch2 3 CONFIG REQUIRED)

message(STATUS "Loading tests...")

file(GLOB UTILITY_TESTS CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/unit/utility/*.cpp"
)

add_executable(bitloop_tests
    ${UTILITY_TESTS}
)

target_link_libraries(bitloop_tests PRIVATE
    bitloop
    Catch2::Catch2WithMain
)
target_compile_features(bitloop_tests PRIVATE cxx_std_23)

include(Catch)
catch_discover_tests(bitloop_tests)
