#include "newVulkanLayer.h"
#include "C:\Users\arlev\Documents\GitHub\Mariposa_v2\1.2.162.0\Include\vulkan\vulkan.h"
#include <stdio.h>

constexpr char *validationLayers[] = {"VK_LAYER_KHRONOS_validation"};
constexpr char *deviceExtensions[] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
constexpr char *extensions[] = { "VK_KHR_surface", "VK_KHR_win32_surface" };
const uint32_t MP_MAX_IMAGES_IN_FLIGHT = 2;

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

struct mpUniformBufferObject
{
    alignas(16) mat4x4 Model;
    alignas(16) mat4x4 View;
    alignas(16) mat4x4 Proj;

	vec3 position;
	vec3 colour;
	float ambient;
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

    VkFramebuffer *pFramebuffers;
    bool32 *pFramebufferResized;
    uint32_t currentFrame;

    VkCommandPool commandPool;
    VkCommandBuffer *pCommandbuffers;

    struct
    {
        VkPipeline graphicsPipeline;

        VkBuffer *vertexbuffers;
        VkDeviceMemory *vertexbufferMemories;
        VkBuffer *indexbuffers;
        VkDeviceMemory *indexbufferMemories;
        VkBuffer *pUniformbuffers;
        VkDeviceMemory *pUniformbufferMemories;

        VkShaderModule vertShaderModule;
        VkShaderModule fragShaderModule;
    } scene;

    struct
    {
        VkPipeline graphicsPipeline;

        VkBuffer vertexbuffer;
        VkDeviceMemory vertexbufferMemory;
        VkBuffer indexbuffer;
        VkDeviceMemory indexbufferMemory;

        VkShaderModule vertShaderModule;
        VkShaderModule fragShaderModule;
    } gui;

    VkImage depthImage;
    VkFormat depthFormat;
    VkDeviceMemory depthImageMemory;
    VkImageView depthImageView;

    VkSemaphore imageAvailableSemaphores[MP_MAX_IMAGES_IN_FLIGHT];
    VkSemaphore renderFinishedSemaphores[MP_MAX_IMAGES_IN_FLIGHT];
    VkFence inFlightFences[MP_MAX_IMAGES_IN_FLIGHT];
    VkFence *pInFlightImageFences;

    mpUniformBufferObject ubo;
};

static void PrepareRenderPass(VkFormat swapchainImageFormat, VkFormat depthFormat, VkDevice device, VkRenderPass &renderPass)
{

}

