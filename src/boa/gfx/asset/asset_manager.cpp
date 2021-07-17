#include "boa/macros.h"
#include "boa/gfx/linear_types.h"
#include "boa/gfx/asset/asset_manager.h"
#include <array>

namespace boa::gfx {

uint32_t AssetManager::load_model(const glTFModel &model, const std::string &name) {
    LOG_INFO("(Asset) Loading model '{}'", name);

    VkModel new_model(*this, m_renderer, name, model);
    m_models.push_back(std::move(new_model));

    return m_models.size() - 1;
}

uint32_t AssetManager::load_skybox(const std::array<std::string, 6> &texture_paths) {
    LOG_INFO("(Asset) Loading skybox {}", m_skyboxes.size());

    VkSkybox new_skybox(m_renderer, texture_paths);
    m_skyboxes.push_back(std::move(new_skybox));
    return m_skyboxes.size() - 1;
}

uint32_t AssetManager::create_material(vk::Pipeline pipeline, vk::PipelineLayout layout, const std::string &name) {
    VkMaterial material;
    material.pipeline = pipeline;
    material.pipeline_layout = layout;
    material.name = name;
    m_materials.push_back(std::move(material));
    return m_materials.size() - 1;
}

}