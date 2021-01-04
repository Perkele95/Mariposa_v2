#include "VulkanLayer.h"
#include "vulkan\vulkan.h"
#include <stdio.h>

constexpr char* validationLayers[] = {"VK_LAYER_KHRONOS_validation"};
constexpr char* deviceExtensions[] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
constexpr uint32_t MP_MAX_IMAGES_IN_FLIGHT = 2;

struct QueueFamilyIndices {
    struct
    {
        uint32_t value;
        bool32 hasValue;
    }graphicsFamily, presentFamily;

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
// Aligned according to vulkan specs
struct UniformbufferObject
{
    alignas(16) mat4x4 Model;
    alignas(16) mat4x4 View;
    alignas(16) mat4x4 Proj;

	alignas(16) vec3 position;
	alignas(16) vec3 diffuse;
	alignas(4) float constant;
	alignas(4) float linear;
	alignas(4) float quadratic;
	alignas(4) float ambient;
};

struct LightUBO
{

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

    VkPipeline pipeline;
    VkPipelineLayout pipelineLayout;

    VkShaderModule vertShaderModule;
    VkShaderModule fragShaderModule;

    struct
    {
        VkPipeline pipeline;
        VkPipelineLayout pipelineLayout;

        VkShaderModule vertShaderModule;
        VkShaderModule fragShaderModule;

        VkBuffer vertexbuffer;
        VkDeviceMemory vertexbufferMemory;
        VkBuffer indexbuffer;
        VkDeviceMemory indexbufferMemory;
    } gui;

    VkFramebuffer *pFramebuffers;
    bool32 *pFramebufferResized;
    uint32_t currentFrame;

    VkCommandPool commandPool;
    VkCommandBuffer *pCommandbuffers;

    VkBuffer vertexbuffers[MP_REGION_SIZE][MP_REGION_SIZE][MP_REGION_SIZE];
    VkDeviceMemory vertexbufferMemories[MP_REGION_SIZE][MP_REGION_SIZE][MP_REGION_SIZE];
    VkBuffer indexbuffers[MP_REGION_SIZE][MP_REGION_SIZE][MP_REGION_SIZE];
    VkDeviceMemory indexbufferMemories[MP_REGION_SIZE][MP_REGION_SIZE][MP_REGION_SIZE];

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
};

static bool32 IsDeviceSuitable(mpVkRenderer *renderer, VkPhysicalDevice *checkedPhysDevice, mpMemoryRegion tempMemory)
{
    QueueFamilyIndices indices = {};
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(*checkedPhysDevice, &queueFamilyCount, nullptr);
    VkQueueFamilyProperties* queueFamilyProperties = static_cast<VkQueueFamilyProperties*>(mpAllocateIntoRegion(tempMemory, sizeof(VkQueueFamilyProperties) * queueFamilyCount));
    vkGetPhysicalDeviceQueueFamilyProperties(*checkedPhysDevice, &queueFamilyCount, queueFamilyProperties);

    for(uint32_t i = 0; i < queueFamilyCount; i++){
        if(queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT){
            indices.graphicsFamily.value = i;
            indices.graphicsFamily.hasValue = true;
        }

        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(*checkedPhysDevice, i, renderer->surface, &presentSupport);
        if(presentSupport){
            indices.presentFamily.value = i;
            indices.presentFamily.hasValue = true;
        }

        if(indices.graphicsFamily.hasValue && indices.presentFamily.hasValue){
            indices.isComplete = true;
            break;
        }
    }

    uint32_t extensionCount = 0;
    vkEnumerateDeviceExtensionProperties(*checkedPhysDevice, nullptr, &extensionCount, nullptr);
    VkExtensionProperties* availableExtensions = static_cast<VkExtensionProperties*>(mpAllocateIntoRegion(tempMemory, sizeof(VkExtensionProperties) * extensionCount));
    vkEnumerateDeviceExtensionProperties(*checkedPhysDevice, nullptr, &extensionCount, availableExtensions);

    bool32 extensionsSupported = false;
    for(uint32_t i = 0; i < arraysize(deviceExtensions); i++){
        for(uint32_t k = 0; k < extensionCount; k++){
            if(strcmp(deviceExtensions[i], availableExtensions[k].extensionName) == 0){
                extensionsSupported = true;
                break;
            }
        }
    }

    bool32 swapChainAdequate = false;
    if(extensionsSupported){
        VkSurfaceCapabilitiesKHR capabilities = {};
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(*checkedPhysDevice, renderer->surface, &capabilities);
        uint32_t formatCount = 0, presentModeCount = 0;
        vkGetPhysicalDeviceSurfaceFormatsKHR(*checkedPhysDevice, renderer->surface, &formatCount, nullptr);
        vkGetPhysicalDeviceSurfacePresentModesKHR(*checkedPhysDevice, renderer->surface, &presentModeCount, nullptr);

        if((formatCount > 0) && (presentModeCount > 0))
            swapChainAdequate = true;
    }

    renderer->indices = indices;
    return indices.isComplete && extensionsSupported && swapChainAdequate;
}

static VkFormat FindSupportedDepthFormat(VkPhysicalDevice physDevice)
{
    VkFormat formats[] = {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT};
    VkFormatFeatureFlags features = VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT;
    VkFormat result = VK_FORMAT_UNDEFINED;

    for(uint32_t i = 0; i < arraysize(formats); i++){
        VkFormatProperties props = {};
        vkGetPhysicalDeviceFormatProperties(physDevice, formats[i], &props);

        if((props.optimalTilingFeatures & features) == features){
            result = formats[i];
            break;
        }
    }
    mp_assert(result != VK_FORMAT_UNDEFINED);
    return result;
}

static void PrepareGpu(mpVkRenderer *renderer, mpMemoryRegion tempMemory)
{
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(renderer->instance, &deviceCount, nullptr);
    if(deviceCount == 0)
        puts("Failed to find GPUs with Vulkan Support");

    VkPhysicalDevice* devices = static_cast<VkPhysicalDevice*>(mpAllocateIntoRegion(tempMemory, sizeof(VkPhysicalDevice) * deviceCount));
    vkEnumeratePhysicalDevices(renderer->instance, &deviceCount, devices);

    for(uint32_t i = 0; i < deviceCount; i++){
        if(IsDeviceSuitable(renderer, &devices[i], tempMemory)){
            renderer->gpu = devices[i];
            break;
        }
    }
    renderer->depthFormat = FindSupportedDepthFormat(renderer->gpu);

    if(renderer->gpu == VK_NULL_HANDLE)
        puts("Failed to find a suitable GPU!");
}

static void PrepareDevice(mpVkRenderer *renderer, bool32 enableValidation)
{
    VkDeviceQueueCreateInfo queueCreateInfos[2] = {};
    uint32_t uniqueQueueFamilies[2] = {};
    uint32_t queueCount = 0;

    if(renderer->indices.graphicsFamily.value == renderer->indices.presentFamily.value){
        uniqueQueueFamilies[0] = renderer->indices.graphicsFamily.value;
        queueCount = 1;
    }
    else{
        uniqueQueueFamilies[0] = renderer->indices.graphicsFamily.value;
        uniqueQueueFamilies[1] = renderer->indices.presentFamily.value;
        queueCount = 2;
    }

    float queuePriority = 1.0f;
    for(uint32_t i = 0; i < queueCount; i++){
        queueCreateInfos[i].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfos[i].queueFamilyIndex = uniqueQueueFamilies[i];
        queueCreateInfos[i].queueCount = queueCount;
        queueCreateInfos[i].pQueuePriorities = &queuePriority;
    }

    VkPhysicalDeviceFeatures deviceFeatures = {};

    VkDeviceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.queueCreateInfoCount = queueCount;
    createInfo.pQueueCreateInfos = queueCreateInfos;
    createInfo.pEnabledFeatures = &deviceFeatures;
    createInfo.enabledExtensionCount = arraysize(deviceExtensions);
    createInfo.ppEnabledExtensionNames = deviceExtensions;

    if(enableValidation){
        createInfo.enabledLayerCount = arraysize(validationLayers);
        createInfo.ppEnabledLayerNames = validationLayers;
    }

    VkResult error = vkCreateDevice(renderer->gpu, &createInfo, nullptr, &renderer->device);
    mp_assert(!error);

    vkGetDeviceQueue(renderer->device, renderer->indices.graphicsFamily.value, 0, &renderer->graphicsQueue);
    vkGetDeviceQueue(renderer->device, renderer->indices.presentFamily.value, 0, &renderer->presentQueue);
}

static void PrepareVkRenderer(mpVkRenderer *renderer, bool32 enableValidation, const mpCallbacks *const callbacks, mpMemoryRegion tempMemory)
{
    VkApplicationInfo appInfo = {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Mariposa";
    appInfo.pEngineName = appInfo.pApplicationName;
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo instanceInfo = {};
    instanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceInfo.pApplicationInfo = &appInfo;

    if(enableValidation){
        instanceInfo.enabledLayerCount = arraysize(validationLayers);
        instanceInfo.ppEnabledLayerNames = validationLayers;
    }

    uint32_t extensionsCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionsCount, nullptr);
    VkExtensionProperties* extensionProperties = static_cast<VkExtensionProperties*>(mpAllocateIntoRegion(tempMemory, sizeof(VkExtensionProperties) * extensionsCount));
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionsCount, extensionProperties);

