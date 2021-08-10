#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_vulkan.h"
#include "glm/gtc/type_ptr.hpp"
#include "boa/ngn/engine.h"
#include "boa/ngn/object.h"
#include "boa/ngn/file_dialog.h"
#include <filesystem>
#include <fstream>
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
    float *current_scale = glm::value_ptr(transformable.scale);

    float current_rotation[3] = {
        glm::degrees(glm::yaw(transformable.orientation)),
        glm::degrees(glm::pitch(transformable.orientation)),
        glm::degrees(glm::roll(transformable.orientation)),
    };

    ImGuizmo::DecomposeMatrixToComponents(transform_matrix, current_translation, current_rotation, current_scale);

    ImGui::InputFloat3("T", current_translation);
    ImGui::InputFloat3("R", current_rotation);
    ImGui::InputFloat3("S", current_scale);

    transformable.scale = glm::vec3(std::max(0.001f, transformable.scale.x),
                                    std::max(0.001f, transformable.scale.y),
                                    std::max(0.001f, transformable.scale.z));

    ImGuizmo::RecomposeMatrixFromComponents(current_translation, current_rotation, current_scale, transform_matrix);

    /*transformable.orientation = glm::quat(glm::vec3(glm::radians(current_rotation[2]),
                                                    glm::radians(current_rotation[0]),
                                                    glm::radians(current_rotation[1])));*/
    transformable.orientation = glm::quat(glm::vec3(glm::radians(current_rotation[0]),
                                                    glm::radians(current_rotation[1]),
                                                    glm::radians(current_rotation[2])));

    physics_controller.sync_physics_transform(selected_entity);

    static bool use_snap(false);
    ImGui::Checkbox("Use snapping", &use_snap);

    static float translate_snap[3]  = { 1.0f, 1.0f, 1.0f },
                 rotate_snap[3]     = { 45.0f, 45.0f, 45.0f },
                 scale_snap[3]      = { 1.0f, 1.0f, 1.0f };
    float snap[3];
    switch (current_tool) {
    case ImGuizmo::TRANSLATE:
        ImGui::InputFloat3("Snap", translate_snap);
        snap[0] = translate_snap[0];
        snap[1] = translate_snap[1];
        snap[2] = translate_snap[2];
        break;
    case ImGuizmo::ROTATE:
        ImGui::InputFloat("Angle Snap", rotate_snap);
        snap[0] = rotate_snap[0];
        snap[1] = rotate_snap[1];
        snap[2] = rotate_snap[2];
        break;
    case ImGuizmo::SCALE:
        ImGui::InputFloat("Scale Snap", scale_snap);
        snap[0] = scale_snap[0];
        snap[1] = scale_snap[1];
        snap[2] = scale_snap[2];
        break;
    }

    glm::mat4 &view_m = renderer.get_view();
    glm::mat4 projection_m = renderer.get_projection();
    projection_m[1][1] *= -1;

    float *view_m_raw = glm::value_ptr(view_m);
    float *projection_m_raw = glm::value_ptr(projection_m);

    ImGuiIO &io = ImGui::GetIO();
    ImGuizmo::SetRect(0, 0, io.DisplaySize.x, io.DisplaySize.y);

    // Can't use currently because `ViewManipulate` does not set `IsOver()`
    /*ImGuizmo::ViewManipulate(view_m_raw,
                             10.f,
                             ImVec2(io.DisplaySize.x - 128, 0),
                             ImVec2(128, 128),
                             0x10101010);*/

    ImGuizmo::Manipulate(view_m_raw,
                         projection_m_raw,
                         current_tool, ImGuizmo::WORLD, transform_matrix, NULL, use_snap ? snap : NULL);

    ImGui::LabelText("", "Currently Selected: %d", last_selected_entity.value());
}

void Engine::draw_inactive_animation() const {
    ImGui::GetStyle().Alpha = 0.25f;

    int dummy_id = 0;
    float dummy_speed = 0.0f;
    bool dummy_loop = false;
    ImGui::InputInt("Animation ID", &dummy_id);
    ImGui::InputFloat("Speed", &dummy_speed);
    ImGui::Checkbox("Loop", &dummy_loop);

    ImGui::Button("Play Animation");

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

    auto &animated = entity_group.get_component<boa::gfx::Animated>(selected_entity);

    static int id = 0;
    static float speed = 1.0f;
    static bool loop = false;
    ImGui::InputInt("Animation ID", &id);
    ImGui::InputFloat("Speed", &speed);
    ImGui::Checkbox("Loop", &loop);

    if (id >= animated.animations.size())
        id = animated.animations.size() - 1;
    else if (id < 0)
        id = 0;

    if (speed < 0.0f)
        speed = 0.0f;
    else if (speed > 100.0f)
        speed = 100.0f;

    if (ImGui::Button("Play Animation"))
        animation_controller.play_animation(selected_entity, id, loop, speed);
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
            ImGui::MenuItem("Statistics", nullptr, &m_ui_state.show_statistics);
            ImGui::MenuItem("Script Editor", nullptr, &m_ui_state.show_script_editor);
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

    if (ImGui::Button("Add to World") && current_entity.has_value()) {
        auto &base_renderable = entity_group.get_component<boa::gfx::BaseRenderable>(current_entity.value());
        auto &loaded_asset = entity_group.get_component<boa::ngn::LoadedAsset>(current_entity.value());

        uint32_t new_entity = entity_group.copy_entity<boa::ngn::LoadedAsset, boa::gfx::Animated>(current_entity.value());
        entity_group.enable_and_make<boa::gfx::Renderable>(new_entity, base_renderable.model_id);
        entity_group.enable_and_make<boa::ngn::EngineSelectable>(new_entity, false);

        glm::vec3 new_pos = camera.get_position() + camera.get_target() * 5.0f;
        entity_group.enable_and_make<boa::gfx::Transformable>(new_entity,
                                                              glm::quat{ 0.0f, 0.0f, 0.0f, 0.0f },
                                                              std::move(new_pos),
                                                              glm::vec3{ 1.0f, 1.0f, 1.0f });
        physics_controller.add_entity(new_entity, new_mass);
    }

    ImGui::End();
}

