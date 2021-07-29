#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_vulkan.h"
#include "glm/gtc/type_ptr.hpp"
#include "boa/ngn/engine.h"
#include "boa/ngn/object.h"
#include "boa/ngn/file_dialog.h"
#include <filesystem>
#include <string>

namespace boa::ngn {

void Engine::draw_global_lighting() {
    auto &entity_group = boa::ecs::EntityGroup::get();
    auto g_light_e = entity_group.find_first_entity_with_component<boa::gfx::GlobalLight>();
    if (g_light_e.has_value()) {
        auto &g_light = entity_group.get_component<boa::gfx::GlobalLight>(g_light_e.value());
        ImGui::ColorEdit3("Ambient", &g_light.ambient.x);
        ImGui::ColorEdit3("Diffuse", &g_light.diffuse.x);
        ImGui::ColorEdit3("Specular", &g_light.specular.x);
        ImGui::InputFloat3("Direction", &g_light.direction.x);
    }
}

void Engine::draw_inactive_toolbox(ImGuizmo::OPERATION current_tool) const {
    ImGui::GetStyle().Alpha = 0.25f;

    float dummy_trs[3] = { 0.0f, 0.0f, 0.0f };
    ImGui::InputFloat3("T", dummy_trs);
    ImGui::InputFloat3("R", dummy_trs);
    ImGui::InputFloat3("S", dummy_trs);

    bool dummy_snap = false;
    ImGui::Checkbox("Use snapping", &dummy_snap);

    float dummy_as_snap = 0.0f;
    switch (current_tool) {
    case ImGuizmo::TRANSLATE:
        ImGui::InputFloat3("Snap", dummy_trs);
        break;
    case ImGuizmo::ROTATE:
        ImGui::InputFloat("Angle Snap", &dummy_as_snap);
        break;
    case ImGuizmo::SCALE:
        ImGui::InputFloat("Scale Snap", &dummy_as_snap);
        break;
    }

    ImGui::LabelText("", "Currently Selected: None");

    ImGui::GetStyle().Alpha = 1.00f;
}

void Engine::draw_toolbox() {
    static ImGuizmo::OPERATION current_tool(ImGuizmo::TRANSLATE);

    if (!last_selected_entity.has_value() || m_mode == EngineMode::Physics) {
        draw_inactive_toolbox(current_tool);
        return;
    }

    auto &entity_group = boa::ecs::EntityGroup::get();

    uint32_t selected_entity = last_selected_entity.value();
    if (!entity_group.has_component<boa::gfx::Transformable>(selected_entity)) {
        draw_inactive_toolbox(current_tool);
        return;
    }

    if (ImGui::RadioButton("Translate", current_tool == ImGuizmo::TRANSLATE))
        current_tool = ImGuizmo::TRANSLATE;
    ImGui::SameLine();
    if (ImGui::RadioButton("Rotate", current_tool == ImGuizmo::ROTATE))
        current_tool = ImGuizmo::ROTATE;
    ImGui::SameLine();
    if (ImGui::RadioButton("Scale", current_tool == ImGuizmo::SCALE))
        current_tool = ImGuizmo::SCALE;

    auto &transformable = entity_group.get_component<boa::gfx::Transformable>(selected_entity);

    float *transform_matrix = glm::value_ptr(transformable.transform_matrix);
    float *current_translation = glm::value_ptr(transformable.translation);
    float *current_matrix_scale = glm::value_ptr(transformable.scale);

    float translation[3] = { current_translation[0], current_translation[1], current_translation[2] };
    float scale[3] = { current_matrix_scale[0], current_matrix_scale[1], current_matrix_scale[2] };
    float rotation[3] = {
        glm::degrees(glm::pitch(transformable.orientation)),
        glm::degrees(glm::yaw(transformable.orientation)),
        glm::degrees(glm::roll(transformable.orientation)),
    };

    ImGuizmo::DecomposeMatrixToComponents(transform_matrix, translation, rotation, scale);
    ImGui::InputFloat3("T", translation);
    ImGui::InputFloat3("R", rotation);
    ImGui::InputFloat3("S", scale);
    ImGuizmo::RecomposeMatrixFromComponents(translation, rotation, scale, transform_matrix);

    physics_controller.sync_physics_transform(selected_entity);

    static bool use_snap(false);
    ImGui::Checkbox("Use snapping", &use_snap);

    glm::vec3 snap;
    switch (current_tool) {
    case ImGuizmo::TRANSLATE:
        //snap = config.mSnapTranslation;
        snap = glm::vec3{ 1.0f, 1.0f, 1.0f };
        ImGui::InputFloat3("Snap", &snap.x);
        break;
    case ImGuizmo::ROTATE:
        snap = glm::vec3{ 30.0f, 30.0f, 30.0f };
        //snap = config.mSnapRotation;
        ImGui::InputFloat("Angle Snap", &snap.x);
        break;
    case ImGuizmo::SCALE:
        snap = glm::vec3{ 1.0f, 1.0f, 1.0f };
        //snap = config.mSnapScale;
        ImGui::InputFloat("Scale Snap", &snap.x);
        break;
    }

    glm::mat4 view_m = renderer.get_view();
    glm::mat4 projection_m = renderer.get_projection();
    projection_m[1][1] *= -1;

    float *view_m_raw = glm::value_ptr(view_m);
    float *projection_m_raw = glm::value_ptr(projection_m);

    ImGuiIO &io = ImGui::GetIO();
    ImGuizmo::SetRect(0, 0, io.DisplaySize.x, io.DisplaySize.y);
    ImGuizmo::Manipulate(view_m_raw,
                         projection_m_raw,
                         current_tool, ImGuizmo::WORLD, transform_matrix, NULL, use_snap ? &snap.x : NULL);

    ImGui::LabelText("", "Currently Selected: %d", last_selected_entity.value());
}

void Engine::draw_inactive_animation() const {
    ImGui::GetStyle().Alpha = 0.25f;

    ImGui::Button("Play 0 Animation");

    ImGui::GetStyle().Alpha = 1.00f;
}

void Engine::draw_animation() {
    if (!last_selected_entity.has_value()) {
        draw_inactive_animation();
        return;
    }

    auto &entity_group = boa::ecs::EntityGroup::get();

    uint32_t selected_entity = last_selected_entity.value();
    if (!entity_group.has_component<boa::gfx::Animated>(selected_entity)) {
        draw_inactive_animation();
        return;
    }

    if (ImGui::Button("Play 0 Animation"))
        animation_controller.play_animation(selected_entity, 0);
}

void Engine::draw_main_menu_bar() {
    bool show_save_dialog = false;

    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Save World"))
                FileDialog::get().open("Save scene to file");
            if (ImGui::MenuItem("Open World"))
                FileDialog::get().open("Open world from file");
            if (ImGui::MenuItem("Import Model"))
                FileDialog::get().open("Import model");
            if (ImGui::MenuItem("Exit")) {
                window.set_should_close();
            }

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Windows")) {
            ImGui::MenuItem("Scene Properties", nullptr, &m_ui_state.show_scene_properties);
            ImGui::MenuItem("Object Properties", nullptr, &m_ui_state.show_object_properties);
            ImGui::MenuItem("Create Entity", nullptr, &m_ui_state.show_entity_create);
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("View")) {
            if (ImGui::MenuItem("Renderer Bounding Boxes", nullptr, &m_ui_state.show_renderer_bounding_boxes))
                renderer.set_draw_bounding_boxes(m_ui_state.show_renderer_bounding_boxes);
            if (ImGui::MenuItem("Physics Bounding Boxes", nullptr, &m_ui_state.show_physics_bounding_boxes))
                physics_controller.debug_reset();
            ImGui::EndMenu();
        }

