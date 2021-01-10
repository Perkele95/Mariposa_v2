#pragma once

#include "..\core.h"
#include "vulkan\vulkan.h"

struct mpUniformbufferObject
{
    alignas(16) mat4x4 model;
    alignas(16) mat4x4 view;
    alignas(16) mat4x4 proj;

	alignas(16) vec3 lightPosition;
	alignas(16) vec3 lightDiffuse;
	alignas(4) float lightConstant;
	alignas(4) float lightLinear;
	alignas(4) float lightQuadratic;
	alignas(4) float lightAmbient;
};

constexpr uint32_t MP_MAX_TEXTURES = 8;
constexpr char *validationLayers[] = {"VK_LAYER_KHRONOS_validation"};
constexpr char *deviceExtensions[] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
constexpr char *requiredExtensions[] = { "VK_KHR_surface", "VK_KHR_win32_surface" };
constexpr uint32_t MAX_IMAGES_IN_FLIGHT = 2;
/*
 *  Order of execution (not strictly enforced, but wrong order does not work):
 *  1 - LinkMemory
 *  2 - InitDevice
 *  3 - LoadShaders
 *  4 - LoadTextures
 *  5 - InitResources
*/
struct mpRenderer
{
    // API
    void LinkMemory(mpMemoryRegion rendererMemory, mpMemoryRegion temporaryMemory);
    void InitDevice(mpCore &core, bool32 enableValidation);
    void LoadTextures(const char *paths[], uint32_t count);
    void InitResources(mpCore &core);
    void RecreateGuiBuffers(mpGUI &gui);
    void RecreateSceneBuffer(mpMesh &mesh, uint32_t x, uint32_t y, uint32_t z);
    void Update(mpCore &core);
    void Cleanup();
private:
    // MP memory
    mpMemoryRegion mpVkMemory;
    mpMemoryRegion tempMemory;
    // Shared fields
    VkInstance instance;
    VkPhysicalDevice gpu;
    VkDevice device;
    VkSurfaceKHR surface;
    VkQueue graphicsQueue;
    VkQueue presentQueue;
    uint32_t graphicsQueueFamily;
    uint32_t presentQueueFamily;
    bool32 separatePresentQueue;

    VkSwapchainKHR swapchain;
    VkFormat swapchainFormat;
    VkExtent2D swapchainExtent;
    VkSurfaceCapabilitiesKHR surfaceCapabilities;
    VkSurfaceFormatKHR surfaceFormat;
    VkPresentModeKHR presentMode;
    VkRenderPass renderPass;
    VkDescriptorPool descriptorPool;
    VkCommandPool commandPool;

    struct
    {
        VkImage image;
        VkFormat format;
        VkDeviceMemory imageMemory;
        VkImageView imageView;
    }depth;

    mpUniformbufferObject ubo;
    // Shared fields :: swapchain dependant resources
    uint32_t swapchainImageCount;
    uint32_t currentFrame;
    VkImage *pSwapchainImages;
    VkImageView *pSwapchainImageViews;
    VkFramebuffer *pFramebuffers;
    VkCommandBuffer *pCommandbuffers;
    VkBuffer *pUniformbuffers;
    VkDeviceMemory *pUniformbufferMemories;
    VkFence *pInFlightImageFences;

    struct
    {
        VkPipeline pipeline;
        VkPipelineLayout pipelineLayout;
        VkDescriptorSet *pDescriptorSets;
        VkDescriptorSetLayout descriptorSetLayout;

        VkShaderModule vertShaderModule;
        VkShaderModule fragShaderModule;
        VkBuffer vertexbuffers[MP_REGION_SIZE][MP_REGION_SIZE][MP_REGION_SIZE];
        VkDeviceMemory vertexbufferMemories[MP_REGION_SIZE][MP_REGION_SIZE][MP_REGION_SIZE];
        VkBuffer indexbuffers[MP_REGION_SIZE][MP_REGION_SIZE][MP_REGION_SIZE];
        VkDeviceMemory indexbufferMemories[MP_REGION_SIZE][MP_REGION_SIZE][MP_REGION_SIZE];
    }scene;

    struct
    {
        VkPipeline pipeline;
        VkPipelineLayout pipelineLayout;
        VkDescriptorSetLayout descriptorSetLayout;
        VkDescriptorSet *pDescriptorSets;

        VkShaderModule vertShaderModule;
        VkShaderModule fragShaderModule;
    }gui;

    struct
    {
        VkBuffer *pVertexbuffers;
        VkDeviceMemory *pVertexbufferMemories;
        VkBuffer *pIndexbuffers;
        VkDeviceMemory *pIndexbufferMemories;
        VkImage *pImages;
        VkImageView *pImageViews;
        VkDeviceMemory *pImageMemories;
        uint32_t count;
        VkSampler sampler;
    }texture;

    // Max images in flight dependant resources
    VkSemaphore imageAvailableSPs[MAX_IMAGES_IN_FLIGHT];
    VkSemaphore renderFinishedSPs[MAX_IMAGES_IN_FLIGHT];
    VkFence inFlightFences[MAX_IMAGES_IN_FLIGHT];

    // Private methods
    void LoadShaderModule(const mpCallbacks &callbacks, VkShaderModule *pModule, const char *path);
    void PrepareTextureImage(VkImage &image, VkDeviceMemory &imageMemory, const char *filePath);
    void TransitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);
    bool32 CheckPhysicalDeviceSuitability(VkPhysicalDevice &checkedPhysDevice);
    VkImageView CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);
    void PrepareRenderPass();
    void LoadShaders(const mpCallbacks &callbacks);
    void PrepareScenePipeline();
    void PrepareGuiPipeline();
    VkImage CreateImage(VkExtent2D extent, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage);
    void BindImageMemory(VkImage &image, VkDeviceMemory &imageMemory, VkMemoryPropertyFlags memProps);
    void PrepareDepthResources();
    void PrepareFramebuffers();
    void CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags propertyFlags, VkBuffer &buffer, VkDeviceMemory &bufferMemory);
    void mapBufferData(void *rawBuffer, VkDeviceSize bufferSize, VkBuffer *buffer, VkDeviceMemory *bufferMemory, VkBufferUsageFlags usage);
    void PrepareScenebuffers(mpVoxelRegion *region);
    void PrepareUniformbuffers();
    void PrepareDescriptorPool();
    void PrepareSceneDescriptorSets();
    void PrepareGuiDescriptorSets();
    void PrepareCommandbuffers();
    void PrepareSyncObjects();
    void CleanUpSwapchain();
    void ReCreateSwapchain(int32_t width, int32_t height);
    void DrawFrame(const mpVoxelRegion &region, const mpGUI &mpgui, uint32_t imageIndex);
};