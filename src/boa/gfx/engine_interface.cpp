#include "boa/gfx/engine_interface.h"
#include "boa/gfx/lighting.h"
#include "boa/ecs/ecs.h"
#include "glm/gtc/type_ptr.hpp"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_vulkan.h"
#include "imgui.h"

namespace boa::gfx {

void draw_engine_interface() {
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    
    ImGui::Begin("Boa Window");

    static float ambient_lighting[3] = { 0.1f, 0.1f, 0.1f };
    static float diffuse_lighting[3] = { 0.68f, 0.68f, 0.68f };
    static float specular_lighting[3] = { 0.3f, 0.3f, 0.3f };
    static float direction[3] = { -0.2f, 1.0f, -0.3f };

    ImGui::BeginChild("Global Lighting");

    ImGui::ColorEdit3("Ambient", ambient_lighting);
    ImGui::ColorEdit3("Diffuse", diffuse_lighting);
    ImGui::ColorEdit3("Specular", specular_lighting);

    ImGui::InputFloat3("Direction", direction);

    ImGui::EndChild();

    auto &entity_group = boa::ecs::EntityGroup::get();
    auto g_light_e = entity_group.find_first_entity_with_component<boa::gfx::GlobalLight>();
    if (g_light_e.has_value()) {
        auto &g_light = entity_group.get_component<boa::gfx::GlobalLight>(g_light_e.value());
        g_light.direction = glm::make_vec3(direction);
        g_light.ambient = glm::make_vec3(ambient_lighting);
        g_light.diffuse = glm::make_vec3(diffuse_lighting);
        g_light.specular = glm::make_vec3(specular_lighting);
    }

    /*ImGui::Button("Hello!");
    static float pos[3] = { 0.0f, 0.0f, 0.0f };
    static char buf[6];
    ImGui::InputText("string", buf, IM_ARRAYSIZE(buf));
    ImGui::InputFloat3("Position", &pos[0]);*/

    ImGui::End();
}

}