    char* extensions[] = { "VK_KHR_surface", "VK_KHR_win32_surface" };

    instanceInfo.enabledExtensionCount = arraysize(extensions);
    instanceInfo.ppEnabledExtensionNames = extensions;

    for(uint32_t i = 0; i < extensionsCount; i++){
        MP_LOG_INFO("Vk Extennsion %d: %s\n", i, extensionProperties[i].extensionName);
    }

    VkResult error = vkCreateInstance(&instanceInfo, 0, &renderer->instance);
    mp_assert(!error);

    callbacks->GetSurface(renderer->instance, &renderer->surface);

    PrepareGpu(renderer, tempMemory);
    PrepareDevice(renderer, enableValidation);

    // Prepare Descriptor set layouts
    VkDescriptorSetLayoutBinding uboLayoutBinding = {};
    uboLayoutBinding.binding = 0;
    uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboLayoutBinding.descriptorCount = 1;
    uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    uboLayoutBinding.pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutCreateInfo layoutInfo = {};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 1;
    layoutInfo.pBindings = &uboLayoutBinding;

    error = vkCreateDescriptorSetLayout(renderer->device, &layoutInfo, nullptr, &renderer->descriptorSetLayout);
    mp_assert(!error);

    // Prepare command pool
    VkCommandPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = renderer->indices.graphicsFamily.value;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    error = vkCreateCommandPool(renderer->device, &poolInfo, nullptr, &renderer->commandPool);
    mp_assert(!error);

    printf("Vulkan instance, physical device, logical device, descriptor set layouts and command pool prepared\n");
}

inline static uint32_t Uint32Clamp(uint32_t value, uint32_t min, uint32_t max)
{
    if(value < min)
        value = min;
    else if(value > max)
        value = max;

    return value;
}

static VkImageView CreateImageView(VkDevice device, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags)
{
    VkImageViewCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    createInfo.image = image;
    createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    createInfo.format = format;
    createInfo.components = {
        VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY,
        VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY
    };
    createInfo.subresourceRange.aspectMask = aspectFlags;
    createInfo.subresourceRange.baseMipLevel = 0;
    createInfo.subresourceRange.levelCount = 1;
    createInfo.subresourceRange.baseArrayLayer = 0;
    createInfo.subresourceRange.layerCount = 1;

    VkImageView imageView;
    VkResult error = vkCreateImageView(device, &createInfo, nullptr, &imageView);
    mp_assert(!error);

    return imageView;
}

