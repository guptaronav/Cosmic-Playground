#include "App.h"
#include <iostream>

int main() {
    App app(1280, 720, "Cosmic Playground");

    if (!app.init()) {
        std::cerr << "Failed to initialise Cosmic Playground.\n";
        return 1;
    }

    app.run();
    return 0;
}
