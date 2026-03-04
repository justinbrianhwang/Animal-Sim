#include "core/Application.h"
#include <cstdio>
#include <cstdlib>

int main() {
    try {
        animsim::Application app;
        app.run();
    } catch (const std::exception& e) {
        std::fprintf(stderr, "Fatal error: %s\n", e.what());
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