inline VkSurfaceFormatKHR ChooseSwapSurfaceFormat(VkSurfaceFormatKHR *availableFormats, uint32_t formatCount)
{
    for(uint32_t i = 0; i < formatCount; i++)
        if(availableFormats[i].format == VK_FORMAT_B8G8R8A8_SRGB && availableFormats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            return availableFormats[i];

    return availableFormats[0];
}

inline VkPresentModeKHR ChooseSwapPresentMode(VkPresentModeKHR *availablePresentModes, uint32_t presentModeCount)
{
    /* NOTE:
        VK_PRESENT_MODE_IMMEDIATE_KHR   == Vsync OFF
        VK_PRESENT_MODE_FIFO_KHR        == Vsync ON, double buffering
        VK_PRESENT_MODE_MAILBOX_KHR     == Vsync ON, triple buffering
    */
    for(uint32_t i = 0; i < presentModeCount; i++){
        if(availablePresentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR)
            return availablePresentModes[i];
    }

    return VK_PRESENT_MODE_FIFO_KHR;
}

inline VkExtent2D ChooseSwapExtent(VkSurfaceCapabilitiesKHR capabilities, int32_t windowWidth, int32_t windowHeight)
{
    if(capabilities.currentExtent.width != 0xFFFFFFFF){
        return capabilities.currentExtent;
    }
    else{
        VkExtent2D actualExtent = { static_cast<uint32_t>(windowWidth), static_cast<uint32_t>(windowHeight) };

        actualExtent.width = Uint32Clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        actualExtent.height = Uint32Clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

        return actualExtent;
    }
}

static void PrepareVkSwapChain(mpVkRenderer *renderer, mpMemoryRegion vulkanRegion, int32_t width, int32_t height)
{
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(renderer->gpu, renderer->surface, &renderer->swapDetails.capabilities);

    vkGetPhysicalDeviceSurfaceFormatsKHR(renderer->gpu, renderer->surface, &renderer->swapDetails.formatCount, nullptr);
    if(renderer->swapDetails.formatCount > 0){
        renderer->swapDetails.pFormats = static_cast<VkSurfaceFormatKHR*>(mpAllocateIntoRegion(vulkanRegion, sizeof(VkSurfaceFormatKHR) * renderer->swapDetails.formatCount));
        vkGetPhysicalDeviceSurfaceFormatsKHR(renderer->gpu, renderer->surface, &renderer->swapDetails.formatCount, renderer->swapDetails.pFormats);
    }

    vkGetPhysicalDeviceSurfacePresentModesKHR(renderer->gpu, renderer->surface, &renderer->swapDetails.presentModeCount, nullptr);
    if(renderer->swapDetails.presentModeCount > 0){
        renderer->swapDetails.pPresentModes = static_cast<VkPresentModeKHR*>(mpAllocateIntoRegion(vulkanRegion, sizeof(VkSurfaceFormatKHR) * renderer->swapDetails.presentModeCount));
        vkGetPhysicalDeviceSurfacePresentModesKHR(renderer->gpu, renderer->surface, &renderer->swapDetails.presentModeCount, renderer->swapDetails.pPresentModes);
    }

    mp_assert((renderer->swapDetails.formatCount > 0) && (renderer->swapDetails.presentModeCount > 0))

    VkSurfaceFormatKHR surfaceFormat = ChooseSwapSurfaceFormat(renderer->swapDetails.pFormats, renderer->swapDetails.formatCount);
    VkPresentModeKHR presentMode = ChooseSwapPresentMode(renderer->swapDetails.pPresentModes, renderer->swapDetails.presentModeCount);
    VkExtent2D extent = ChooseSwapExtent(renderer->swapDetails.capabilities, width, height);

    uint32_t imageCount = renderer->swapDetails.capabilities.minImageCount + 1;
    if(renderer->swapDetails.capabilities.maxImageCount > 0 && imageCount > renderer->swapDetails.capabilities.maxImageCount)
        imageCount = renderer->swapDetails.capabilities.maxImageCount;

    VkSwapchainCreateInfoKHR createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = renderer->surface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    uint32_t queueFamilyIndices[] = { renderer->indices.graphicsFamily.value, renderer->indices.presentFamily.value };

    if(renderer->indices.graphicsFamily.value != renderer->indices.presentFamily.value){
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    }
    else{
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount = 0;
        createInfo.pQueueFamilyIndices = nullptr;
    }

    createInfo.preTransform = renderer->swapDetails.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = VK_NULL_HANDLE;

    VkResult error = vkCreateSwapchainKHR(renderer->device, &createInfo, nullptr, &renderer->swapChain);
    mp_assert(!error)

    vkGetSwapchainImagesKHR(renderer->device, renderer->swapChain, &imageCount, nullptr);
    renderer->pSwapChainImages = static_cast<VkImage*>(mpAllocateIntoRegion(vulkanRegion, sizeof(VkImageView) * imageCount));
    vkGetSwapchainImagesKHR(renderer->device, renderer->swapChain, &imageCount, renderer->pSwapChainImages);

    renderer->swapChainImageCount = imageCount;
    renderer->swapChainImageFormat = surfaceFormat.format;
    renderer->swapChainExtent = extent;

    renderer->pSwapChainImageViews = static_cast<VkImageView*>(mpAllocateIntoRegion(vulkanRegion, sizeof(VkImageView) * renderer->swapChainImageCount));
    for(uint32_t i = 0; i < renderer->swapChainImageCount; i++)
        renderer->pSwapChainImageViews[i] = CreateImageView(renderer->device, renderer->pSwapChainImages[i], renderer->swapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT);
}

static void RebuildSwapChain(mpVkRenderer *renderer, uint32_t width, uint32_t height)
{
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(renderer->gpu, renderer->surface, &renderer->swapDetails.capabilities);

    vkGetPhysicalDeviceSurfaceFormatsKHR(renderer->gpu, renderer->surface, &renderer->swapDetails.formatCount, nullptr);
    if(renderer->swapDetails.formatCount > 0)
        vkGetPhysicalDeviceSurfaceFormatsKHR(renderer->gpu, renderer->surface, &renderer->swapDetails.formatCount, renderer->swapDetails.pFormats);

    vkGetPhysicalDeviceSurfacePresentModesKHR(renderer->gpu, renderer->surface, &renderer->swapDetails.presentModeCount, nullptr);
    if(renderer->swapDetails.presentModeCount > 0)
        vkGetPhysicalDeviceSurfacePresentModesKHR(renderer->gpu, renderer->surface, &renderer->swapDetails.presentModeCount, renderer->swapDetails.pPresentModes);

    mp_assert((renderer->swapDetails.formatCount > 0) && (renderer->swapDetails.presentModeCount > 0))

    VkSurfaceFormatKHR surfaceFormat = ChooseSwapSurfaceFormat(renderer->swapDetails.pFormats, renderer->swapDetails.formatCount);
    VkPresentModeKHR presentMode = ChooseSwapPresentMode(renderer->swapDetails.pPresentModes, renderer->swapDetails.presentModeCount);
    VkExtent2D extent = ChooseSwapExtent(renderer->swapDetails.capabilities, width, height);

    uint32_t imageCount = renderer->swapDetails.capabilities.minImageCount + 1;
    if(renderer->swapDetails.capabilities.maxImageCount > 0 && imageCount > renderer->swapDetails.capabilities.maxImageCount)
        imageCount = renderer->swapDetails.capabilities.maxImageCount;

    VkSwapchainCreateInfoKHR createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = renderer->surface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    uint32_t queueFamilyIndices[] = { renderer->indices.graphicsFamily.value, renderer->indices.presentFamily.value };

    if(renderer->indices.graphicsFamily.value != renderer->indices.presentFamily.value){
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    }
    else{
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount = 0;
        createInfo.pQueueFamilyIndices = nullptr;
    }

    createInfo.preTransform = renderer->swapDetails.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = VK_NULL_HANDLE;

    VkResult error = vkCreateSwapchainKHR(renderer->device, &createInfo, nullptr, &renderer->swapChain);
    mp_assert(!error)

    vkGetSwapchainImagesKHR(renderer->device, renderer->swapChain, &imageCount, nullptr);
    vkGetSwapchainImagesKHR(renderer->device, renderer->swapChain, &imageCount, renderer->pSwapChainImages);

    renderer->swapChainImageCount = imageCount;
    renderer->swapChainImageFormat = surfaceFormat.format;
    renderer->swapChainExtent = extent;

    // Create image views
    for(uint32_t i = 0; i < renderer->swapChainImageCount; i++)
        renderer->pSwapChainImageViews[i] = CreateImageView(renderer->device, renderer->pSwapChainImages[i], renderer->swapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT);
}

static void PrepareVkRenderPass(mpVkRenderer *renderer)
{
    VkAttachmentDescription colorAttachment = {};
    colorAttachment.format = renderer->swapChainImageFormat;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentRef = {};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentDescription depthAttachment = {};
    depthAttachment.format = renderer->depthFormat;
    depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthAttachmentRef = {};
    depthAttachmentRef.attachment = 1;
    depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;
    subpass.pDepthStencilAttachment = &depthAttachmentRef;

    VkSubpassDependency dependency = {};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkAttachmentDescription attachments[] = {colorAttachment, depthAttachment};
    constexpr uint32_t attachmentsSize = arraysize(attachments);

    VkRenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = attachmentsSize;
    renderPassInfo.pAttachments = attachments;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    VkResult error = vkCreateRenderPass(renderer->device, &renderPassInfo, nullptr, &renderer->renderPass);
    mp_assert(!error);
}

inline static void PrepareShaderModule(const VkDevice &device, VkShaderModule *shaderModule, const mpCallbacks *const callbacks, const char *filePath)
{
    mpFile shader = callbacks->mpReadFile(filePath);

    VkShaderModuleCreateInfo shaderModuleInfo = {};
    shaderModuleInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shaderModuleInfo.codeSize = shader.size;
    shaderModuleInfo.pCode = (const uint32_t*)shader.handle;

    VkResult error = vkCreateShaderModule(device, &shaderModuleInfo, nullptr, shaderModule);
    mp_assert(!error);
    callbacks->mpCloseFile(&shader);
}

static void PrepareShaderModules(mpVkRenderer *renderer, const mpCallbacks *const callbacks)
{
    PrepareShaderModule(renderer->device, &renderer->vertShaderModule, callbacks, "../assets/shaders/vert.spv");
    PrepareShaderModule(renderer->device, &renderer->fragShaderModule, callbacks, "../assets/shaders/frag.spv");

    PrepareShaderModule(renderer->device, &renderer->gui.vertShaderModule, callbacks, "../assets/shaders/gui_vert.spv");
    PrepareShaderModule(renderer->device, &renderer->gui.fragShaderModule, callbacks, "../assets/shaders/gui_frag.spv");
}

inline static VkPipelineShaderStageCreateInfo getShaderStageCreateInfo(VkShaderStageFlagBits stage, VkShaderModule &module)
{
    VkPipelineShaderStageCreateInfo result = {};
    result.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    result.stage = stage;
    result.module = module;
    result.pName = "main";
    return result;
}

inline static VkPipelineInputAssemblyStateCreateInfo getInputAssembly()
{
    VkPipelineInputAssemblyStateCreateInfo result = {};
    result.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    result.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    result.primitiveRestartEnable = VK_FALSE;
    return result;
}

inline static VkViewport getViewport(VkExtent2D extent)
{
    VkViewport result = {};
    result.width = static_cast<float>(extent.width);
    result.height = static_cast<float>(extent.height);
    result.minDepth = 0.0f;
    result.maxDepth = 1.0f;
    return result;
}

inline static VkRect2D getScissor(VkExtent2D extent)
{
    VkRect2D result = {};
    result.offset = { 0, 0 };
    result.extent = extent;
    return result;
}

inline static VkPipelineRasterizationStateCreateInfo getRasterizer(VkFrontFace frontFace)
{
    VkPipelineRasterizationStateCreateInfo result = {};
    result.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    result.depthClampEnable = VK_FALSE;
    result.rasterizerDiscardEnable = VK_FALSE;
    result.polygonMode = VK_POLYGON_MODE_FILL;
    result.lineWidth = 1.0f;
    result.cullMode = VK_CULL_MODE_BACK_BIT;
    result.frontFace = frontFace;
    result.depthBiasEnable = VK_FALSE;
    return result;
}

inline static VkPipelineDepthStencilStateCreateInfo getDepthStencil()
{
    VkPipelineDepthStencilStateCreateInfo result = {};
    result.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    result.depthTestEnable = VK_TRUE;
    result.depthWriteEnable = VK_TRUE;
    result.depthCompareOp = VK_COMPARE_OP_LESS;
    result.depthBoundsTestEnable = VK_FALSE;
    result.stencilTestEnable = VK_FALSE;
    return result;
}

static void PrepareVkPipeline(mpVkRenderer *renderer)
{
    VkPipelineShaderStageCreateInfo vertShaderStageInfo = getShaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, renderer->vertShaderModule);
    VkPipelineShaderStageCreateInfo fragShaderStageInfo = getShaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, renderer->fragShaderModule);
    VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};
    constexpr uint32_t shaderStagesSize = arraysize(shaderStages);

    VkVertexInputBindingDescription bindingDescription = {};
    bindingDescription.binding = 0;
    bindingDescription.stride = sizeof(mpVertex);
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    // TODO: should be constexpr
    VkVertexInputAttributeDescription attributeDescs[3] = {};
    attributeDescs[0].binding = 0;
    attributeDescs[0].location = 0;
    attributeDescs[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescs[0].offset = offsetof(mpVertex, position);

    attributeDescs[1].binding = 0;
    attributeDescs[1].location = 1;
    attributeDescs[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescs[1].offset = offsetof(mpVertex, normal);

    attributeDescs[2].binding = 0;
    attributeDescs[2].location = 2;
    attributeDescs[2].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    attributeDescs[2].offset = offsetof(mpVertex, colour);

    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputInfo.vertexAttributeDescriptionCount = arraysize(attributeDescs);
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescs;

    VkPipelineInputAssemblyStateCreateInfo inputAssembly = getInputAssembly();

    VkViewport viewport = getViewport(renderer->swapChainExtent);
    VkRect2D scissor = getScissor(renderer->swapChainExtent);

    VkPipelineViewportStateCreateInfo viewportState = {};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo rasterizer = getRasterizer(VK_FRONT_FACE_COUNTER_CLOCKWISE);

    VkPipelineMultisampleStateCreateInfo multisampling = {};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineDepthStencilStateCreateInfo depthStencil = getDepthStencil();

    VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo colorBlend = {};
    colorBlend.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlend.logicOpEnable = VK_FALSE;
    colorBlend.logicOp = VK_LOGIC_OP_COPY;
    colorBlend.attachmentCount = 1;
    colorBlend.pAttachments = &colorBlendAttachment;
    colorBlend.blendConstants[0] = 0.0f;
    colorBlend.blendConstants[1] = 0.0f;
    colorBlend.blendConstants[2] = 0.0f;
    colorBlend.blendConstants[3] = 0.0f;

    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.pSetLayouts = &renderer->descriptorSetLayout;
    pipelineLayoutInfo.setLayoutCount = 1;

    VkResult error = vkCreatePipelineLayout(renderer->device, &pipelineLayoutInfo, nullptr, &renderer->pipelineLayout);
    mp_assert(!error);

    VkGraphicsPipelineCreateInfo pipelineInfo = {};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = shaderStagesSize;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.pColorBlendState = &colorBlend;
    pipelineInfo.layout = renderer->pipelineLayout;
    pipelineInfo.renderPass = renderer->renderPass;

    error = vkCreateGraphicsPipelines(renderer->device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &renderer->pipeline);
    mp_assert(!error);
}

static void PrepareVkGuiPipeline(mpVkRenderer *renderer)
{
    VkPipelineShaderStageCreateInfo vertShaderStageInfo = getShaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, renderer->gui.vertShaderModule);
    VkPipelineShaderStageCreateInfo fragShaderStageInfo = getShaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, renderer->gui.fragShaderModule);
    VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };
    constexpr uint32_t shaderStagesCount = arraysize(shaderStages);

    VkVertexInputBindingDescription bindingDescription = {};
    bindingDescription.binding = 0;
    bindingDescription.stride = sizeof(mpGuiVertex);
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    VkVertexInputAttributeDescription attributeDescs[2] = {};
    attributeDescs[0].binding = 0;
    attributeDescs[0].location = 0;
    attributeDescs[0].format = VK_FORMAT_R32G32_SFLOAT;
    attributeDescs[0].offset = offsetof(mpGuiVertex, position);

    attributeDescs[1].binding = 0;
    attributeDescs[1].location = 1;
    attributeDescs[1].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    attributeDescs[1].offset = offsetof(mpGuiVertex, colour);

    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputInfo.vertexAttributeDescriptionCount = arraysize(attributeDescs);
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescs;

    VkPipelineInputAssemblyStateCreateInfo inputAssembly = getInputAssembly();

    VkViewport viewport = getViewport(renderer->swapChainExtent);
    VkRect2D scissor = getScissor(renderer->swapChainExtent);

    VkPipelineViewportStateCreateInfo viewportState = {};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo rasterizer = getRasterizer(VK_FRONT_FACE_CLOCKWISE);

    VkPipelineMultisampleStateCreateInfo multisampling = {};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineDepthStencilStateCreateInfo depthStencil = getDepthStencil();

    VkPipelineColorBlendAttachmentState colourBlendAttachment = {};
    colourBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colourBlendAttachment.blendEnable = VK_TRUE;
    colourBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colourBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colourBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    colourBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colourBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colourBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

    VkPipelineColorBlendStateCreateInfo colourblend = {};
    colourblend.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colourblend.logicOpEnable = VK_FALSE;
    colourblend.logicOp = VK_LOGIC_OP_COPY;
    colourblend.attachmentCount = 1;
    colourblend.pAttachments = &colourBlendAttachment;
    colourblend.blendConstants[0] = 0.0f;
    colourblend.blendConstants[1] = 0.0f;
    colourblend.blendConstants[2] = 0.0f;
    colourblend.blendConstants[3] = 0.0f;

    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

    VkResult error = vkCreatePipelineLayout(renderer->device, &pipelineLayoutInfo, nullptr, &renderer->gui.pipelineLayout);
    mp_assert(!error);

    VkGraphicsPipelineCreateInfo pipelineInfo = {};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = shaderStagesCount;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pColorBlendState = &colourblend;
    pipelineInfo.layout = renderer->gui.pipelineLayout;
    pipelineInfo.renderPass = renderer->renderPass;

    error = vkCreateGraphicsPipelines(renderer->device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &renderer->gui.pipeline);
    mp_assert(!error);
}

