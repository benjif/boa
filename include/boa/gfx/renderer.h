#ifndef BOA_GFX_RENDERER_H
#define BOA_GFX_RENDERER_H

#include "vk_mem_alloc.h"
#include "boa/deletion_queue.h"
#include "boa/gfx/window.h"
#include "boa/gfx/vk/util.h"
#include "boa/gfx/vk/types.h"
#include "boa/gfx/asset/mesh.h"
#include "boa/gfx/asset/texture.h"
#include "boa/gfx/camera.h"
#include "boa/input/keyboard.h"
#include "boa/input/mouse.h"
#include <glm/gtx/transform.hpp>
#include <glm/glm.hpp>
#include <optional>
#include <string>
#include <functional>

namespace boa::gfx {

struct Material {
    vk::DescriptorSet texture_set{ VK_NULL_HANDLE };
    vk::Pipeline pipeline;
    vk::PipelineLayout pipeline_layout;
};

struct Model {
    // TODO: ensure mesh and material are not null
    Mesh *mesh{ nullptr };
    Material *material{ nullptr };
    glm::mat4 transform_matrix;
};

class Renderer {
public:
    static constexpr uint32_t INIT_WIDTH = 1280;
    static constexpr uint32_t INIT_HEIGHT = 960;
    static constexpr const char *WINDOW_TITLE = "Vulkan";

    struct WindowUserPointers {
        Renderer *renderer;
        input::Keyboard *keyboard;
        input::Mouse *mouse;
    };

    Renderer();
    void run();

private:
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
    };

    struct PushConstants {
        glm::vec4 extra;
        glm::mat4 model_view_projection;
    };

    struct PerFrame {
        vk::Semaphore present_sem, render_sem;
        vk::Fence render_fence;
        vk::CommandPool command_pool;
        vk::CommandBuffer command_buffer;
        vk::DescriptorSet parent_set;
        VmaBuffer transformations;

        DeletionQueue deletion_queue;
    };

    struct UploadContext {
        vk::Fence upload_fence;
        vk::CommandPool command_pool;
    };

    Window m_window{ INIT_WIDTH, INIT_HEIGHT, WINDOW_TITLE };
    WindowUserPointers m_user_pointers;
    input::Keyboard m_keyboard;
    input::Mouse m_mouse;
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
    vk::DescriptorSetLayout m_texture_descriptor_set_layout;
    vk::DescriptorPool m_descriptor_pool;

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

    std::vector<Model> m_models;
    std::unordered_map<std::string, Material> m_materials;
    std::unordered_map<std::string, Mesh> m_meshes;
    std::unordered_map<std::string, Texture> m_textures;

    static void framebuffer_size_callback(void *user_ptr_v, int w, int h);

    Material *create_material(vk::Pipeline pipeline, vk::PipelineLayout layout, const std::string &name);
    Material *get_material(const std::string &name);
    Mesh *get_mesh(const std::string &name);

    PerFrame &current_frame();

    void input_update(float time_change);
    void draw_frame();
    void draw_objects(vk::CommandBuffer cmd, Model *first, size_t count);

    void init_window_user_pointers();
    void init_window();
    void init_input();
    void init_imgui();

    void cleanup();

    void create_instance();
    void select_physical_device();
    void create_logical_device();
    void create_allocator();
    void create_surface();
    void create_swapchain();
    void create_commands();
    void create_default_renderpass();
    void create_framebuffer();
    void create_sync_objects();
    void create_pipelines();
    void create_descriptors();
    void create_scene();

    void load_meshes();
    void load_textures();
    void upload_mesh_vertices(Mesh &mesh);
    void upload_mesh_indices(Mesh &mesh);
    void upload_mesh(Mesh &mesh);

    void immediate_command(std::function<void(vk::CommandBuffer cmd)> &&function);

    vk::ShaderModule load_shader(const char *path);
    vk::ImageView create_image_view(vk::Image image, vk::Format format, vk::ImageAspectFlags aspect_flags, uint32_t mip_levels = 1);
    VmaBuffer create_buffer(size_t size, vk::BufferUsageFlags usage, VmaMemoryUsage memory_usage);

    SwapChainSupportDetails query_swap_chain_support(vk::PhysicalDevice device);
    vk::SampleCountFlagBits get_max_sample_count();
    bool check_device(vk::PhysicalDevice device);
    bool check_device_extension_support(vk::PhysicalDevice device);
    QueueFamilyIndices find_queue_families(vk::PhysicalDevice device);

    VkDebugUtilsMessengerEXT m_debug_messenger;
    void setup_debug_messenger();
    static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(
        VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
        VkDebugUtilsMessageTypeFlagsEXT message_type,
        const VkDebugUtilsMessengerCallbackDataEXT *p_callback_data,
        void *p_user_data);

    friend class Texture;
};

}

#endif