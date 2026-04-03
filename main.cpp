#include <exception>
#include <iostream>

#include "core/GameController.h"

/**
 * What it does:
 * Starts the Color Tube game application.
 * Inputs:
 * Command-line arguments are ignored.
 * Outputs:
 * Returns 0 on normal exit and 1 on fatal error.
 */
int main() {
    try {
        GameController controller;
        controller.run();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Fatal error: " << error.what() << "\n";
        return 1;
    } catch (...) {
        std::cerr << "Fatal error: unknown exception\n";
        return 1;
    }
}
