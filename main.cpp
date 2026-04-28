#include <exception>
#include <iostream>

#include "core/GameController.h"

/*
 Entry program

 cmdline arg:
 No command-line arguments needed, arguments will be ignored

 Exit code:
 0: normal exit, 1: fatal error occured
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