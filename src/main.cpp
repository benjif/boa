#include "boa/utl/macros.h"
#include "boa/gme/engine.h"

int main(int argc, char **argv) {
    LOG_INFO("(Global) Started");

    boa::gme::Engine engine;
    engine.run();

    LOG_INFO("(Global) Exiting");
    return EXIT_SUCCESS;
}
