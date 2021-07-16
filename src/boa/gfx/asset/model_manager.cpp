#include "boa/gfx/asset/model_manager.h"

namespace boa::gfx {

uint32_t ModelManager::load_model(Renderer &renderer, const glTFModel &model, const std::string &name) {
    VkModel new_model(*this, renderer, name, model);
    m_models.push_back(std::move(new_model));

    return m_models.size() - 1;
}

uint32_t ModelManager::create_material(vk::Pipeline pipeline, vk::PipelineLayout layout, const std::string &name) {
    VkMaterial material;
    material.pipeline = pipeline;
    material.pipeline_layout = layout;
    material.name = name;
    m_materials.push_back(std::move(material));
    return m_materials.size() - 1;
}

}