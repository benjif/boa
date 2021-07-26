#include "boa/utl/macros.h"
#include "boa/ngn/engine.h"

int main(int argc, char **argv) {
    LOG_INFO("(Global) Started");

    boa::ngn::Engine engine;
    engine.run();

    LOG_INFO("(Global) Exiting");
    return EXIT_SUCCESS;
}