inline static uint32_t FindMemoryType(const VkPhysicalDevice &physDevice, uint32_t typeFilter, VkMemoryPropertyFlags propertyFlags)
{
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(physDevice, &memProperties);

    for(uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
        if(typeFilter & (1 << i) && (memProperties.memoryTypes[i].propertyFlags & propertyFlags) == propertyFlags)
            return i;

    puts("Failed to find suitable memory type!");
    return 0;
}

static void PrepareDepthResources(mpVkRenderer *renderer)
{
    VkImageCreateInfo imageInfo = {};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = renderer->swapChainExtent.width;
    imageInfo.extent.height = renderer->swapChainExtent.height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = renderer->depthFormat;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VkResult error = vkCreateImage(renderer->device, &imageInfo, nullptr, &renderer->depthImage);
    mp_assert(!error);

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(renderer->device, renderer->depthImage, &memRequirements);

    VkMemoryAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = FindMemoryType(renderer->gpu, memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    error = vkAllocateMemory(renderer->device, &allocInfo, nullptr, &renderer->depthImageMemory);
    mp_assert(!error);

    vkBindImageMemory(renderer->device, renderer->depthImage, renderer->depthImageMemory, 0);

    renderer->depthImageView = CreateImageView(renderer->device, renderer->depthImage, renderer->depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);
}

static void PrepareVkFrameBuffers(mpVkRenderer *renderer)
{
    for(uint32_t i = 0; i < renderer->swapChainImageCount; i++){
        VkImageView attachments[] = { renderer->pSwapChainImageViews[i] , renderer->depthImageView };
        constexpr uint32_t attachmentsSize = arraysize(attachments);

        VkFramebufferCreateInfo framebufferInfo = {};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = renderer->renderPass;
        framebufferInfo.attachmentCount = attachmentsSize;
        framebufferInfo.pAttachments = attachments;
        framebufferInfo.width = renderer->swapChainExtent.width;
        framebufferInfo.height = renderer->swapChainExtent.height;
        framebufferInfo.layers = 1;

        VkResult error = vkCreateFramebuffer(renderer->device, &framebufferInfo, nullptr, &renderer->pFramebuffers[i]);
        mp_assert(!error);
    }
}


static void CreateBuffer(VkPhysicalDevice gpu, VkDevice device, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags propertyFlags, VkBuffer* buffer, VkDeviceMemory* bufferMemory)
{
    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VkResult error = vkCreateBuffer(device, &bufferInfo, nullptr, buffer);
    mp_assert(!error);

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(device, *buffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = FindMemoryType(gpu, memRequirements.memoryTypeBits, propertyFlags);

    error = vkAllocateMemory(device, &allocInfo, nullptr, bufferMemory);
    mp_assert(!error);

    vkBindBufferMemory(device, *buffer, *bufferMemory, 0);
}

inline static VkCommandBuffer BeginSingleTimeCommands(VkDevice device, VkCommandPool cmdPool)
{
    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = cmdPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    VkResult error = vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);
    mp_assert(!error);

    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    error = vkBeginCommandBuffer(commandBuffer, &beginInfo);
    mp_assert(!error);

    return commandBuffer;
}

inline static void EndSingleTimeCommands(VkDevice device, VkCommandPool cmdPool, VkQueue graphicsQueue, VkCommandBuffer commandBuffer)
{
    VkResult error = vkEndCommandBuffer(commandBuffer);
    mp_assert(!error);

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    error = vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    mp_assert(!error);
    error = vkQueueWaitIdle(graphicsQueue);
    mp_assert(!error);

    vkFreeCommandBuffers(device, cmdPool, 1, &commandBuffer);
}

struct mpMapVertexBufferDataInfo
{
    VkPhysicalDevice gpu;
    VkDevice device;
    VkCommandPool cmdPool;
    VkQueue graphicsQueue;
    void *rawBuffer;
    VkDeviceSize bufferSize;
    VkBuffer *buffer;
    VkDeviceMemory *bufferMemory;
    VkBufferUsageFlags usageFlags;
};

static void mapVertexBufferData(mpMapVertexBufferDataInfo &info)
{
    constexpr uint32_t bufferSrcFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    VkBuffer vertStagingbuffer;
    VkDeviceMemory vertStagingbufferMemory;
    void *vertData;
    CreateBuffer(info.gpu, info.device, info.bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, bufferSrcFlags, &vertStagingbuffer, &vertStagingbufferMemory);
    vkMapMemory(info.device, vertStagingbufferMemory, 0, info.bufferSize, 0, &vertData);
    memcpy(vertData, info.rawBuffer, static_cast<size_t>(info.bufferSize));
    vkUnmapMemory(info.device, vertStagingbufferMemory);
    CreateBuffer(info.gpu, info.device, info.bufferSize, info.usageFlags, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, info.buffer, info.bufferMemory);

    VkCommandBuffer cmdBuffer = BeginSingleTimeCommands(info.device, info.cmdPool);
    VkBufferCopy copyRegion = {};
    copyRegion.size = info.bufferSize;
    vkCmdCopyBuffer(cmdBuffer, vertStagingbuffer, *(info.buffer), 1, &copyRegion);
    EndSingleTimeCommands(info.device, info.cmdPool, info.graphicsQueue, cmdBuffer);

    vkDestroyBuffer(info.device, vertStagingbuffer, nullptr);
    vkFreeMemory(info.device, vertStagingbufferMemory, nullptr);
}

static void PrepareVkGeometryBuffers(mpVkRenderer *renderer, const mpVoxelRegion *region)
{
    constexpr VkBufferUsageFlags vertUsageDstFlags = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    constexpr VkBufferUsageFlags indexUsageDstFlags = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;

    for(int32_t z = 0; z < MP_REGION_SIZE; z++){
        for(int32_t y = 0; y < MP_REGION_SIZE; y++){
            for(int32_t x = 0; x < MP_REGION_SIZE; x++)
            {
                const mpMesh &mesh = region->meshArray[x][y][z];
                if(mesh.vertexCount == 0)
                    continue;

                mpMapVertexBufferDataInfo info = {};
                info.device = renderer->device;
                info.gpu = renderer->gpu;
                info.graphicsQueue = renderer->graphicsQueue;
                info.cmdPool = renderer->commandPool;
                info.bufferSize = mesh.vertexCount * sizeof(mpVertex);
                info.rawBuffer = mesh.vertices;
                info.buffer = &renderer->vertexbuffers[x][y][z];
                info.bufferMemory = &renderer->vertexbufferMemories[x][y][z];
                info.usageFlags = vertUsageDstFlags;
                mapVertexBufferData(info);

                info.bufferSize = mesh.indexCount * sizeof(uint16_t);
                info.rawBuffer = mesh.indices;
                info.buffer = &renderer->indexbuffers[x][y][z];
                info.bufferMemory = &renderer->indexbufferMemories[x][y][z];
                info.usageFlags = indexUsageDstFlags;
                mapVertexBufferData(info);
            }
        }
    }
}

static void PrepareVkCommandbuffers(mpVkRenderer *renderer, mpMemoryRegion tempMemory)
{
    // Prepare uniform buffers
    VkDeviceSize bufferSize = sizeof(UniformbufferObject);
    uint32_t bufferSrcFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

    for(uint32_t i = 0; i < renderer->swapChainImageCount; i++)
        CreateBuffer(renderer->gpu, renderer->device, bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, bufferSrcFlags, &renderer->pUniformbuffers[i], &renderer->pUniformbufferMemories[i]);
    // Prepare descriptor pool
    VkDescriptorPoolSize poolSize = {};
    poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSize.descriptorCount = renderer->swapChainImageCount;

    VkDescriptorPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = &poolSize;
    poolInfo.maxSets = renderer->swapChainImageCount;

    VkResult error = vkCreateDescriptorPool(renderer->device, &poolInfo, nullptr, &renderer->descriptorPool);
    mp_assert(!error);
    // Prepare descriptorsets
    VkDescriptorSetLayout *pLayouts = static_cast<VkDescriptorSetLayout*>(mpAllocateIntoRegion(tempMemory, sizeof(VkDescriptorSetLayout) * renderer->swapChainImageCount));
    for(uint32_t i = 0; i < renderer->swapChainImageCount; i++)
        pLayouts[i] = renderer->descriptorSetLayout;

    VkDescriptorSetAllocateInfo descriptorSetAllocInfo = {};
    descriptorSetAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    descriptorSetAllocInfo.descriptorPool = renderer->descriptorPool;
    descriptorSetAllocInfo.descriptorSetCount = renderer->swapChainImageCount;
    descriptorSetAllocInfo.pSetLayouts = pLayouts;

    error = vkAllocateDescriptorSets(renderer->device, &descriptorSetAllocInfo, renderer->pDescriptorSets);
    mp_assert(!error);

    for(uint32_t i = 0; i < renderer->swapChainImageCount; i++){
        VkDescriptorBufferInfo bufferInfo = {};
        bufferInfo.buffer = renderer->pUniformbuffers[i];
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(UniformbufferObject);

        VkWriteDescriptorSet descriptorWrite = {};
        descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.dstSet = renderer->pDescriptorSets[i];
        descriptorWrite.dstBinding = 0;
        descriptorWrite.dstArrayElement = 0;
        descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrite.descriptorCount = 1;
        descriptorWrite.pBufferInfo = &bufferInfo;

        vkUpdateDescriptorSets(renderer->device, 1, &descriptorWrite, 0, nullptr);
    }
    // Prepare command buffers
    VkCommandBufferAllocateInfo cmdBufferAllocInfo = {};
    cmdBufferAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmdBufferAllocInfo.commandPool = renderer->commandPool;
    cmdBufferAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmdBufferAllocInfo.commandBufferCount = renderer->swapChainImageCount;

    error = vkAllocateCommandBuffers(renderer->device, &cmdBufferAllocInfo, renderer->pCommandbuffers);
    mp_assert(!error);
}

static void PrepareVkSyncObjects(mpVkRenderer *renderer)
{
    VkSemaphoreCreateInfo semaphoreInfo = {};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo = {};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for(uint32_t i = 0; i < MP_MAX_IMAGES_IN_FLIGHT; i++){
        VkResult error = vkCreateSemaphore(renderer->device, &semaphoreInfo, nullptr, &renderer->imageAvailableSemaphores[i]);
        mp_assert(!error);
        error = vkCreateSemaphore(renderer->device, &semaphoreInfo, nullptr, &renderer->renderFinishedSemaphores[i]);
        mp_assert(!error);
        error = vkCreateFence(renderer->device, &fenceInfo, nullptr, &renderer->inFlightFences[i]);
        mp_assert(!error);
    }
}

static bool32 CheckValidationLayerSupport(mpMemoryRegion vulkanRegion)
{
    uint32_t layerCount = 0;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
    VkLayerProperties* availableLayers = static_cast<VkLayerProperties*>(mpAllocateIntoRegion(vulkanRegion, sizeof(VkLayerProperties) * layerCount));
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers);

    bool32 layerFound = false;
    for(uint32_t i = 0; i < arraysize(validationLayers); i++){
        for(uint32_t k = 0; k < layerCount; k++){
            if(strcmp(validationLayers[i], availableLayers[k].layerName) == 0){
                layerFound = true;
                break;
            }
        }
    }
    return layerFound;
}

inline static void beginRender(VkCommandBuffer &commandBuffer, VkRenderPass &renderPass, VkFramebuffer &frameBuffer, VkExtent2D &extent)
{
    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

    VkResult error = vkBeginCommandBuffer(commandBuffer, &beginInfo);
    mp_assert(!error);

    VkClearValue colorValue = {};
    colorValue.color = {0.2f, 0.4f, 0.8f, 1.0f};
    VkClearValue depthStencilValue = {};
    depthStencilValue.depthStencil = { 1.0f, 0 };

    const VkClearValue clearValues[] = {colorValue, depthStencilValue};
    constexpr uint32_t clearValueCount = arraysize(clearValues);

    VkRenderPassBeginInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = renderPass;
    renderPassInfo.framebuffer = frameBuffer;
    renderPassInfo.renderArea.offset = { 0, 0 };
    renderPassInfo.renderArea.extent = extent;
    renderPassInfo.clearValueCount = clearValueCount;
    renderPassInfo.pClearValues = clearValues;

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
}

inline static void endRender(VkCommandBuffer commandBuffer)
{
    vkCmdEndRenderPass(commandBuffer);
    VkResult error = vkEndCommandBuffer(commandBuffer);
    mp_assert(!error);
}

inline static void drawIndexed(VkCommandBuffer commandbuffer, VkBuffer *vertexbuffer, VkBuffer indexbuffer, uint32_t indexCount)
{
    constexpr VkDeviceSize offsets[] = { 0 };
    vkCmdBindVertexBuffers(commandbuffer, 0, 1, vertexbuffer, offsets);
    vkCmdBindIndexBuffer(commandbuffer, indexbuffer, 0, VK_INDEX_TYPE_UINT16);
    vkCmdDrawIndexed(commandbuffer, indexCount, 1, 0, 0, 0);
}

inline static void DrawImages(mpVkRenderer &renderer, const mpVoxelRegion *region, const mpGuiData &guiData)
{
    vkDeviceWaitIdle(renderer.device);
    for(uint32_t image = 0; image < renderer.swapChainImageCount; image++){
        VkCommandBuffer &commandBuffer = renderer.pCommandbuffers[image];
        beginRender(commandBuffer, renderer.renderPass, renderer.pFramebuffers[image], renderer.swapChainExtent);

        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, renderer.pipeline);
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, renderer.pipelineLayout, 0, 1, &renderer.pDescriptorSets[image], 0, nullptr);

        for(int32_t z = 0; z < MP_REGION_SIZE; z++){
            for(int32_t y = 0; y < MP_REGION_SIZE; y++){
                for(int32_t x = 0; x < MP_REGION_SIZE; x++){
                    const mpMesh &mesh = region->meshArray[x][y][z];
                    if(mesh.vertexCount > 0)
                        drawIndexed(commandBuffer, &renderer.vertexbuffers[x][y][z], renderer.indexbuffers[x][y][z], mesh.indexCount);
                }
            }
        }
        endRender(commandBuffer);
    }
}

