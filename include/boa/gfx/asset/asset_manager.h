#ifndef BOA_GFX_ASSET_ASSET_MANAGER_H
#define BOA_GFX_ASSET_ASSET_MANAGER_H

#include "boa/macros.h"
#include "boa/gfx/asset/vkasset.h"
#include <string>
#include <array>

namespace boa::gfx {

class Renderer;

class AssetManager {
    REMOVE_COPY_AND_ASSIGN(AssetManager);
public:
    AssetManager() {}

    // TODO: take vulkan context-specific handles out of Renderer class so we can instead pass
    // a `VulkanContext` class/struct here instead of the entire Renderer
    uint32_t load_model(Renderer &renderer, const glTFModel &model, const std::string &name);
    uint32_t load_skybox(Renderer &renderer, const std::array<std::string, 6> &texture_paths);

    VkModel &get_model(size_t index) { return m_models.at(index); }
    VkMaterial &get_material(size_t index) { return m_materials.at(index); }
    VkSkybox &get_skybox(size_t index) { return m_skyboxes.at(index); }

private:
    uint32_t create_material(vk::Pipeline pipeline, vk::PipelineLayout layout, const std::string &name);

    std::vector<VkMaterial> m_materials;
    std::vector<VkModel> m_models;
    std::vector<VkSkybox> m_skyboxes;

    friend class Renderer;
    friend class VkModel;
    friend class VkTexture;
};

}

#endif