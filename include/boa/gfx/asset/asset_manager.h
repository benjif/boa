#ifndef BOA_GFX_ASSET_ASSET_MANAGER_H
#define BOA_GFX_ASSET_ASSET_MANAGER_H

#include "boa/macros.h"
#include "boa/gfx/asset/asset.h"
#include "boa/gfx/asset/lighting_type.h"
#include <string>
#include <array>

namespace boa::gfx {

class Renderer;

class AssetManager {
    REMOVE_COPY_AND_ASSIGN(AssetManager);
public:
    AssetManager(Renderer &renderer)
        : m_renderer(renderer)
    {
    }

    void load_model(uint32_t e_id, const glTFModel &model, const std::string &name,
        LightingInteractivity preferred_lighting = LightingInteractivity::Unlit);

    // loads in order of: X+, X-, Y+, Y-, Z+, Z-
    // (right, left, top, bottom, front, back)
    void load_skybox(uint32_t e_id, const std::array<std::string, 6> &texture_paths);

    uint32_t create_material(vk::Pipeline pipeline, vk::PipelineLayout layout, const std::string &name);
    Material &get_material(size_t index) { return m_materials.at(index); }

    std::optional<uint32_t> get_active_skybox() { return m_active_skybox; }
    void set_active_skybox(uint32_t id) { m_active_skybox = id; }
    void reset_active_skybox() { m_active_skybox.reset(); }

private:
    Renderer &m_renderer;

    std::vector<Material> m_materials;
    std::optional<uint32_t> m_active_skybox;
};

}

#endif