void mpVulkanInit(mpCore *core, mpMemoryRegion vulkanMemory, mpMemoryRegion tempMemory, bool32 enableValidation)
{
    core->rendererHandle = mpAllocateIntoRegion(vulkanMemory, sizeof(mpVkRenderer));
    mpVkRenderer *renderer = static_cast<mpVkRenderer*>(core->rendererHandle);

    if(enableValidation && !CheckValidationLayerSupport(vulkanMemory))
        puts("WARNING: Validation layers requested, but not available\n");

    PrepareVkRenderer(renderer, enableValidation, &core->callbacks, tempMemory);
    PrepareVkSwapChain(renderer, vulkanMemory, core->windowInfo.width, core->windowInfo.height);

    PrepareVkRenderPass(renderer);
    PrepareShaderModules(renderer, &core->callbacks);
    PrepareVkPipeline(renderer);
    PrepareVkGuiPipeline(renderer);

    renderer->pFramebuffers =          static_cast<VkFramebuffer*>(  mpAllocateIntoRegion(vulkanMemory, sizeof(VkFramebuffer) * renderer->swapChainImageCount));
    renderer->pUniformbuffers =        static_cast<VkBuffer*>(       mpAllocateIntoRegion(vulkanMemory, sizeof(VkBuffer) * renderer->swapChainImageCount));
    renderer->pUniformbufferMemories = static_cast<VkDeviceMemory*>( mpAllocateIntoRegion(vulkanMemory, sizeof(VkDeviceMemory) * renderer->swapChainImageCount));
    renderer->pCommandbuffers =        static_cast<VkCommandBuffer*>(mpAllocateIntoRegion(vulkanMemory, sizeof(VkCommandBuffer) * renderer->swapChainImageCount));
    renderer->pInFlightImageFences =   static_cast<VkFence*>(        mpAllocateIntoRegion(vulkanMemory, sizeof(VkFence) * renderer->swapChainImageCount));
    renderer->pDescriptorSets =        static_cast<VkDescriptorSet*>(mpAllocateIntoRegion(vulkanMemory, sizeof(VkDescriptorSet) * renderer->swapChainImageCount));

    PrepareDepthResources(renderer);
    PrepareVkFrameBuffers(renderer);
    PrepareVkGeometryBuffers(renderer, core->region);
    PrepareVkCommandbuffers(renderer, tempMemory);
    PrepareVkSyncObjects(renderer);
}

