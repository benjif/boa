#include "boa/macros.h"
#include "boa/ecs/ecs.h"
#include "boa/gfx/linear.h"
#include "boa/gfx/asset/asset_manager.h"
#include <array>

namespace boa::gfx {

void AssetManager::load_model(uint32_t e_id, const glTFModel &model, const std::string &name, LightingInteractivity preferred_lighting) {
    LOG_INFO("(Asset) Loading model '{}' into entity {}", name, e_id);

    auto &entity_group = ecs::EntityGroup::get();
    entity_group.enable_and_make<RenderableModel>(e_id, *this, m_renderer, name, model, preferred_lighting);
}

void AssetManager::load_skybox(uint32_t e_id, const std::array<std::string, 6> &texture_paths) {
    LOG_INFO("(Asset) Loading skybox into entity {}", e_id);

    auto &entity_group = ecs::EntityGroup::get();
    entity_group.enable_and_make<Skybox>(e_id, m_renderer, std::forward<const std::array<std::string, 6> &>(texture_paths));
}

uint32_t AssetManager::create_material(vk::Pipeline pipeline, vk::PipelineLayout layout, const std::string &name) {
    Material material;
    material.pipeline = pipeline;
    material.pipeline_layout = layout;
    material.name = name;
    m_materials.push_back(std::move(material));
    return m_materials.size() - 1;
}

}