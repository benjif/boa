#include "boa/macros.h"
#include "boa/gfx/asset/model.h"
#include "boa/gfx/renderer.h"

int main(int argc, char **argv) {
    boa::gfx::Model model;
    model.open_gltf_file("models/cube/Cube.gltf");
    //model.debug_print();

    boa::gfx::Model model2;
    model2.open_gltf_file("models/BoxInterleaved.gltf");

    boa::gfx::Renderer renderer;
    renderer.load_model(model, "cube");
    renderer.load_model(model2, "box_interleaved");
    renderer.run();

    return 0;
}