void mpVkRecreateGeometryBuffer(mpHandle rendererHandle, mpMesh &mesh, const vec3Int index)
{
    if(mesh.vertexCount == 0)
        return;

    constexpr VkBufferUsageFlags vertUsageDstFlags = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    constexpr VkBufferUsageFlags indexUsageDstFlags = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    mpVkRenderer *renderer = static_cast<mpVkRenderer*>(rendererHandle);

    vkDeviceWaitIdle(renderer->device);
    vkDestroyBuffer(renderer->device, renderer->vertexbuffers[index.x][index.y][index.z], nullptr);
    vkDestroyBuffer(renderer->device, renderer->indexbuffers[index.x][index.y][index.z], nullptr);
    vkFreeMemory(renderer->device, renderer->vertexbufferMemories[index.x][index.y][index.z], nullptr);
    vkFreeMemory(renderer->device, renderer->indexbufferMemories[index.x][index.y][index.z], nullptr);

    mpMapVertexBufferDataInfo info = {};
    info.device = renderer->device;
    info.gpu = renderer->gpu;
    info.graphicsQueue = renderer->graphicsQueue;
    info.cmdPool = renderer->commandPool;
    info.bufferSize = mesh.vertexCount * sizeof(mpVertex);
    info.rawBuffer = mesh.vertices;
    info.buffer = &renderer->vertexbuffers[index.x][index.y][index.z];
    info.bufferMemory = &renderer->vertexbufferMemories[index.x][index.y][index.z];
    info.usageFlags = vertUsageDstFlags;
    mapVertexBufferData(info);

    info.bufferSize = mesh.indexCount * sizeof(uint16_t);
    info.rawBuffer = mesh.indices;
    info.buffer = &renderer->indexbuffers[index.x][index.y][index.z];
    info.bufferMemory = &renderer->indexbufferMemories[index.x][index.y][index.z];
    info.usageFlags = indexUsageDstFlags;
    mapVertexBufferData(info);
}

