#include "boa/macros.h"
#include "boa/gfx/asset/model.h"
#include "boa/gfx/renderer.h"

int main(int argc, char **argv) {
    LOG_INFO("(Global) Started");

    //boa::gfx::Model model;
    //model.open_gltf_file("models/cube/Cube.gltf");

    //boa::gfx::Model model2;
    //model2.open_gltf_file("models/domino_crown.gltf");

    boa::gfx::Model virtual_city;
    virtual_city.open_gltf_file("models/virtual_city/VC.gltf");

    boa::gfx::Renderer renderer;
    //renderer.load_model(model, "cube");
    //renderer.load_model(model2, "domino_crown");
    renderer.load_model(virtual_city, "virtual_city");

    //boa::gfx::Skybox daytime = renderer.load_skybox("skybox/daytime.png");
    //renderer.set_skybox(daytime);

    renderer.run();

    LOG_INFO("(Global) Exiting");
    return EXIT_SUCCESS;
}