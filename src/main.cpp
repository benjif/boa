#include "boa/utl/macros.h"
#include "boa/ngn/engine.h"

int main(int argc, char **argv) {
    LOG_INFO("(Global) Started");

    std::string default_path(argc > 1 ? argv[1] : "save/default.json");

    boa::ngn::Engine engine(default_path);
    engine.run();

    LOG_INFO("(Global) Exiting");
    return EXIT_SUCCESS;
}
