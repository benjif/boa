#ifndef BOA_GFX_ASSET_ASSET_MANAGER_H
#define BOA_GFX_ASSET_ASSET_MANAGER_H

#include "boa/utl/macros.h"
#include "boa/gfx/asset/asset.h"
#include "boa/gfx/lighting_type.h"
#include <string>
#include <array>

namespace boa::gfx {

class Renderer;

struct Renderable {
    uint32_t model_id;
};

class AssetManager {
    REMOVE_COPY_AND_ASSIGN(AssetManager);
public:
    AssetManager(Renderer &renderer)
        : m_renderer(renderer)
    {
    }

    void load_model(uint32_t e_id, const glTFModel &model, LightingInteractivity preferred_lighting = LightingInteractivity::Unlit);
    const Model &get_model(uint32_t id) const { return m_models[id]; }

    // loads in order of: X+, X-, Y+, Y-, Z+, Z-
    // (right, left, top, bottom, front, back)
    void load_skybox(uint32_t e_id, const std::array<std::string, 6> &texture_paths);

    uint32_t create_material(vk::Pipeline pipeline, vk::PipelineLayout layout);
    Material &get_material(size_t index) { return m_materials.at(index); }

    std::optional<uint32_t> get_active_skybox() const { return m_active_skybox; }
    void set_active_skybox(uint32_t id) { m_active_skybox = id; }
    void reset_active_skybox() { m_active_skybox.reset(); }

private:
    Renderer &m_renderer;

    std::vector<Material> m_materials;
    std::vector<Model> m_models;
    std::optional<uint32_t> m_active_skybox;
};

}

#endif
