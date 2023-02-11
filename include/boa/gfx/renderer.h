#ifndef BOA_GFX_RENDERER_H
#define BOA_GFX_RENDERER_H

#include "boa/utl/deletion_queue.h"
#include "boa/utl/macros.h"
#include "boa/ctl/keyboard.h"
#include "boa/ctl/mouse.h"
#include "boa/gfx/window.h"
#include "boa/gfx/vk/util.h"
#include "boa/gfx/vk/types.h"
#include "boa/gfx/lighting.h"
#include "boa/gfx/asset/gltf_model.h"
#include "boa/gfx/asset/asset.h"
#include "boa/gfx/camera.h"
#include "boa/gfx/asset/asset_manager.h"
#include "glm/gtx/transform.hpp"
#include <functional>
#include <optional>
#include <utility>
#include <string>

namespace boa::gfx {

class DebugDrawer;

class Renderer {
    REMOVE_COPY_AND_ASSIGN(Renderer);
public:
    static constexpr uint32_t INIT_WIDTH = 1280;
    static constexpr uint32_t INIT_HEIGHT = 960;
    static constexpr const char *WINDOW_TITLE = "Boa Engine";

    struct WindowUserPointers {
        Renderer *renderer;
        ctl::Keyboard *keyboard;
        ctl::Mouse *mouse;
    };

    Renderer();
    ~Renderer();

    void draw_frame();
    void wait_for_all_frames() const;
    void wait_idle() const;

    void add_debug_drawer(DebugDrawer *debug_drawer);

    AssetManager &get_asset_manager() {
        return m_asset_manager;
    }

    VmaAllocator get_allocator() {
        return m_allocator;
    }

    vk::Device get_device() {
        return m_device.get();
    }

    ctl::Keyboard &get_keyboard()   { return m_keyboard;    }
    ctl::Mouse &get_mouse()         { return m_mouse;       }
    Camera &get_camera()            { return m_camera;      }
    Window &get_window()            { return m_window;      }

    uint32_t get_width() const {
        return m_window_extent.width;
    }
    uint32_t get_height() const {
        return m_window_extent.height;
    }

    uint32_t get_frame_count() const {
        return m_frame;
    }

    glm::mat4 &get_view() {
        return m_transforms.view;
    }
    glm::mat4 &get_projection() {
        return m_transforms.projection;
    }
    glm::mat4 &get_view_projection() {
        return m_transforms.view_projection;
    }

    void set_draw_bounding_boxes(bool draw) {
        m_draw_bounding_boxes = draw;
    }
    bool get_draw_bounding_boxes() const {
        return m_draw_bounding_boxes;
    }

    void set_ui_mouse_enabled(bool mouse_enabled);

    enum {
        UNTEXTURED_MATERIAL_INDEX,
        TEXTURED_MATERIAL_INDEX,
        BOUNDING_BOX_MATERIAL_INDEX,
        UNTEXTURED_BLINN_PHONG_MATERIAL_INDEX,
        TEXTURED_BLINN_PHONG_MATERIAL_INDEX,

        NUMBER_OF_DEFAULT_MATERIALS
    };

private:
    enum {
        SWAPCHAIN_DELETE_TAG = (1 << 1),
        FRAMEBUFF_DELETE_TAG = (2 << 1),
    };

    const std::vector<const char *> validation_layers = {
        "VK_LAYER_KHRONOS_validation"
    };

    const std::vector<const char *> device_extensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        VK_KHR_IMAGELESS_FRAMEBUFFER_EXTENSION_NAME,
    };

#ifdef NDEBUG
    const bool validation_enabled = false;
#else
    const bool validation_enabled = true;
#endif

    constexpr static uint32_t FRAMES_IN_FLIGHT = 2;
    constexpr static uint32_t MAX_IMAGE_DESCRIPTORS = 125;

    struct QueueFamilyIndices {
        std::optional<uint32_t> graphics_family;
        std::optional<uint32_t> present_family;

        bool is_complete() {
            return graphics_family.has_value() && present_family.has_value();
        }
    };

    struct SwapChainSupportDetails {
        vk::SurfaceCapabilitiesKHR capabilities;
        std::vector<vk::SurfaceFormatKHR> formats;
        std::vector<vk::PresentModeKHR> present_modes;
    };

    struct Transformations {
        glm::mat4 view;
        glm::mat4 projection;
        glm::mat4 view_projection;
        // TODO: move skybox_view_projection into a different uniform/push constant
        //       struct, I'm too tired right now
        glm::mat4 skybox_view_projection;
    } m_transforms;

    constexpr static uint32_t MAX_POINT_LIGHTS = 16;

    struct BlinnPhong {
        GlobalLight global_light;
        PointLight point_lights[MAX_POINT_LIGHTS];
        uint32_t point_lights_count;
        glm::vec3 camera_position;
    };

    struct PushConstants {
        glm::ivec4 extra0;
        glm::vec4 extra1;
        glm::mat4 model_view_projection;
    };