void mpVkRecreateGUIBuffers(mpHandle rendererHandle, const mpGuiData &guiData)
{
    if(guiData.vertexCount == 0)
        return;

    constexpr VkBufferUsageFlags vertUsageDstFlags = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    constexpr VkBufferUsageFlags indexUsageDstFlags = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    mpVkRenderer *renderer = static_cast<mpVkRenderer*>(rendererHandle);

    vkDeviceWaitIdle(renderer->device);
    vkDestroyBuffer(renderer->device, renderer->gui.vertexbuffer, nullptr);
    vkFreeMemory(renderer->device, renderer->gui.vertexbufferMemory, nullptr);
    vkDestroyBuffer(renderer->device, renderer->gui.indexbuffer, nullptr);
    vkFreeMemory(renderer->device, renderer->gui.indexbufferMemory, nullptr);

    mpMapVertexBufferDataInfo info = {};
    info.device = renderer->device;
    info.gpu = renderer->gpu;
    info.graphicsQueue = renderer->graphicsQueue;
    info.cmdPool = renderer->commandPool;
    info.bufferSize = guiData.vertexCount * sizeof(mpGuiVertex);
    info.rawBuffer = guiData.vertices;
    info.buffer = &renderer->gui.vertexbuffer;
    info.bufferMemory = &renderer->gui.vertexbufferMemory;
    info.usageFlags = vertUsageDstFlags;
    mapVertexBufferData(info);

    info.bufferSize = guiData.indexCount * sizeof(uint16_t);
    info.rawBuffer = guiData.indices;
    info.buffer = &renderer->gui.indexbuffer;
    info.bufferMemory = &renderer->gui.indexbufferMemory;
    info.usageFlags = indexUsageDstFlags;
    mapVertexBufferData(info);
}

static void CleanupSwapChain(mpVkRenderer *renderer)
{
    vkDestroyImageView(renderer->device, renderer->depthImageView, nullptr);
    vkDestroyImage(renderer->device, renderer->depthImage, nullptr);
    vkFreeMemory(renderer->device, renderer->depthImageMemory, nullptr);

    for(uint32_t i = 0; i < renderer->swapChainImageCount; i++)
        vkDestroyFramebuffer(renderer->device, renderer->pFramebuffers[i], nullptr);

    vkFreeCommandBuffers(renderer->device, renderer->commandPool, renderer->swapChainImageCount, renderer->pCommandbuffers);
    vkDestroyPipeline(renderer->device, renderer->pipeline, nullptr);
    vkDestroyPipelineLayout(renderer->device, renderer->pipelineLayout, nullptr);
    vkDestroyPipeline(renderer->device, renderer->gui.pipeline, nullptr);
    vkDestroyPipelineLayout(renderer->device, renderer->gui.pipelineLayout, nullptr);
    vkDestroyRenderPass(renderer->device, renderer->renderPass, nullptr);

    for(uint32_t i = 0; i < renderer->swapChainImageCount; i++){
        vkDestroyImageView(renderer->device, renderer->pSwapChainImageViews[i], nullptr);
        vkDestroyBuffer(renderer->device, renderer->pUniformbuffers[i], nullptr);
        vkFreeMemory(renderer->device, renderer->pUniformbufferMemories[i], nullptr);
    }

    vkDestroySwapchainKHR(renderer->device, renderer->swapChain, nullptr);
    vkDestroyDescriptorPool(renderer->device, renderer->descriptorPool, nullptr);
}

static void RecreateSwapChain(mpVkRenderer *renderer, uint32_t width, uint32_t height, mpMemoryRegion tempMemory)
{
    vkDeviceWaitIdle(renderer->device);
    CleanupSwapChain(renderer);

    RebuildSwapChain(renderer, width, height);
    PrepareVkRenderPass(renderer);
    PrepareVkPipeline(renderer);
    PrepareVkGuiPipeline(renderer);
    PrepareDepthResources(renderer);
    PrepareVkFrameBuffers(renderer);
    PrepareVkCommandbuffers(renderer, tempMemory);
}

