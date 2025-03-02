// test_main.cpp
#define CATCH_CONFIG_RUNNER
#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_session.hpp>

int main(int argc, char* argv[])
{
    // Perform any global initialization here

    // Run Catch2 tests
    int result = Catch::Session().run(argc, argv);

    // Perform any global cleanup here

    return result;
}