    struct PerFrame {
        vk::Semaphore present_sem, render_sem;
        vk::Fence render_fence;
        vk::CommandPool command_pool;
        vk::CommandBuffer command_buffer;
        vk::DescriptorSet parent_set;
        vk::DescriptorSet parent_blinn_phong_set;
        VmaBuffer transformations_buffer;
        VmaBuffer blinn_phong_buffer;

        DeletionQueue deletion_queue;
    };

    struct UploadContext {
        vk::Fence upload_fence;
        vk::CommandPool command_pool;
    };

    std::vector<DebugDrawer *> m_debug_drawers;

    Window m_window{ INIT_WIDTH, INIT_HEIGHT, WINDOW_TITLE };
    WindowUserPointers m_user_pointers;
    bool m_framebuffer_resize{ false };

    // TODO: move input and camera stuff outside of renderer class
    ctl::Keyboard m_keyboard;
    ctl::Mouse m_mouse;
    Camera m_camera;

    DeletionQueue m_deletion_queue;
    UploadContext m_upload_context;

    uint32_t m_frame{ 0 };

    VmaAllocator m_allocator;

    vk::Extent2D m_window_extent{ INIT_WIDTH, INIT_HEIGHT };
    vk::UniqueInstance m_instance;
    vk::PhysicalDevice m_physical_device;
    vk::UniqueDevice m_device;
    vk::PhysicalDeviceProperties m_device_properties;
    vk::FormatProperties m_device_format_properties;
    vk::UniqueSurfaceKHR m_surface;
    vk::Queue m_graphics_queue;
    vk::Queue m_present_queue;
    vk::DescriptorSetLayout m_descriptor_set_layout;
    vk::DescriptorSetLayout m_textures_set_layout;
    vk::DescriptorSetLayout m_blinn_phong_set_layout;
    vk::DescriptorSetLayout m_skybox_set_layout;
    vk::DescriptorPool m_descriptor_pool;

    VmaBuffer m_skybox_index_buffer;
    VmaBuffer m_skybox_vertex_buffer;
    vk::Pipeline m_skybox_pipeline;
    vk::PipelineLayout m_skybox_pipeline_layout;

    vk::SampleCountFlagBits m_msaa_samples{ vk::SampleCountFlagBits::e1 };

    vk::SwapchainKHR m_swapchain;
    vk::Format m_swapchain_format;
    std::vector<vk::Image> m_swapchain_images;
    std::vector<vk::ImageView> m_swapchain_image_views;

    vk::RenderPass m_renderpass;
    vk::Framebuffer m_framebuffer;

    PerFrame m_frames[FRAMES_IN_FLIGHT];

    VmaImage m_depth_image;
    vk::ImageView m_depth_image_view;
    vk::Format m_depth_format;

    VmaImage m_msaa_image;
    vk::ImageView m_msaa_image_view;

    Frustum m_frustum;
    AssetManager m_asset_manager;
    bool m_draw_bounding_boxes{ false };

    static void framebuffer_size_callback(void *user_ptr_v, int w, int h);
    void wait_if_minimized();

    PerFrame &current_frame();

    void draw_renderables(vk::CommandBuffer cmd);

    void init_window_user_pointers();
    void init_window();
    void init_imgui();

    void cleanup();

    void create_instance();
    void select_physical_device();
    void create_logical_device();
    void create_allocator();
    void create_surface();
    void create_swapchain();
    void recreate_swapchain();
    void create_commands();
    void create_default_renderpass();
    void create_framebuffer();
    void create_sync_objects();
    void create_pipelines();
    void create_descriptors();
    void create_skybox_resources();

    void immediate_command(std::function<void(vk::CommandBuffer cmd)> &&function);

    vk::ShaderModule load_shader(const char *path);
    vk::ImageView create_image_view(vk::Image image, vk::Format format,
        vk::ImageAspectFlags aspect_flags, uint32_t mip_levels = 1) const;
    VmaBuffer create_buffer(size_t size, vk::BufferUsageFlags usage, VmaMemoryUsage memory_usage) const;

    SwapChainSupportDetails query_swap_chain_support(vk::PhysicalDevice device) const;
    vk::SampleCountFlagBits get_max_sample_count() const;
    bool check_device(vk::PhysicalDevice device) const;
    bool check_device_extension_support(vk::PhysicalDevice device) const;
    QueueFamilyIndices find_queue_families(vk::PhysicalDevice device) const;

    VkDebugUtilsMessengerEXT m_debug_messenger;
    void setup_debug_messenger();
    static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(
        VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
        VkDebugUtilsMessageTypeFlagsEXT message_type,
        const VkDebugUtilsMessengerCallbackDataEXT *p_callback_data,
        void *p_user_data);

    friend class GPUModel;
    friend class GPUTexture;
    friend class GPUSkybox;
    friend class DebugDrawer;
};

}

#endif
