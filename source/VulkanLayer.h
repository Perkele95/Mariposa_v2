#pragma once

#include "core.h"

#ifdef _WIN32
#include "..\Vulkan\Include\vulkan\vulkan_win32.h"
#endif

void mpVulkanInit(mpRenderer *pRenderer, mpMemory *memory, mpWindowData *windowData, const mpRenderData *renderData, const mpCallbacks *const callbacks);
void mpVulkanUpdate(mpRenderer *pRenderer, const mpRenderData *renderData, const mpCamera *const camera, const mpWindowData *const windowData);
void mpVulkanCleanup(mpRenderer *pRenderer, uint32_t batchCount);

const uint32_t MP_MAX_IMAGES_IN_FLIGHT = 2;

struct QueueFamilyIndices {
    uint32_t graphicsFamily;
    bool32 hasGraphicsFamily;
    uint32_t presentFamily;
    bool32 hasPresentFamily;
    bool32 isComplete;
};

struct SwapChainSupportDetails
{
    VkSurfaceCapabilitiesKHR capabilities;
    VkSurfaceFormatKHR *pFormats;
    VkPresentModeKHR *pPresentModes;
    uint32_t formatCount;
    uint32_t presentModeCount;
};

struct UniformbufferObject
{
    alignas(16) mat4x4 Model;
    alignas(16) mat4x4 View;
    alignas(16) mat4x4 Proj;
};

struct mpVkRenderer
{
    VkInstance instance;
    VkPhysicalDevice gpu;
    VkDevice device;
    VkSurfaceKHR surface;
    
    VkQueue graphicsQueue;
    VkQueue presentQueue;
    QueueFamilyIndices indices;
    
    VkSwapchainKHR swapChain;
    VkImage *pSwapChainImages;
    VkImageView *pSwapChainImageViews;
    VkFormat swapChainImageFormat;
    VkExtent2D swapChainExtent;
    uint32_t swapChainImageCount;
    SwapChainSupportDetails swapDetails;
    
    VkRenderPass renderPass;
    VkDescriptorPool descriptorPool;
    VkDescriptorSet *pDescriptorSets;
    VkDescriptorSetLayout descriptorSetLayout;
    VkPipelineLayout pipelineLayout;
    VkPipeline graphicsPipeline;
    
    VkShaderModule vertShaderModule;
    VkShaderModule fragShaderModule;
    
    VkFramebuffer *pFramebuffers;
    bool32 *pFramebufferResized;
    uint32_t currentFrame;
    
    VkCommandPool commandPool;
    VkCommandBuffer *pCommandbuffers;
    
    VkBuffer *vertexbuffers;
    VkDeviceMemory *vertexbufferMemories;
    VkBuffer *indexbuffers;
    VkDeviceMemory *indexbufferMemories;
    VkBuffer *pUniformbuffers;
    VkDeviceMemory *pUniformbufferMemories;
    
    VkImage depthImage;
    VkFormat depthFormat;
    VkDeviceMemory depthImageMemory;
    VkImageView depthImageView;
    
    VkSemaphore imageAvailableSemaphores[MP_MAX_IMAGES_IN_FLIGHT];
    VkSemaphore renderFinishedSemaphores[MP_MAX_IMAGES_IN_FLIGHT];
    VkFence inFlightFences[MP_MAX_IMAGES_IN_FLIGHT];
    VkFence *pInFlightImageFences;
    
    UniformbufferObject ubo;
    
    mpFile vertexShader;
    mpFile fragmentShader;
};