inline static void UpdateUBOs(mpVkRenderer *renderer, const mpCamera *camera, const mpPointLight *light, uint32_t imageIndex)
{
    renderer->ubo.Model = camera->model;
    renderer->ubo.View = camera->view;
    renderer->ubo.Proj = camera->projection;
    renderer->ubo.position = light->position;
    renderer->ubo.constant = light->constant;
    renderer->ubo.linear = light->linear;
    renderer->ubo.quadratic = light->quadratic;
    renderer->ubo.diffuse = light->diffuse;
    renderer->ubo.ambient = light->ambient;

    void *data;
    constexpr size_t dataSize = sizeof(UniformbufferObject);
    vkMapMemory(renderer->device, renderer->pUniformbufferMemories[imageIndex], 0, dataSize, 0, &data);
    memcpy(data, &renderer->ubo, dataSize);
    vkUnmapMemory(renderer->device, renderer->pUniformbufferMemories[imageIndex]);
}

inline static void DrawFrame(mpVkRenderer &renderer, const mpVoxelRegion *region, const uint32_t guiIndexCount, const uint32_t imageIndex)
{
    vkDeviceWaitIdle(renderer.device);
    VkCommandBuffer &commandBuffer = renderer.pCommandbuffers[imageIndex];
    beginRender(commandBuffer, renderer.renderPass, renderer.pFramebuffers[imageIndex], renderer.swapChainExtent);

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, renderer.pipeline);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, renderer.pipelineLayout, 0, 1, &renderer.pDescriptorSets[imageIndex], 0, nullptr);

    for(int32_t z = 0; z < MP_REGION_SIZE; z++){
        for(int32_t y = 0; y < MP_REGION_SIZE; y++){
            for(int32_t x = 0; x < MP_REGION_SIZE; x++){
                const mpMesh &mesh = region->meshArray[x][y][z];
                if(mesh.vertexCount > 0)
                    drawIndexed(commandBuffer, &renderer.vertexbuffers[x][y][z], renderer.indexbuffers[x][y][z], mesh.indexCount);
            }
        }
    }
    if(guiIndexCount > 0){
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, renderer.gui.pipeline);
        drawIndexed(commandBuffer, &renderer.gui.vertexbuffer, renderer.gui.indexbuffer, guiIndexCount);
    }

    endRender(commandBuffer);
}

void mpVulkanUpdate(mpCore *core, mpMemoryRegion vulkanMemory, mpMemoryRegion tempMemory)
{
    if(core->windowInfo.width == 0 || core->windowInfo.height == 0)
        return;

    mpVkRenderer *renderer = static_cast<mpVkRenderer*>(core->rendererHandle);

    vkWaitForFences(renderer->device, 1, &renderer->inFlightFences[renderer->currentFrame], VK_TRUE, UINT64MAX);

    uint32_t imageIndex = 0;
    VkResult imageResult = vkAcquireNextImageKHR(renderer->device, renderer->swapChain, UINT64MAX, renderer->imageAvailableSemaphores[renderer->currentFrame], VK_NULL_HANDLE, &imageIndex);
    if(imageResult == VK_ERROR_OUT_OF_DATE_KHR) {
        RecreateSwapChain(renderer, core->windowInfo.width, core->windowInfo.height, tempMemory);
        DrawFrame(*renderer, core->region, core->gui.data.indexCount, imageIndex);
        return;
    }
    else if(imageResult != VK_SUCCESS && imageResult != VK_SUBOPTIMAL_KHR) {
        puts("Failed to acquire swap chain image!");
    }

    UpdateUBOs(renderer, &core->camera, &core->pointLight, imageIndex);
    DrawFrame(*renderer, core->region, core->gui.data.indexCount, imageIndex);

    if(renderer->pInFlightImageFences[imageIndex] != VK_NULL_HANDLE)
        vkWaitForFences(renderer->device, 1, &renderer->pInFlightImageFences[imageIndex], VK_TRUE, UINT64MAX);

    renderer->pInFlightImageFences[imageIndex] = renderer->inFlightFences[renderer->currentFrame];

    VkSemaphore waitSemaphores[] = { renderer->imageAvailableSemaphores[renderer->currentFrame] };
    VkSemaphore signalSemaphores[] = { renderer->renderFinishedSemaphores[renderer->currentFrame] };
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &renderer->pCommandbuffers[imageIndex];
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    vkResetFences(renderer->device, 1, &renderer->inFlightFences[renderer->currentFrame]);

    VkResult error = vkQueueSubmit(renderer->graphicsQueue, 1, &submitInfo, renderer->inFlightFences[renderer->currentFrame]);
    mp_assert(!error);

    const VkSwapchainKHR swapChains[] = { renderer->swapChain };

    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &imageIndex;

    VkResult presentResult = vkQueuePresentKHR(renderer->presentQueue, &presentInfo);
    if(presentResult == VK_ERROR_OUT_OF_DATE_KHR || presentResult == VK_SUBOPTIMAL_KHR || core->windowInfo.hasResized)
        RecreateSwapChain(renderer, core->windowInfo.width, core->windowInfo.height, tempMemory);

    renderer->currentFrame = (renderer->currentFrame + 1) % MP_MAX_IMAGES_IN_FLIGHT;
}

void mpVulkanCleanup(mpHandle *rendererHandle)
{
    mpVkRenderer *renderer = static_cast<mpVkRenderer*>(*rendererHandle);

    vkDeviceWaitIdle(renderer->device);
    CleanupSwapChain(renderer);

    vkDestroyShaderModule(renderer->device, renderer->vertShaderModule, nullptr);
    vkDestroyShaderModule(renderer->device, renderer->fragShaderModule, nullptr);
    vkDestroyShaderModule(renderer->device, renderer->gui.vertShaderModule, nullptr);
    vkDestroyShaderModule(renderer->device, renderer->gui.fragShaderModule, nullptr);
    vkDestroyDescriptorSetLayout(renderer->device, renderer->descriptorSetLayout, nullptr);

    for(int32_t z = 0; z < MP_REGION_SIZE; z++){
        for(int32_t y = 0; y < MP_REGION_SIZE; y++){
            for(int32_t x = 0; x < MP_REGION_SIZE; x++){
                vkDestroyBuffer(renderer->device, renderer->indexbuffers[x][y][z], nullptr);
                vkFreeMemory(renderer->device, renderer->indexbufferMemories[x][y][z], nullptr);
                vkDestroyBuffer(renderer->device, renderer->vertexbuffers[x][y][z], nullptr);
                vkFreeMemory(renderer->device, renderer->vertexbufferMemories[x][y][z], nullptr);
            }
        }
    }
    vkDestroyBuffer(renderer->device, renderer->gui.vertexbuffer, nullptr);
    vkDestroyBuffer(renderer->device, renderer->gui.indexbuffer, nullptr);
    vkFreeMemory(renderer->device, renderer->gui.vertexbufferMemory, nullptr);
    vkFreeMemory(renderer->device, renderer->gui.indexbufferMemory, nullptr);

    for(uint32_t i = 0; i < MP_MAX_IMAGES_IN_FLIGHT; i++) {
        vkDestroySemaphore(renderer->device, renderer->imageAvailableSemaphores[i], nullptr);
        vkDestroySemaphore(renderer->device, renderer->renderFinishedSemaphores[i], nullptr);
        vkDestroyFence(renderer->device, renderer->inFlightFences[i], nullptr);
    }

    vkDestroyCommandPool(renderer->device, renderer->commandPool, nullptr);
    vkDestroyDevice(renderer->device, nullptr);
    vkDestroySurfaceKHR(renderer->instance, renderer->surface, nullptr);
    vkDestroyInstance(renderer->instance, nullptr);
}
