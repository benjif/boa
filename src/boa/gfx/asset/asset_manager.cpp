#include "boa/utl/macros.h"
#include "boa/ecs/ecs.h"
#include "boa/ngn/object.h"
#include "boa/gfx/linear.h"
#include "boa/gfx/asset/asset_manager.h"
#include "boa/gfx/renderer.h"
#include <array>

namespace boa::gfx {

AssetManager::AssetManager(Renderer &renderer)
    : m_renderer(renderer)
{
}

uint32_t AssetManager::load_model(const glTFModel &model, LightingInteractivity preferred_lighting) {
    std::string file_path = model.get_file_path();
    LOG_INFO("(Asset) Loading model at '{}'", file_path);

    auto &entity_group = ecs::EntityGroup::get();
    uint32_t new_entity = entity_group.new_entity();

    size_t model_index = 0;
    if (m_model_path_to_model_index.count(file_path) == 0) {
        model_index = m_models.size();
        m_models.emplace_back(*this, m_renderer, model, preferred_lighting);
        m_model_path_to_model_index[file_path] = model_index;
        entity_group.enable_and_make<BaseRenderable>(new_entity, model_index);
        entity_group.enable_and_make<boa::ngn::LoadedAsset>(new_entity, std::move(file_path));
    } else {
        model_index = m_model_path_to_model_index.at(file_path);
    }

    return new_entity;
}

void AssetManager::load_model_into_entity(uint32_t e_id, const glTFModel &model, LightingInteractivity preferred_lighting) {
    std::string file_path = model.get_file_path();
    LOG_INFO("(Asset) Loading model at '{}'", file_path);

    auto &entity_group = ecs::EntityGroup::get();

    size_t model_index = 0;
    if (m_model_path_to_model_index.count(file_path) == 0) {
        model_index = m_models.size();
        m_models.emplace_back(*this, m_renderer, model, preferred_lighting);
        m_model_path_to_model_index[file_path] = model_index;
        entity_group.enable_and_make<BaseRenderable>(e_id, model_index);
    } else {
        model_index = m_model_path_to_model_index.at(file_path);
    }

    entity_group.enable_and_make<Renderable>(e_id, model_index);
    entity_group.enable_and_make<boa::ngn::LoadedAsset>(e_id, std::move(file_path));
}

void AssetManager::load_skybox_into_entity(uint32_t e_id, const std::array<std::string, 6> &texture_paths) {
    LOG_INFO("(Asset) Loading skybox into entity {}", e_id);

    auto &entity_group = ecs::EntityGroup::get();
    entity_group.enable_and_make<GPUSkybox>(e_id, *this, m_renderer, std::forward<const std::array<std::string, 6> &>(texture_paths));

    std::stringstream resource_paths_s;
    for (int i = 0; i < 6; i++) {
        resource_paths_s << texture_paths[i];
        if (i != 5)
            resource_paths_s << ';';
    }

    entity_group.enable_and_make<boa::ngn::LoadedAsset>(e_id, resource_paths_s.str());
}

uint32_t AssetManager::create_material(vk::Pipeline pipeline, vk::PipelineLayout layout) {
    GPUMaterial material;
    material.pipeline = pipeline;
    material.pipeline_layout = layout;
    m_materials.push_back(std::move(material));
    return m_materials.size() - 1;
}

/*std::string AssetManager::get_entity_resource_paths(uint32_t e_id) const {
    return m_entity_resource_paths.at(e_id);
}*/

void AssetManager::reset() {
    m_renderer.wait_for_all_frames();
    m_deletion_queue.flush();
    m_materials.erase(m_materials.begin() + m_renderer.NUMBER_OF_DEFAULT_MATERIALS, m_materials.end());
    m_models.clear();
    //m_models_meta_data.clear();
    //m_entity_resource_paths.clear();
    m_model_path_to_model_index.clear();
    m_active_skybox.reset();
}

}
