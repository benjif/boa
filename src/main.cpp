//#include "boa/gfx/renderer.h"
#include "boa/gfx/scene.h"
#include "boa/macros.h"

int main(int argc, char **argv) {
    boa::gfx::Scene scene;
    scene.add_from_gltf_file("models/BoxInterleaved.gltf");

    scene.debug_print();

    // scene.add_from_obj_png_files("models/box.obj", "textures/box.png");

    //boa::gfx::Renderer renderer;
    //renderer.load_scene(scene);
    //renderer.run();

    return 0;
}