        const float item_spacing = ImGui::GetStyle().ItemSpacing.x;
        const float mode_label_padding = 10.0f;
        static float mode_label_width = 0.0f;

        ImGui::SameLine(ImGui::GetWindowWidth() - mode_label_width - item_spacing - mode_label_padding);

        ImGui::SetNextItemWidth(100.0f);

        switch (m_mode) {
        case EngineMode::Physics:
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.1, 0.2, 0.2, 1.0));
            ImGui::Text("(Physics)");
            break;
        case EngineMode::Edit:
        default:
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5, 0.5, 0.5, 1.0));
            ImGui::Text("(Editing)");
            break;
        }
        ImGui::PopStyleColor();

        mode_label_width = ImGui::GetItemRectSize().x;

        ImGui::EndMainMenuBar();
    }

    if (FileDialog::get().draw("Save scene to file", FileDialog::Mode::Save, { ".json" })) {
        m_state.save_to_json(asset_manager, physics_controller, FileDialog::get().get_selected_path().c_str());
        m_dialog_shown = DialogShown::None;
    }

    if (FileDialog::get().draw("Open world from file", FileDialog::Mode::Open, { ".json" })) {
        try {
            m_state.load_from_json(FileDialog::get().get_selected_path().c_str());
            asset_manager.reset();
            entity_group.clear_entities();
            m_state.add_entities(asset_manager, animation_controller, physics_controller);
            m_dialog_shown = DialogShown::None;
        } catch (const std::runtime_error &err) {
            LOG_WARN("(Engine) Failed to open world file");
        }
    }

    if (FileDialog::get().draw("Import model", FileDialog::Mode::Open, { ".gltf" })) {
        try {
            boa::gfx::glTFModel model;
            model.open_gltf_file(FileDialog::get().get_selected_path().c_str());
            uint32_t new_entity = asset_manager.load_model(model);
            animation_controller.load_animations(new_entity, model);
        } catch (const std::runtime_error &err) {
            LOG_WARN("(Engine) Failed to import model");
        }
    }
}

