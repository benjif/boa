#include "prettywriter.h"
//#include "writer.h"
#include "document.h"
#include "istreamwrapper.h"
#include "ostreamwrapper.h"
#include "boa/utl/macros.h"
#include "boa/ngn/engine_state.h"
#include "boa/ngn/object.h"
#include "boa/gfx/asset/asset_manager.h"
#include "boa/gfx/asset/animation.h"
#include "boa/phy/physics_controller.h"
#include "boa/ecs/ecs.h"
#include "glm/gtc/type_ptr.hpp"
#include "glm/gtx/quaternion.hpp"
#include "glm/gtx/transform.hpp"
#include <fstream>
#include <filesystem>
#include <unordered_map>

namespace boa::ngn {

void EngineState::save_to_json(boa::gfx::AssetManager &asset_manager,
                             boa::phy::PhysicsController &physics_controller,
                             const char *file_path) {
    auto &entity_group = boa::ecs::EntityGroup::get();

    rapidjson::Document document;
    document.SetObject();

    rapidjson::Value models(rapidjson::kArrayType);
    rapidjson::Value renderables(rapidjson::kArrayType);
    rapidjson::Value skyboxes(rapidjson::kArrayType);
    rapidjson::Value global_lights(rapidjson::kArrayType);
    rapidjson::Value point_lights(rapidjson::kArrayType);

    uint32_t model_count = 0;
    std::unordered_map<uint32_t, uint32_t> model_id_to_object_index;
    entity_group.for_each_entity_with_component<boa::gfx::Renderable>([&](uint32_t e_id) {
        auto &renderable = entity_group.get_component<boa::gfx::Renderable>(e_id);
        auto &transform = entity_group.get_component<boa::gfx::Transformable>(e_id);
        auto &loaded_asset = entity_group.get_component<boa::ngn::LoadedAsset>(e_id);
        auto &renderable_model = asset_manager.get_model(renderable.model_id);

        if (model_id_to_object_index.count(renderable.model_id) == 0) {
            rapidjson::Value model(rapidjson::kObjectType);
            model.AddMember(rapidjson::Value("path", document.GetAllocator()).Move(),
                            rapidjson::Value(loaded_asset.resource_path.c_str(), document.GetAllocator()).Move(),
                            document.GetAllocator());
            models.PushBack(model.Move(), document.GetAllocator());

            model_id_to_object_index[renderable.model_id] = model_count++;
        }

        rapidjson::Value renderable_object(rapidjson::kObjectType);
        renderable_object.AddMember("model",
                                    model_id_to_object_index.at(renderable.model_id),
                                    document.GetAllocator());
        renderable_object.AddMember("mass",
                                    (double)physics_controller.get_entity_mass(e_id),
                                    document.GetAllocator());

        switch (renderable_model.lighting) {
        case boa::gfx::LightingInteractivity::BlinnPhong:
            renderable_object.AddMember("lighting", "blinn_phong", document.GetAllocator());
            break;
        case boa::gfx::LightingInteractivity::Unlit:
            renderable_object.AddMember("lighting", "unlit", document.GetAllocator());
        default:
            break;
        }

        rapidjson::Value transform_object(rapidjson::kObjectType),
                         translation_array(rapidjson::kArrayType),
                         orientation_array(rapidjson::kArrayType),
                         scale_array(rapidjson::kArrayType);

        for (int i = 0; i < 3; i++) {
            translation_array.PushBack(transform.translation[i], document.GetAllocator());
            scale_array.PushBack(transform.scale[i], document.GetAllocator());
        }

        for (int i = 0; i < 4; i++)
            orientation_array.PushBack(transform.orientation[i], document.GetAllocator());

        transform_object.AddMember("translation", translation_array, document.GetAllocator());
        transform_object.AddMember("orientation", orientation_array, document.GetAllocator());
        transform_object.AddMember("scale", scale_array, document.GetAllocator());
        renderable_object.AddMember("transform", transform_object, document.GetAllocator());

        renderables.PushBack(renderable_object.Move(), document.GetAllocator());
        return Iteration::Continue;
    });

    entity_group.for_each_entity_with_component<boa::gfx::GPUSkybox>([&](uint32_t e_id) {
        rapidjson::Value skybox_object(rapidjson::kObjectType);
        rapidjson::Value paths_array(rapidjson::kArrayType);

        auto &loaded_asset = entity_group.get_component<boa::ngn::LoadedAsset>(e_id);

        std::string resource_paths = loaded_asset.resource_path;
        std::istringstream resource_paths_s(resource_paths);

        std::string resource_path;
        while (std::getline(resource_paths_s, resource_path, ';')) {
            paths_array.PushBack(rapidjson::Value(resource_path.c_str(), document.GetAllocator()),
                                 document.GetAllocator());
        }

        skybox_object.AddMember("paths", paths_array, document.GetAllocator());

        skyboxes.PushBack(skybox_object.Move(), document.GetAllocator());
        return Iteration::Continue;
    });

    entity_group.for_each_entity_with_component<boa::gfx::GlobalLight>([&](uint32_t e_id) {
        auto &global_light = entity_group.get_component<boa::gfx::GlobalLight>(e_id);

        rapidjson::Value global_light_object(rapidjson::kObjectType),
                         direction_array(rapidjson::kArrayType),
                         ambient_array(rapidjson::kArrayType),
                         diffuse_array(rapidjson::kArrayType),
                         specular_array(rapidjson::kArrayType);

        for (int i = 0; i < 3; i++) {
            direction_array.PushBack(global_light.direction[i], document.GetAllocator());
            ambient_array.PushBack(global_light.ambient[i], document.GetAllocator());
            diffuse_array.PushBack(global_light.diffuse[i], document.GetAllocator());
            specular_array.PushBack(global_light.specular[i], document.GetAllocator());
        }

        global_light_object.AddMember("direction", direction_array.Move(), document.GetAllocator());
        global_light_object.AddMember("ambient", ambient_array.Move(), document.GetAllocator());
        global_light_object.AddMember("diffuse", diffuse_array.Move(), document.GetAllocator());
        global_light_object.AddMember("specular", specular_array.Move(), document.GetAllocator());

        global_lights.PushBack(global_light_object.Move(), document.GetAllocator());
        return Iteration::Continue;
    });

    entity_group.for_each_entity_with_component<boa::gfx::PointLight>([&](uint32_t e_id) {
        auto &point_light = entity_group.get_component<boa::gfx::PointLight>(e_id);

        rapidjson::Value point_light_object(rapidjson::kObjectType),
                         position_array(rapidjson::kArrayType),
                         ambient_array(rapidjson::kArrayType),
                         diffuse_array(rapidjson::kArrayType),
                         specular_array(rapidjson::kArrayType);

        for (int i = 0; i < 3; i++) {
            position_array.PushBack(point_light.position[i], document.GetAllocator());
            ambient_array.PushBack(point_light.ambient[i], document.GetAllocator());
            diffuse_array.PushBack(point_light.diffuse[i], document.GetAllocator());
            specular_array.PushBack(point_light.specular[i], document.GetAllocator());
        }

        point_light_object.AddMember("position", position_array.Move(), document.GetAllocator());
        point_light_object.AddMember("ambient", ambient_array.Move(), document.GetAllocator());
        point_light_object.AddMember("diffuse", diffuse_array.Move(), document.GetAllocator());
        point_light_object.AddMember("specular", specular_array.Move(), document.GetAllocator());
        point_light_object.AddMember("constant", point_light.constant, document.GetAllocator());
        point_light_object.AddMember("linear", point_light.linear, document.GetAllocator());
        point_light_object.AddMember("quadratic", point_light.quadratic, document.GetAllocator());

        point_lights.PushBack(point_light_object.Move(), document.GetAllocator());
        return Iteration::Continue;
    });

    document.AddMember("models", models, document.GetAllocator());
    document.AddMember("renderables", renderables, document.GetAllocator());
    document.AddMember("skyboxes", skyboxes, document.GetAllocator());
    document.AddMember("global_lights", global_lights, document.GetAllocator());
    document.AddMember("point_lights", point_lights, document.GetAllocator());

    std::ofstream json_stream(file_path);
    rapidjson::OStreamWrapper json_stream_wrapper(json_stream);

    rapidjson::PrettyWriter<rapidjson::OStreamWrapper> writer(json_stream_wrapper);
    document.Accept(writer);
}

void EngineState::load_from_json(const char *file_path) {
    assert(std::filesystem::exists(std::filesystem::path(file_path)));

    m_default_skybox = 0;
    m_models.clear();
    m_renderables.clear();
    m_skyboxes.clear();
    m_global_lights.clear();
    m_point_lights.clear();

    std::ifstream json_stream(file_path);
    rapidjson::IStreamWrapper json_stream_wrapper(json_stream);

    rapidjson::Document document;
    document.ParseStream(json_stream_wrapper);

    assert(document.IsObject());

    if (document.HasMember("models")) {
        const rapidjson::Value &models = document["models"];
        assert(models.IsArray());
        m_models.reserve(models.Size());
        for (auto &model : models.GetArray()) {
            assert(model["path"].IsString());
            m_models.emplace_back(model["path"].GetString());
        }
    }

    if (document.HasMember("renderables")) {
        const rapidjson::Value &renderables = document["renderables"];
        assert(renderables.IsArray());
        m_renderables.reserve(renderables.Size());
        for (auto &renderable : renderables.GetArray()) {
            assert(renderable["model"].IsInt());
            assert(renderable["mass"].IsFloat());
            assert(renderable["transform"].IsObject());
            assert(renderable["lighting"].IsString());
            assert(renderable["transform"]["translation"].IsArray() && renderable["transform"]["translation"].Size() == 3);
            assert(renderable["transform"]["orientation"].IsArray() && renderable["transform"]["orientation"].Size() == 4);
            assert(renderable["transform"]["scale"].IsArray() && renderable["transform"]["scale"].Size() == 3);

            SavedRenderable new_renderable;
            new_renderable.model = renderable["model"].GetInt();
            new_renderable.mass = renderable["mass"].GetFloat();
            new_renderable.translation = glm::vec3{
                renderable["transform"]["translation"].GetArray()[0].GetFloat(),
                renderable["transform"]["translation"].GetArray()[1].GetFloat(),
                renderable["transform"]["translation"].GetArray()[2].GetFloat(),
            };
            new_renderable.orientation = glm::quat{
                renderable["transform"]["orientation"].GetArray()[0].GetFloat(),
                renderable["transform"]["orientation"].GetArray()[1].GetFloat(),
                renderable["transform"]["orientation"].GetArray()[2].GetFloat(),
                renderable["transform"]["orientation"].GetArray()[3].GetFloat(),
            };
            new_renderable.scale = glm::vec3{
                renderable["transform"]["scale"].GetArray()[0].GetFloat(),
                renderable["transform"]["scale"].GetArray()[1].GetFloat(),
                renderable["transform"]["scale"].GetArray()[2].GetFloat(),
            };

            if (std::string(renderable["lighting"].GetString()).compare("blinn_phong") == 0)
                new_renderable.lighting = boa::gfx::LightingInteractivity::BlinnPhong;
            else if (std::string(renderable["lighting"].GetString()).compare("unlit") == 0)
                new_renderable.lighting = boa::gfx::LightingInteractivity::Unlit;
            else
                LOG_WARN("(Save State) Unrecognized lighting type in JSON scene");

            if (renderable.HasMember("engine_configurable"))
                new_renderable.engine_configurable = renderable["engine_configurable"].GetBool();

            assert(new_renderable.model < m_models.size());

            m_renderables.push_back(std::move(new_renderable));
        }
    }

    if (document.HasMember("global_lights")) {
        const rapidjson::Value &global_lights = document["global_lights"];
        assert(global_lights.IsArray());
        m_global_lights.reserve(global_lights.Size());

        for (auto &global_light : global_lights.GetArray()) {
            assert(global_light["direction"].IsArray() && global_light["direction"].Size() == 3);
            assert(global_light["ambient"].IsArray() && global_light["ambient"].Size() == 3);
            assert(global_light["diffuse"].IsArray() && global_light["diffuse"].Size() == 3);
            assert(global_light["specular"].IsArray() && global_light["specular"].Size() == 3);

            const auto &direction = global_light["direction"].GetArray();
            const auto &ambient = global_light["ambient"].GetArray();
            const auto &diffuse = global_light["diffuse"].GetArray();
            const auto &specular = global_light["specular"].GetArray();

            m_global_lights.emplace_back(
                glm::vec3(direction[0].GetFloat(), direction[1].GetFloat(), direction[2].GetFloat()),
                glm::vec3(ambient[0].GetFloat(), ambient[1].GetFloat(), ambient[2].GetFloat()),
                glm::vec3(diffuse[0].GetFloat(), diffuse[1].GetFloat(), diffuse[2].GetFloat()),
                glm::vec3(specular[0].GetFloat(), specular[1].GetFloat(), specular[2].GetFloat()));
        }
    }

    if (document.HasMember("point_lights")) {
        const rapidjson::Value &point_lights = document["point_lights"];
        assert(point_lights.IsArray());
        m_point_lights.reserve(point_lights.Size());

        for (auto &point_light : point_lights.GetArray()) {
            assert(point_light["position"].IsArray() && point_light["position"].Size() == 3);
            assert(point_light["ambient"].IsArray() && point_light["ambient"].Size() == 3);
            assert(point_light["diffuse"].IsArray() && point_light["diffuse"].Size() == 3);
            assert(point_light["specular"].IsArray() && point_light["specular"].Size() == 3);
            assert(point_light["constant"].IsFloat() && point_light["linear"].IsFloat() && point_light["quadratic"].IsFloat());

            const auto &position = point_light["position"].GetArray();
            const auto &ambient = point_light["ambient"].GetArray();
            const auto &diffuse = point_light["diffuse"].GetArray();
            const auto &specular = point_light["specular"].GetArray();

            m_point_lights.emplace_back(
                glm::vec3(position[0].GetFloat(), position[1].GetFloat(), position[2].GetFloat()),
                glm::vec3(ambient[0].GetFloat(), ambient[1].GetFloat(), ambient[2].GetFloat()),
                glm::vec3(diffuse[0].GetFloat(), diffuse[1].GetFloat(), diffuse[2].GetFloat()),
                glm::vec3(specular[0].GetFloat(), specular[1].GetFloat(), specular[2].GetFloat()),
                point_light["constant"].GetFloat(),
                point_light["linear"].GetFloat(),
                point_light["quadratic"].GetFloat());
        }
    }

    if (document.HasMember("skyboxes")) {
        const rapidjson::Value &skyboxes = document["skyboxes"];
        assert(skyboxes.IsArray());
        m_skyboxes.reserve(skyboxes.Size());

        for (auto &skybox : skyboxes.GetArray()) {
            assert(skybox["paths"].IsArray() && skybox["paths"].Size() == 6);
            const auto &paths = skybox["paths"].GetArray();
            m_skyboxes.emplace_back(std::array<std::string, 6>{
                paths[0].GetString(),
                paths[1].GetString(),
                paths[2].GetString(),
                paths[3].GetString(),
                paths[4].GetString(),
                paths[5].GetString(),
            });
        }

        if (document.HasMember("default_skybox")) {
            assert(document["default_skybox"].IsInt());
            m_default_skybox = document["default_skybox"].GetInt();
        }
    }
}

void EngineState::add_entities(boa::gfx::AssetManager &asset_manager,
                             boa::gfx::AnimationController &animation_controller,
                             boa::phy::PhysicsController &physics_controller) {
    auto &entity_group = boa::ecs::EntityGroup::get();

    uint32_t count = 0;
    for (const auto &skybox : m_skyboxes) {
        uint32_t new_entity = entity_group.new_entity();
        asset_manager.load_skybox_into_entity(new_entity, skybox.texture_paths);
        if (count++ == m_default_skybox)
            asset_manager.set_active_skybox(new_entity);
    }

    for (auto &renderable : m_renderables) {
        uint32_t new_entity = entity_group.new_entity();
        asset_manager.load_model_into_entity(new_entity, m_models[renderable.model], renderable.lighting);
        entity_group.enable_and_make<boa::gfx::Transformable>(new_entity,
                                                              std::move(renderable.orientation),
                                                              std::move(renderable.translation),
                                                              std::move(renderable.scale));

        physics_controller.add_entity(new_entity, renderable.mass);
        animation_controller.load_animations(new_entity, m_models[renderable.model]);

        if (renderable.engine_configurable)
            entity_group.enable_and_make<EngineConfigurable>(new_entity, false);
    }

    for (auto &global_light : m_global_lights) {
        uint32_t new_entity = entity_group.new_entity();
        entity_group.enable_and_make<boa::gfx::GlobalLight>(new_entity,
            std::move(global_light.direction),
            std::move(global_light.ambient),
            std::move(global_light.diffuse),
            std::move(global_light.specular));
    }

    for (auto &point_light : m_point_lights) {
        uint32_t new_entity = entity_group.new_entity();
        entity_group.enable_and_make<boa::gfx::PointLight>(new_entity,
            std::move(point_light.position),
            std::move(point_light.ambient),
            std::move(point_light.diffuse),
            std::move(point_light.specular),
            point_light.constant,
            point_light.linear,
            point_light.quadratic);
    }
}

}
