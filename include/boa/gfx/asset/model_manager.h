#ifndef BOA_GFX_ASSET_MODEL_MANAGER_H
#define BOA_GFX_ASSET_MODEL_MANAGER_H

#include "boa/macros.h"
#include "boa/gfx/asset/vkasset.h"
#include <string>

namespace boa::gfx {

class Renderer;

class ModelManager {
    REMOVE_COPY_AND_ASSIGN(ModelManager);
public:
    ModelManager() {}

    // TODO: take vulkan context-specific handles out of Renderer class so we can instead pass
    // a `VulkanContext` class/struct here instead of the entire Renderer
    uint32_t load_model(Renderer &renderer, const glTFModel &model, const std::string &name);

    VkModel &get_model(size_t index) { return m_models.at(index); }
    VkMaterial &get_material(size_t index) { return m_materials.at(index); }

private:
    uint32_t create_material(vk::Pipeline pipeline, vk::PipelineLayout layout, const std::string &name);

    std::vector<VkMaterial> m_materials;
    std::vector<VkModel> m_models;

    friend class Renderer;
    friend class VkModel;
    friend class VkTexture;
};

}

#endif