void Engine::draw_inactive_physics_properties() {
    ImGui::GetStyle().Alpha = 0.25f;

    ImGui::LabelText("Mass", "");

    ImGui::GetStyle().Alpha = 1.00f;
}

void Engine::draw_physics_properties() {
    if (!last_selected_entity.has_value()) {
        draw_inactive_physics_properties();
        return;
    }

    ImGui::LabelText("Mass", std::to_string(physics_controller.get_entity_mass(last_selected_entity.value())).c_str());

    glm::dvec3 linear_velocity = physics_controller.get_linear_velocity(last_selected_entity.value());
    ImGui::LabelText("Linear Velocity",
                     "(%.1f, %.1f, %.1f)", linear_velocity.x, linear_velocity.y, linear_velocity.z);
    glm::dvec3 angular_velocity = physics_controller.get_angular_velocity(last_selected_entity.value());
    ImGui::LabelText("Angular Velocity",
                     "(%.1f, %.1f, %.1f)", angular_velocity.x, angular_velocity.y, angular_velocity.z);
}

void Engine::draw_scene_properties_window() {
    ImGui::Begin("Scene Properties", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoFocusOnAppearing);

    ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_None;
    if (ImGui::BeginTabBar("Scene Properties", tab_bar_flags)) {
        if (ImGui::BeginTabItem("Global Lighting")) {
            draw_global_lighting();
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }

    ImGui::End();
}

void Engine::draw_object_properties_window() {
    ImGui::Begin("Object Properties", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoFocusOnAppearing);

    ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_None;
    if (ImGui::BeginTabBar("Object Properties", tab_bar_flags)) {
        if (ImGui::BeginTabItem("Toolbox")) {
            draw_toolbox();
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Animations")) {
            draw_animation();
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Physics")) {
            draw_physics_properties();
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }

    ImGui::End();
}

void Engine::draw_entity_create_window() {
    ImGui::Begin("Create Entity", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoFocusOnAppearing);

    static const std::string dummy_string = "";
    static const auto first_entity = entity_group.find_first_entity_with_component<boa::gfx::BaseRenderable, boa::ngn::LoadedAsset>();
    static const std::string *current_item_label =
        first_entity.has_value() ? &entity_group.get_component<boa::ngn::LoadedAsset>(first_entity.value()).resource_path : &dummy_string;
    static std::optional<uint32_t> current_entity = first_entity;

    if (ImGui::BeginCombo("Available Models", current_item_label->c_str(), ImGuiComboFlags_PopupAlignLeft)) {
        size_t i = 0;
        entity_group.for_each_entity_with_component<boa::gfx::BaseRenderable, boa::ngn::LoadedAsset>([&](uint32_t e_id) {
            auto &loaded_asset = entity_group.get_component<boa::ngn::LoadedAsset>(e_id);
            const bool is_selected = (current_entity == e_id);
            if (ImGui::Selectable(loaded_asset.resource_path.c_str(), is_selected)) {
                current_entity = e_id;
                current_item_label = &loaded_asset.resource_path;
            }
            if (is_selected)
                ImGui::SetItemDefaultFocus();

            i++;
            return Iteration::Continue;
        });

        ImGui::EndCombo();
    }

    static float new_mass = 0.0f;
    ImGui::InputFloat("Mass", &new_mass, 0.0f, 1000.0f, "%.3f");

    if (ImGui::Button("Add to scene") && current_entity.has_value()) {
        auto &base_renderable = entity_group.get_component<boa::gfx::BaseRenderable>(current_entity.value());
        auto &loaded_asset = entity_group.get_component<boa::ngn::LoadedAsset>(current_entity.value());

        uint32_t new_entity = entity_group.copy_entity<boa::ngn::LoadedAsset, boa::gfx::Animated>(current_entity.value());
        entity_group.enable_and_make<boa::gfx::Renderable>(new_entity, base_renderable.model_id);
        entity_group.enable_and_make<boa::ngn::EngineConfigurable>(new_entity, true);
        m_ui_state.show_object_properties = true;
        last_selected_entity = new_entity;

        glm::vec3 new_pos = camera.get_position() + camera.get_target() * 5.0f;
        entity_group.enable_and_make<boa::gfx::Transformable>(new_entity,
                                                              glm::quat{ 0.0f, 0.0f, 0.0f, 0.0f },
                                                              std::move(new_pos),
                                                              glm::vec3{ 1.0f, 1.0f, 1.0f });
        physics_controller.add_entity(new_entity, new_mass);
    }

    ImGui::End();
}

void Engine::draw_engine_interface() {
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    ImGuizmo::BeginFrame();

    draw_main_menu_bar();

    if (m_ui_state.show_scene_properties)
        draw_scene_properties_window();
    if (m_ui_state.show_object_properties)
        draw_object_properties_window();
    if (m_ui_state.show_entity_create)
        draw_entity_create_window();

    ImGui::EndFrame();
}

}