void Engine::draw_statistics_window() const {
    ImGui::Begin("Statistics", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoFocusOnAppearing);

    static const size_t sample_interval = 16;
    static const size_t sample_count = 32;

    static size_t frame_count = 0;
    frame_count++;

    static auto last_time = std::chrono::high_resolution_clock::now();
    static auto current_time = last_time;

    current_time = std::chrono::high_resolution_clock::now();
    float time_change =
        std::chrono::duration<float, std::chrono::seconds::period>(current_time - last_time).count();
    last_time = current_time;

    static float fps_samples[sample_count] = {};
    static size_t offset = 0;

    if (frame_count % sample_interval == 0) {
        fps_samples[offset] = 1.0f / time_change;
        offset = (offset + 1) % IM_ARRAYSIZE(fps_samples);
    }

    float average = 0.0f;
    for (size_t i = 0; i < sample_count; i++)
        average += fps_samples[i];
    average /= sample_count;

    char overlay[32];
    sprintf(overlay, "%f", average);
    ImGui::PlotLines("FPS", fps_samples, IM_ARRAYSIZE(fps_samples), offset, overlay, -1.0f, 1.0f, ImVec2(0, 40.0f));
    ImGui::LabelText(std::to_string(entity_group.size()).c_str(), "Entity Count");

    ImGui::End();
}

void Engine::draw_script_editor_window() {
    ImGui::SetWindowSize(ImVec2(800, 600), ImGuiCond_FirstUseEver);
    ImGui::Begin("Script Editor", nullptr, ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_MenuBar);

    static TextEditor editor;
    static bool new_script_open = false;
    static std::string open_script_path;

    auto lang = TextEditor::LanguageDefinition::Lua();
    editor.SetLanguageDefinition(lang);

    auto cursor_position = editor.GetCursorPosition();

    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("New")) {
                open_script_path.clear();
                editor = TextEditor{};
            }

            if (ImGui::MenuItem("Open"))
                FileDialog::get().open("Open script from file");

            if (ImGui::MenuItem("Save")) {
                if (open_script_path.size() != 0) {
                    std::ofstream out_script_file(open_script_path);
                    out_script_file << editor.GetText();
                    out_script_file.close();
                } else {
                    FileDialog::get().open("Save script to file");
                }
            }

            if (ImGui::MenuItem("Save as"))
                FileDialog::get().open("Save script to file");

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Script")) {
            if (ImGui::MenuItem("Run")) {
                std::string current_script_state = editor.GetText();
                script_controller.run_from_buffer(current_script_state);
            }

            ImGui::EndMenu();
        }

        ImGui::EndMenuBar();
    }

    if (FileDialog::get().draw("Open script from file", FileDialog::Mode::Open, { ".lua" })) {
        open_script_path = FileDialog::get().get_selected_path();
        new_script_open = true;
    }

    if (FileDialog::get().draw("Save script to file", FileDialog::Mode::Save)) {
        std::ofstream out_script_file(FileDialog::get().get_selected_path());
        out_script_file << editor.GetText();
        out_script_file.close();
    }

    static std::string script;
    if (new_script_open) {
        std::ifstream script_file(open_script_path);
        if (!script_file.good()) {
            m_ui_state.show_script_editor = false;
            return;
        }

        std::stringstream script_stream;
        script_stream << script_file.rdbuf();
        script = script_stream.str();
        script_file.close();

        editor.SetText(script);
        new_script_open = false;
    }

    editor.Render("TextEditor");

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
    if (m_ui_state.show_statistics)
        draw_statistics_window();

    // TODO: support multiple text editor windows at once
    // (separate stuff into separate class)
    if (m_ui_state.show_script_editor)
        draw_script_editor_window();

    //ImGui::ShowDemoWindow();

    ImGui::EndFrame();
}

void Engine::cleanup_interface() {
    FileDialog::destroy();
}

}