void mpVkInit(mpCore *core, mpMemoryRegion *vulkanMemory, mpMemoryRegion *tempMemory, bool32 enableValidation)
{
    core->rendererHandle = mpAllocateIntoRegion(vulkanMemory, sizeof(mpVkRenderer));
    mpVkRenderer *renderer = static_cast<mpVkRenderer*>(core->rendererHandle);

    // Find validation layer
    uint32_t layerCount = 0;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
    VkLayerProperties *vLayerProperties = static_cast<VkLayerProperties*>(mpAllocateIntoRegion(tempMemory, sizeof(VkLayerProperties) * layerCount));
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    bool32 validationLayerFound = false;
    constexpr uint32_t validationLayersCount = arraysize(validationLayers);
    for(uint32_t vLayer = 0; layer < validationLayersCount; layer++)
    {
        for(uint32_t prop = 0; prop < layerCount; prop++)
        {
            if(strcmp(validationLayers[layer], vLayerProperties[prop]) == 0)
            {
                validationLayerFound = true;
                break;
            }
        }
    }
    if(enableValidation && !validationLayerFound)
        puts("WARNING: Validation layers requested, but not available\n");

    VkApplicationInfo appInfo = {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = core->name;
    appInfo.pEngineName = appInfo.pApplicationName;
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo instanceInfo = {};
    instanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceInfo.pApplicationInfo = &appInfo;

    if(enableValidation)
    {
        instanceInfo.enabledLayerCount = arraysize(validationLayers);
        instanceInfo.ppEnabledLayerNames = validationLayers;
    }

    uint32_t extensionsCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionsCount, nullptr);
    VkExtensionProperties* extensionProperties = static_cast<VkExtensionProperties*>(mpAllocateIntoRegion(tempMemory, sizeof(VkExtensionProperties) * extensionsCount));
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionsCount, extensionProperties);

    constexpr uint32_t enabledExtensionCount = arraysize(extensions);
    instanceInfo.enabledExtensionCount = enabledExtensionCount;
    instanceInfo.ppEnabledExtensionNames = extensions;

    for(uint32_t i = 0; i < extensionsCount; i++)
    {
        MP_LOG_INFO("Vk Extennsion %d: %s\n", i, extensionProperties[i].extensionName);
    }

    VkResult error = vkCreateInstance(&instanceInfo, 0, &renderer->instance);
    mp_assert(!error);

    core->callbacks.GetSurface(renderer->instance, &renderer->surface);
    // Prepare gpu
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(renderer->instance, &deviceCount, nullptr);
    if(deviceCount == 0)
        puts("Failed to find GPUs with Vulkan Support");

    // Check device suitability
    VkPhysicalDevice *physDevices = static_cast<VkPhysicalDevice*>(mpAllocateIntoRegion(tempMemory, sizeof(VkPhysicalDevice) * deviceCount));
    vkEnumeratePhysicalDevices(renderer->instance, &deviceCount, physDevices);

    for(uint32_t i = 0; i < deviceCount; i++)
    {
        QueueFamilyIndices indices = {};
        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(physDevices[i], &queueFamilyCount, nullptr);
        VkQueueFamilyProperties* queueFamilyProperties = static_cast<VkQueueFamilyProperties*>(mpAllocateIntoRegion(tempMemory, sizeof(VkQueueFamilyProperties) * queueFamilyCount));
        vkGetPhysicalDeviceQueueFamilyProperties(physDevices[i], &queueFamilyCount, queueFamilyProperties);

        for(uint32_t prop = 0; prop < queueFamilyCount; prop++)
        {
            if(queueFamilyProperties[prop].queueFlags & VK_QUEUE_GRAPHICS_BIT)
            {
                indices.graphicsFamily.value = prop;
                indices.graphicsFamily.hasValue = true;
            }
            VkBool32 presentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(*checkedPhysDevice, i, renderer->surface, &presentSupport);
            if(presentSupport)
            {
                indices.presentFamily.value = prop;
                indices.presentFamily.hasValue = true;
            }
            if(indices.graphicsFamily.hasValue && indices.presentFamily.hasValue)
            {
                indices.isComplete = true;
                break;
            }
        }

        uint32_t extensionCount = 0;
        vkEnumerateDeviceExtensionProperties(*checkedPhysDevice, nullptr, &extensionCount, nullptr);
        VkExtensionProperties* availableExtensions = static_cast<VkExtensionProperties*>(malloc(sizeof(VkExtensionProperties) * extensionCount));
        vkEnumerateDeviceExtensionProperties(*checkedPhysDevice, nullptr, &extensionCount, availableExtensions);

        bool32 extensionsSupported = false;
        for(uint32_t i = 0; i < arraysize(deviceExtensions); i++)
        {
            for(uint32_t k = 0; k < extensionCount; k++)
            {
                if(strcmp(deviceExtensions[i], availableExtensions[k].extensionName) == 0)
                {
                    extensionsSupported = true;
                    break;
                }
            }
        }
        bool32 swapChainAdequate = false;
        if(extensionsSupported)
        {
            VkSurfaceCapabilitiesKHR capabilities = {};
            vkGetPhysicalDeviceSurfaceCapabilitiesKHR(*checkedPhysDevice, renderer->surface, &capabilities);
            uint32_t formatCount = 0, presentModeCount = 0;
            vkGetPhysicalDeviceSurfaceFormatsKHR(*checkedPhysDevice, renderer->surface, &formatCount, nullptr);
            vkGetPhysicalDeviceSurfacePresentModesKHR(*checkedPhysDevice, renderer->surface, &presentModeCount, nullptr);

            if((formatCount > 0) && (presentModeCount > 0))
                swapChainAdequate = true;
        }
        renderer->indices = indices;
        if(indices.isComplete && extensionsSupported && swapChainAdequate)
        {
            renderer->gpu = devices[i];
            break;
        }
    }
    VkFormat formats[] = {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT};
    VkFormatFeatureFlags features = VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT;
    VkFormat depthFormat = VK_FORMAT_UNDEFINED;

    constexpr uint32_t formatCount = arraysize(formats);
    for(uint32_t i = 0; i < formatCount; i++)
    {
        VkFormatProperties props = {};
        vkGetPhysicalDeviceFormatProperties(physDevice, formats[i], &props);

        if((props.optimalTilingFeatures & features) == features)
        {
            depthFormat = formats[i];
            break;
        }
    }
    mp_assert(result != VK_FORMAT_UNDEFINED);
    renderer->depthFormat = depthFormat;

    if(renderer->gpu == VK_NULL_HANDLE)
        puts("Failed to find a suitable GPU!");

    // Prepare device
    VkDeviceQueueCreateInfo queueCreateInfos[2] = {};
    uint32_t uniqueQueueFamilies[2] = {};
    uint32_t queueCount = 0;

    if(renderer->indices.graphicsFamily.value == renderer->indices.presentFamily.value)
    {
        uniqueQueueFamilies[0] = renderer->indices.graphicsFamily.value;
        queueCount = 1;
    }
    else
    {
        uniqueQueueFamilies[0] = renderer->indices.graphicsFamily.value;
        uniqueQueueFamilies[1] = renderer->indices.presentFamily.value;
        queueCount = 2;
    }

    float queuePriority = 1.0f;
    for(uint32_t i = 0; i < queueCount; i++)
    {
        queueCreateInfos[i].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfos[i].queueFamilyIndex = uniqueQueueFamilies[i];
        queueCreateInfos[i].queueCount = queueCount;
        queueCreateInfos[i].pQueuePriorities = &queuePriority;
    }

    VkPhysicalDeviceFeatures deviceFeatures = {};

    constexpr uint32_t enabledExtensionCount = arraysize(deviceExtensions);
    VkDeviceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.queueCreateInfoCount = queueCount;
    createInfo.pQueueCreateInfos = queueCreateInfos;
    createInfo.pEnabledFeatures = &deviceFeatures;
    createInfo.enabledExtensionCount = enabledExtensionCount;
    createInfo.ppEnabledExtensionNames = deviceExtensions;

    if(enableValidation)
    {
        createInfo.enabledLayerCount = validationLayersCount;
        createInfo.ppEnabledLayerNames = validationLayers;
    }

    VkResult error = vkCreateDevice(renderer->gpu, &createInfo, nullptr, &renderer->device);
    mp_assert(!error);

    vkGetDeviceQueue(renderer->device, renderer->indices.graphicsFamily.value, 0, &renderer->graphicsQueue);
    vkGetDeviceQueue(renderer->device, renderer->indices.presentFamily.value, 0, &renderer->presentQueue);

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

    // Prepare swap chain
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(renderer->gpu, renderer->surface, &renderer->swapDetails.capabilities);
    vkGetPhysicalDeviceSurfaceFormatsKHR(renderer->gpu, renderer->surface, &renderer->swapDetails.formatCount, nullptr);
    if(renderer->swapDetails.formatCount > 0)
    {
        renderer->swapDetails.pFormats = static_cast<VkSurfaceFormatKHR*>(mpAllocateIntoRegion(vulkanRegion, sizeof(VkSurfaceFormatKHR) * renderer->swapDetails.formatCount));
        vkGetPhysicalDeviceSurfaceFormatsKHR(renderer->gpu, renderer->surface, &renderer->swapDetails.formatCount, renderer->swapDetails.pFormats);
    }

    vkGetPhysicalDeviceSurfacePresentModesKHR(renderer->gpu, renderer->surface, &renderer->swapDetails.presentModeCount, nullptr);
    if(renderer->swapDetails.presentModeCount > 0)
    {
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

    if(renderer->indices.graphicsFamily.value != renderer->indices.presentFamily.value)
    {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    }
    else
    {
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

    PrepareRenderPass(renderer->swapChainImageFormat, renderer->depthFormat, renderer->device, renderer->renderPass);

    puts("Vulkan initialised");
}