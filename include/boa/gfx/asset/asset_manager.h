#ifndef BOA_GFX_ASSET_ASSET_MANAGER_H
#define BOA_GFX_ASSET_ASSET_MANAGER_H

#include "boa/utl/iteration.h"
#include "boa/utl/macros.h"
#include "boa/utl/deletion_queue.h"
#include "boa/gfx/asset/asset.h"
#include "boa/gfx/lighting_type.h"
#include <string>
#include <array>

namespace boa::gfx {

class Renderer;

struct BaseRenderable {
    BaseRenderable(uint32_t id)
        : model_id(id)
    {
    }
    uint32_t model_id;
};

struct Renderable {
    Renderable(uint32_t id)
        : model_id(id)
    {
    }
    uint32_t model_id;
};

class AssetManager {
    REMOVE_COPY_AND_ASSIGN(AssetManager);
public:
    /*struct ModelMetaData {
        ModelMetaData(std::string &&fp)
            : file_path(fp)
        {
        }

        std::string file_path;
    };*/

    AssetManager(Renderer &renderer);

    // loads in order of: X+, X-, Y+, Y-, Z+, Z-
    // (right, left, top, bottom, front, back)
    void load_skybox_into_entity(uint32_t e_id, const std::array<std::string, 6> &texture_paths);

    uint32_t load_model(const glTFModel &model, LightingInteractivity preferred_lighting = LightingInteractivity::Unlit);
    void load_model_into_entity(uint32_t e_id, const glTFModel &model, LightingInteractivity preferred_lighting = LightingInteractivity::Unlit);

    uint32_t create_material(vk::Pipeline pipeline, vk::PipelineLayout layout);

    GPUMaterial &get_material(size_t index) { return m_materials.at(index); }
    const GPUModel &get_model(uint32_t id) const { return m_models[id]; }

    //const ModelMetaData &get_model_meta_data(uint32_t id) const { return m_models_meta_data[id]; }

    /*template <typename C>
    void for_each_model_meta_data(C callback) {
        for (const auto &model : m_models_meta_data) {
            if (callback(model) == Iteration::Break)
                break;
        }
    }*/

    std::optional<uint32_t> get_active_skybox() const { return m_active_skybox; }
    void set_active_skybox(uint32_t id) { m_active_skybox = id; }
    void reset_active_skybox() { m_active_skybox.reset(); }

    void reset();

private:
    Renderer &m_renderer;
    DeletionQueue m_deletion_queue;

    std::unordered_map<std::string, uint32_t> m_model_path_to_model_index;

    std::vector<GPUModel> m_models;
    std::vector<GPUMaterial> m_materials;
    std::optional<uint32_t> m_active_skybox;

    friend class GPUModel;
    friend class GPUTexture;
    friend class GPUSkybox;
};

}

#endif
