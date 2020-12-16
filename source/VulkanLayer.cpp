#include "VulkanLayer.h"
#include "C:\Users\arlev\Documents\GitHub\Mariposa_v2\1.2.162.0\Include\vulkan\vulkan.h"
#include <stdio.h>
// TODO: static global variables
const char* validationLayers[] = {"VK_LAYER_KHRONOS_validation"};
const char* deviceExtensions[] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

static bool32 IsDeviceSuitable(mpVkRenderer *renderer, VkPhysicalDevice *checkedPhysDevice)
{
    QueueFamilyIndices indices = {};
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(*checkedPhysDevice, &queueFamilyCount, nullptr);
    VkQueueFamilyProperties* queueFamilyProperties = static_cast<VkQueueFamilyProperties*>(malloc(sizeof(VkQueueFamilyProperties) * queueFamilyCount));
    vkGetPhysicalDeviceQueueFamilyProperties(*checkedPhysDevice, &queueFamilyCount, queueFamilyProperties);

    for(uint32_t i = 0; i < queueFamilyCount; i++)
    {
        if(queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            indices.graphicsFamily = i;
            if(!indices.hasGraphicsFamily)
                indices.hasGraphicsFamily = true;
        }

        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(*checkedPhysDevice, i, renderer->surface, &presentSupport);
        if(presentSupport)
        {
            indices.presentFamily = i;
            if(!indices.hasPresentFamily)
                indices.hasPresentFamily = true;
        }

        if(indices.hasGraphicsFamily && indices.hasPresentFamily)
        {
            indices.isComplete = true;
            break;
        }
    }
    free(queueFamilyProperties);

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
    free(availableExtensions);

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
    return indices.isComplete && extensionsSupported && swapChainAdequate;
}

static VkFormat FindSupportedDepthFormat(VkPhysicalDevice physDevice)
{
    VkFormat formats[] = {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT};
    VkFormatFeatureFlags features = VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT;
    VkFormat result = VK_FORMAT_UNDEFINED;

    for(uint32_t i = 0; i < arraysize(formats); i++)
    {
        VkFormatProperties props = {};
        vkGetPhysicalDeviceFormatProperties(physDevice, formats[i], &props);

        if((props.optimalTilingFeatures & features) == features)
        {
            result = formats[i];
            break;
        }
    }
    mp_assert(result != VK_FORMAT_UNDEFINED);
    return result;
}

static void PrepareGpu(mpVkRenderer *renderer)
{
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(renderer->instance, &deviceCount, nullptr);
    if(deviceCount == 0)
        printf("Failed to find GPUs with Vulkan Support");

    VkPhysicalDevice* devices = static_cast<VkPhysicalDevice*>(malloc(sizeof(VkPhysicalDevice) * deviceCount));
    vkEnumeratePhysicalDevices(renderer->instance, &deviceCount, devices);

    for(uint32_t i = 0; i < deviceCount; i++)
    {
        if(IsDeviceSuitable(renderer, &devices[i]))
        {
            renderer->gpu = devices[i];
            break;
        }
    }
    free(devices);

    renderer->depthFormat = FindSupportedDepthFormat(renderer->gpu);

    if(renderer->gpu == VK_NULL_HANDLE)
        printf("Failed to find a suitable GPU!");
}

static void PrepareDevice(mpVkRenderer *renderer, bool32 enableValidation)
{
    VkDeviceQueueCreateInfo queueCreateInfos[2] = {};
    uint32_t uniqueQueueFamilies[2] = {};
    uint32_t queueCount = 0;

    if(renderer->indices.graphicsFamily == renderer->indices.presentFamily)
    {
        uniqueQueueFamilies[0] = renderer->indices.graphicsFamily;
        queueCount = 1;
    }
    else
    {
        uniqueQueueFamilies[0] = renderer->indices.graphicsFamily;
        uniqueQueueFamilies[1] = renderer->indices.presentFamily;
        queueCount = 2;
    }

    float queuePriority = 1.0f;
    for(uint32_t i = 0; i < queueCount; i++)
    {
        VkDeviceQueueCreateInfo queueCreateInfo = {};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = uniqueQueueFamilies[i];
        queueCreateInfo.queueCount = queueCount;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos[i] = queueCreateInfo;
    }

    VkPhysicalDeviceFeatures deviceFeatures = {};

    VkDeviceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.queueCreateInfoCount = queueCount;
    createInfo.pQueueCreateInfos = queueCreateInfos;
    createInfo.pEnabledFeatures = &deviceFeatures;
    createInfo.enabledExtensionCount = arraysize(deviceExtensions);
    createInfo.ppEnabledExtensionNames = deviceExtensions;

    if(enableValidation)
    {
        createInfo.enabledLayerCount = arraysize(validationLayers);
        createInfo.ppEnabledLayerNames = validationLayers;
    }

    VkResult error = vkCreateDevice(renderer->gpu, &createInfo, nullptr, &renderer->device);
    mp_assert(!error);

    vkGetDeviceQueue(renderer->device, renderer->indices.graphicsFamily, 0, &renderer->graphicsQueue);
    vkGetDeviceQueue(renderer->device, renderer->indices.presentFamily, 0, &renderer->presentQueue);
}

static void PrepareVkRenderer(mpVkRenderer *renderer, bool32 enableValidation, const mpCallbacks *const callbacks)
{
    VkApplicationInfo appInfo = {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Mariposa";
    appInfo.pEngineName = "Mariposa";
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
    VkExtensionProperties* extensionProperties = static_cast<VkExtensionProperties*>(malloc(sizeof(VkExtensionProperties) * extensionsCount));
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionsCount, extensionProperties);

    char* extensions[] = { "VK_KHR_surface", "VK_KHR_win32_surface" };

    instanceInfo.enabledExtensionCount = arraysize(extensions);
    instanceInfo.ppEnabledExtensionNames = extensions;

    MP_LOG_INFO
    for(uint32_t i = 0; i < extensionsCount; i++)
        printf("Vk Extennsion %d: %s\n", i, extensionProperties[i].extensionName);
    MP_LOG_RESET

    free(extensionProperties);

    VkResult error = vkCreateInstance(&instanceInfo, 0, &renderer->instance);
    mp_assert(!error);

    callbacks->GetSurface(renderer->instance, &renderer->surface);

    PrepareGpu(renderer);
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
    poolInfo.queueFamilyIndex = renderer->indices.graphicsFamily;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    error = vkCreateCommandPool(renderer->device, &poolInfo, nullptr, &renderer->commandPool);
    mp_assert(!error);

    printf("Vulkan instance, physical device, logical device, descriptor set layouts and command pool prepared\n");
}

static uint32_t Uint32Clamp(uint32_t value, uint32_t min, uint32_t max)
{
    if(value < min)
        return min;
    else if(value > max)
        return max;

    return value;
}

static VkImageView CreateImageView(VkDevice device, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags)
{
    VkImageViewCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    createInfo.image = image;
    createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    createInfo.format = format;
    createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
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

VkSurfaceFormatKHR ChooseSwapSurfaceFormat(VkSurfaceFormatKHR *availableFormats, uint32_t formatCount)
{
    for(uint32_t i = 0; i < formatCount; i++)
    {
        if(availableFormats[i].format == VK_FORMAT_B8G8R8A8_SRGB && availableFormats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            return availableFormats[i];
    }

    return availableFormats[0];
}

VkPresentModeKHR ChooseSwapPresentMode(VkPresentModeKHR *availablePresentModes, uint32_t presentModeCount)
{
    /* NOTE:
        VK_PRESENT_MODE_IMMEDIATE_KHR   == Vsync OFF
        VK_PRESENT_MODE_FIFO_KHR        == Vsync ON, double buffering
        VK_PRESENT_MODE_MAILBOX_KHR     == Vsync ON, triple buffering
    */
    for(uint32_t i = 0; i < presentModeCount; i++)
    {
        if(availablePresentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR)
            return availablePresentModes[i];
    }

    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D ChooseSwapExtent(VkSurfaceCapabilitiesKHR capabilities, int32_t windowWidth, int32_t windowHeight)
{
    if(capabilities.currentExtent.width != 0xFFFFFFFF)
    {
        return capabilities.currentExtent;
    }
    else
    {
        VkExtent2D actualExtent = { static_cast<uint32_t>(windowWidth), static_cast<uint32_t>(windowHeight) };

        actualExtent.width = Uint32Clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        actualExtent.height = Uint32Clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

        return actualExtent;
    }
}

static void PrepareVkSwapChain(mpVkRenderer *renderer, mpMemoryRegion *vulkanRegion, int32_t width, int32_t height)
{
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(renderer->gpu, renderer->surface, &renderer->swapDetails.capabilities);

    vkGetPhysicalDeviceSurfaceFormatsKHR(renderer->gpu, renderer->surface, &renderer->swapDetails.formatCount, nullptr);
    if(renderer->swapDetails.formatCount > 0)
    {
        renderer->swapDetails.pFormats = reinterpret_cast<VkSurfaceFormatKHR*>(mpAllocateIntoRegion(vulkanRegion, sizeof(VkSurfaceFormatKHR) * renderer->swapDetails.formatCount));
        vkGetPhysicalDeviceSurfaceFormatsKHR(renderer->gpu, renderer->surface, &renderer->swapDetails.formatCount, renderer->swapDetails.pFormats);
    }

    vkGetPhysicalDeviceSurfacePresentModesKHR(renderer->gpu, renderer->surface, &renderer->swapDetails.presentModeCount, nullptr);
    if(renderer->swapDetails.presentModeCount > 0)
    {
        renderer->swapDetails.pPresentModes = reinterpret_cast<VkPresentModeKHR*>(mpAllocateIntoRegion(vulkanRegion, sizeof(VkSurfaceFormatKHR) * renderer->swapDetails.presentModeCount));
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

    uint32_t queueFamilyIndices[] = { renderer->indices.graphicsFamily, renderer->indices.presentFamily };

    if(renderer->indices.graphicsFamily != renderer->indices.presentFamily)
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
    renderer->pSwapChainImages = reinterpret_cast<VkImage*>(mpAllocateIntoRegion(vulkanRegion, sizeof(VkImageView) * imageCount));
    vkGetSwapchainImagesKHR(renderer->device, renderer->swapChain, &imageCount, renderer->pSwapChainImages);

    renderer->swapChainImageCount = imageCount;
    renderer->swapChainImageFormat = surfaceFormat.format;
    renderer->swapChainExtent = extent;

    renderer->pSwapChainImageViews = reinterpret_cast<VkImageView*>(mpAllocateIntoRegion(vulkanRegion, sizeof(VkImageView) * renderer->swapChainImageCount));
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

    uint32_t queueFamilyIndices[] = { renderer->indices.graphicsFamily, renderer->indices.presentFamily };

    if(renderer->indices.graphicsFamily != renderer->indices.presentFamily)
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
    // TODO: allcoate images
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

    VkRenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = arraysize(attachments);
    renderPassInfo.pAttachments = attachments;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    VkResult error = vkCreateRenderPass(renderer->device, &renderPassInfo, nullptr, &renderer->renderPass);
    mp_assert(!error);
}

static void PrepareShaderModules(mpVkRenderer *renderer, const mpCallbacks *const callbacks)
{
    renderer->vertexShader = callbacks->mpReadFile("../assets/shaders/vert.spv");
    renderer->fragmentShader = callbacks->mpReadFile("../assets/shaders/frag.spv");

    VkShaderModuleCreateInfo shaderModuleInfo = {};
    shaderModuleInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shaderModuleInfo.codeSize = renderer->vertexShader.size;
    shaderModuleInfo.pCode = (const uint32_t*)renderer->vertexShader.handle;

    VkResult error = vkCreateShaderModule(renderer->device, &shaderModuleInfo, nullptr, &renderer->vertShaderModule);
    mp_assert(!error);

    shaderModuleInfo.codeSize = renderer->fragmentShader.size;
    shaderModuleInfo.pCode = (const uint32_t*)renderer->fragmentShader.handle;

    error = vkCreateShaderModule(renderer->device, &shaderModuleInfo, nullptr, &renderer->fragShaderModule);
    mp_assert(!error);
}

static void PrepareVkPipeline(mpVkRenderer *renderer)
{
    VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = renderer->vertShaderModule;
    vertShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = renderer->fragShaderModule;
    fragShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

    VkVertexInputBindingDescription bindingDescription = {};
    bindingDescription.binding = 0;
    bindingDescription.stride = sizeof(mpVertex);
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

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

    VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)renderer->swapChainExtent.width;
    viewport.height = (float)renderer->swapChainExtent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor = {};
    scissor.offset = { 0, 0 };
    scissor.extent = renderer->swapChainExtent;

    VkPipelineViewportStateCreateInfo viewportState = {};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo rasterizer = {};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;

    VkPipelineMultisampleStateCreateInfo multisampling = {};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineDepthStencilStateCreateInfo depthStencil = {};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_TRUE;
    depthStencil.depthWriteEnable = VK_TRUE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.stencilTestEnable = VK_FALSE;

    VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;
    /*
    colorBlendAttachment.blendEnable = VK_TRUE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
    */
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
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &renderer->descriptorSetLayout;

    VkResult error = vkCreatePipelineLayout(renderer->device, &pipelineLayoutInfo, nullptr, &renderer->pipelineLayout);
    mp_assert(!error);

    VkGraphicsPipelineCreateInfo pipelineInfo = {};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = arraysize(shaderStages);
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

    error = vkCreateGraphicsPipelines(renderer->device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &renderer->graphicsPipeline);
    mp_assert(!error);
}

static uint32_t FindMemoryType(const VkPhysicalDevice *physDevice, uint32_t typeFilter, VkMemoryPropertyFlags propertyFlags)
{
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(*physDevice, &memProperties);

    for(uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
    {
        if(typeFilter & (1 << i) && (memProperties.memoryTypes[i].propertyFlags & propertyFlags) == propertyFlags)
            return i;
    }

    OutputDebugStringA("Failed to find suitable memory type!");
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
    allocInfo.memoryTypeIndex = FindMemoryType(&renderer->gpu, memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    error = vkAllocateMemory(renderer->device, &allocInfo, nullptr, &renderer->depthImageMemory);
    mp_assert(!error);

    vkBindImageMemory(renderer->device, renderer->depthImage, renderer->depthImageMemory, 0);

    renderer->depthImageView = CreateImageView(renderer->device, renderer->depthImage, renderer->depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);
}

static void PrepareVkFrameBuffers(mpVkRenderer *renderer)
{
    PrepareDepthResources(renderer);

    for(uint32_t i = 0; i < renderer->swapChainImageCount; i++)
    {
        VkImageView attachments[] = { renderer->pSwapChainImageViews[i] , renderer->depthImageView };

        VkFramebufferCreateInfo framebufferInfo = {};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = renderer->renderPass;
        framebufferInfo.attachmentCount = arraysize(attachments);
        framebufferInfo.pAttachments = attachments;
        framebufferInfo.width = renderer->swapChainExtent.width;
        framebufferInfo.height = renderer->swapChainExtent.height;
        framebufferInfo.layers = 1;

        VkResult error = vkCreateFramebuffer(renderer->device, &framebufferInfo, nullptr, &renderer->pFramebuffers[i]);
        mp_assert(!error);
    }
}

static void CreateBuffer(mpVkRenderer *renderer, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags propertyFlags, VkBuffer* buffer, VkDeviceMemory* bufferMemory)
{
    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VkResult error = vkCreateBuffer(renderer->device, &bufferInfo, nullptr, buffer);
    mp_assert(!error);

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(renderer->device, *buffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = FindMemoryType(&renderer->gpu, memRequirements.memoryTypeBits, propertyFlags);

    error = vkAllocateMemory(renderer->device, &allocInfo, nullptr, bufferMemory);
    mp_assert(!error);

    vkBindBufferMemory(renderer->device, *buffer, *bufferMemory, 0);
}

static VkCommandBuffer BeginSingleTimeCommands(mpVkRenderer *renderer)
{
    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = renderer->commandPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    VkResult error = vkAllocateCommandBuffers(renderer->device, &allocInfo, &commandBuffer);
    mp_assert(!error);

    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    error = vkBeginCommandBuffer(commandBuffer, &beginInfo);
    mp_assert(!error);

    return commandBuffer;
}

static void EndSingleTimeCommands(mpVkRenderer *renderer, VkCommandBuffer commandBuffer)
{
    VkResult error = vkEndCommandBuffer(commandBuffer);
    mp_assert(!error);

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    error = vkQueueSubmit(renderer->graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    mp_assert(!error);
    error = vkQueueWaitIdle(renderer->graphicsQueue);
    mp_assert(!error);

    vkFreeCommandBuffers(renderer->device, renderer->commandPool, 1, &commandBuffer);
}

static void PrepareVkGeometryBuffers(mpVkRenderer *renderer, const mpRenderData *renderData)
{
    uint32_t bufferSrcFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    const VkBufferUsageFlags vertUsageDstFlags = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    const VkBufferUsageFlags indexUsageDstFlags = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;

    for(uint32_t i = 0; i < renderData->meshCount; i++)
    {
        if(renderData->meshes[i].vertexCount == 0)
            continue;

        const VkDeviceSize vertbufferSize = renderData->meshes[i].vertexCount * sizeof(mpVertex);
        const VkDeviceSize indexbufferSize = renderData->meshes[i].indexCount * sizeof(uint16_t);
        VkBuffer vertStagingbuffer, indexStagingbuffer;
        VkDeviceMemory vertStagingbufferMemory, indexStagingbufferMemory;
        void *vertData, *indexData;

        CreateBuffer(renderer, vertbufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, bufferSrcFlags, &vertStagingbuffer, &vertStagingbufferMemory);
        CreateBuffer(renderer, indexbufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, bufferSrcFlags, &indexStagingbuffer, &indexStagingbufferMemory);
        vkMapMemory(renderer->device, vertStagingbufferMemory, 0, vertbufferSize, 0, &vertData);
        vkMapMemory(renderer->device, indexStagingbufferMemory, 0, indexbufferSize, 0, &indexData);
        memcpy(vertData, renderData->meshes[i].vertices, static_cast<size_t>(vertbufferSize));
        memcpy(indexData, renderData->meshes[i].indices, static_cast<size_t>(indexbufferSize));
        vkUnmapMemory(renderer->device, vertStagingbufferMemory);
        vkUnmapMemory(renderer->device, indexStagingbufferMemory);

        CreateBuffer(renderer, vertbufferSize, vertUsageDstFlags, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &renderer->vertexbuffers[i], &renderer->vertexbufferMemories[i]);
        CreateBuffer(renderer, indexbufferSize, indexUsageDstFlags, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &renderer->indexbuffers[i], &renderer->indexbufferMemories[i]);

        VkCommandBuffer cmdBuffer = BeginSingleTimeCommands(renderer);
        VkBufferCopy copyRegion = {};
        copyRegion.size = vertbufferSize;
        vkCmdCopyBuffer(cmdBuffer, vertStagingbuffer, renderer->vertexbuffers[i], 1, &copyRegion);
        EndSingleTimeCommands(renderer, cmdBuffer);

        cmdBuffer = BeginSingleTimeCommands(renderer);
        copyRegion = {};
        copyRegion.size = indexbufferSize;
        vkCmdCopyBuffer(cmdBuffer, indexStagingbuffer, renderer->indexbuffers[i], 1, &copyRegion);
        EndSingleTimeCommands(renderer, cmdBuffer);

        vkDestroyBuffer(renderer->device, vertStagingbuffer, nullptr);
        vkDestroyBuffer(renderer->device, indexStagingbuffer, nullptr);
        vkFreeMemory(renderer->device, vertStagingbufferMemory, nullptr);
        vkFreeMemory(renderer->device, indexStagingbufferMemory, nullptr);
    }
}

static void PrepareVkCommandbuffers(mpVkRenderer *renderer, const mpRenderData *renderData)
{
    // Prepare uniform buffers
    VkDeviceSize bufferSize = sizeof(UniformbufferObject);
    uint32_t bufferSrcFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

    for(uint32_t i = 0; i < renderer->swapChainImageCount; i++)
        CreateBuffer(renderer, bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, bufferSrcFlags, &renderer->pUniformbuffers[i], &renderer->pUniformbufferMemories[i]);
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
    VkDescriptorSetLayout *pLayouts = static_cast<VkDescriptorSetLayout*>(malloc(sizeof(VkDescriptorSetLayout) * renderer->swapChainImageCount));// TODO: use mp memory system
    for(uint32_t i = 0; i < renderer->swapChainImageCount; i++)
        pLayouts[i] = renderer->descriptorSetLayout;

    VkDescriptorSetAllocateInfo descriptorSetAllocInfo = {};
    descriptorSetAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    descriptorSetAllocInfo.descriptorPool = renderer->descriptorPool;
    descriptorSetAllocInfo.descriptorSetCount = renderer->swapChainImageCount;
    descriptorSetAllocInfo.pSetLayouts = pLayouts;

    error = vkAllocateDescriptorSets(renderer->device, &descriptorSetAllocInfo, renderer->pDescriptorSets);
    mp_assert(!error);
    // TODO: FUSE LOOPS TOGETHER
    for(uint32_t i = 0; i < renderer->swapChainImageCount; i++)
    {
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
    free(pLayouts);
    // Prepare command buffers
    VkCommandBufferAllocateInfo cmdBufferAllocInfo = {};
    cmdBufferAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmdBufferAllocInfo.commandPool = renderer->commandPool;
    cmdBufferAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmdBufferAllocInfo.commandBufferCount = renderer->swapChainImageCount;

    error = vkAllocateCommandBuffers(renderer->device, &cmdBufferAllocInfo, renderer->pCommandbuffers);
    mp_assert(!error);
    // TODO: FUSE LOOPS TOGETHER
    for(uint32_t i = 0; i < renderer->swapChainImageCount; i++)
    {
        VkCommandBufferBeginInfo beginInfo = {};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

        error = vkBeginCommandBuffer(renderer->pCommandbuffers[i], &beginInfo);
        mp_assert(!error);

        VkRenderPassBeginInfo renderPassInfo = {};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = renderer->renderPass;
        renderPassInfo.framebuffer = renderer->pFramebuffers[i];
        renderPassInfo.renderArea.offset = { 0, 0 };
        renderPassInfo.renderArea.extent = renderer->swapChainExtent;

        VkClearValue clearValues[2];
        clearValues[0].color  = { 0.2f, 0.5f, 1.0f, 1.0f };
        clearValues[1].depthStencil = { 1.0f, 0 };
        renderPassInfo.clearValueCount = arraysize(clearValues);
        renderPassInfo.pClearValues = clearValues;

        vkCmdBeginRenderPass(renderer->pCommandbuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdBindPipeline(renderer->pCommandbuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, renderer->graphicsPipeline);

        const VkDeviceSize offsets[] = { 0 };

        vkCmdBindDescriptorSets(renderer->pCommandbuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, renderer->pipelineLayout, 0, 1, &renderer->pDescriptorSets[i], 0, nullptr);

        for(uint32_t k = 0; k < renderData->meshCount; k++)
        {
            if(renderData->meshes[k].vertexCount == 0)
                continue;

            vkCmdBindVertexBuffers(renderer->pCommandbuffers[i], 0, 1, &renderer->vertexbuffers[k], offsets);
            vkCmdBindIndexBuffer(renderer->pCommandbuffers[i], renderer->indexbuffers[k], 0, VK_INDEX_TYPE_UINT16);
            vkCmdDrawIndexed(renderer->pCommandbuffers[i], renderData->meshes[k].indexCount, 1, 0, 0, 0);
        }

        vkCmdEndRenderPass(renderer->pCommandbuffers[i]);

        error = vkEndCommandBuffer(renderer->pCommandbuffers[i]);
        mp_assert(!error);
    }
}

static void PrepareVkSyncObjects(mpVkRenderer *renderer)
{
    VkSemaphoreCreateInfo semaphoreInfo = {};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo = {};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for(uint32_t i = 0; i < MP_MAX_IMAGES_IN_FLIGHT; i++)
    {
        VkResult error = vkCreateSemaphore(renderer->device, &semaphoreInfo, nullptr, &renderer->imageAvailableSemaphores[i]);
        mp_assert(!error);
        error = vkCreateSemaphore(renderer->device, &semaphoreInfo, nullptr, &renderer->renderFinishedSemaphores[i]);
        mp_assert(!error);
        error = vkCreateFence(renderer->device, &fenceInfo, nullptr, &renderer->inFlightFences[i]);
        mp_assert(!error);
    }
}

static bool32 CheckValidationLayerSupport(mpMemoryRegion *vulkanRegion)
{
    uint32_t layerCount = 0;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
    VkLayerProperties* availableLayers = reinterpret_cast<VkLayerProperties*>(mpAllocateIntoRegion(vulkanRegion, sizeof(VkLayerProperties) * layerCount));
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers);

    bool32 layerFound = false;
    for(uint32_t i = 0; i < arraysize(validationLayers); i++)
    {
        for(uint32_t k = 0; k < layerCount; k++)
        {
            if(strcmp(validationLayers[i], availableLayers[k].layerName) == 0)
            {
                layerFound = true;
                break;
            }
        }
    }
    return layerFound;
}

void mpVulkanInit(mpCore *core, mpMemoryRegion *vulkanRegion)
{
    core->rendererHandle = mpAllocateIntoRegion(vulkanRegion, sizeof(mpVkRenderer));
    mpVkRenderer *renderer = static_cast<mpVkRenderer*>(core->rendererHandle);

    bool32 enableValidation = true;
    if(enableValidation && !CheckValidationLayerSupport(vulkanRegion))
        printf("WARNING: Validation layers requested, but not available\n");

    PrepareVkRenderer(renderer, enableValidation, &core->callbacks);
    PrepareVkSwapChain(renderer, vulkanRegion, core->windowInfo.width, core->windowInfo.height);

    PrepareVkRenderPass(renderer);
    PrepareShaderModules(renderer, &core->callbacks);
    PrepareVkPipeline(renderer);

    renderer->pFramebuffers =          static_cast<VkFramebuffer*>(  mpAllocateIntoRegion(vulkanRegion, sizeof(VkFramebuffer) * renderer->swapChainImageCount));
    renderer->pUniformbuffers =        static_cast<VkBuffer*>(       mpAllocateIntoRegion(vulkanRegion, sizeof(VkBuffer) * renderer->swapChainImageCount));
    renderer->pUniformbufferMemories = static_cast<VkDeviceMemory*>( mpAllocateIntoRegion(vulkanRegion, sizeof(VkDeviceMemory) * renderer->swapChainImageCount));
    renderer->pCommandbuffers =        static_cast<VkCommandBuffer*>(mpAllocateIntoRegion(vulkanRegion, sizeof(VkCommandBuffer) * renderer->swapChainImageCount));
    renderer->pInFlightImageFences =   static_cast<VkFence*>(        mpAllocateIntoRegion(vulkanRegion, sizeof(VkFence) * renderer->swapChainImageCount));
    renderer->pDescriptorSets =        static_cast<VkDescriptorSet*>(mpAllocateIntoRegion(vulkanRegion, sizeof(VkDescriptorSet) * renderer->swapChainImageCount));
    renderer->vertexbuffers =          static_cast<VkBuffer*>(       mpAllocateIntoRegion(vulkanRegion, sizeof(VkBuffer) * core->renderData.meshCount));
    renderer->indexbuffers =           static_cast<VkBuffer*>(       mpAllocateIntoRegion(vulkanRegion, sizeof(VkBuffer) * core->renderData.meshCount));
    renderer->vertexbufferMemories =   static_cast<VkDeviceMemory*>( mpAllocateIntoRegion(vulkanRegion, sizeof(VkDeviceMemory) * core->renderData.meshCount));
    renderer->indexbufferMemories =    static_cast<VkDeviceMemory*>( mpAllocateIntoRegion(vulkanRegion, sizeof(VkDeviceMemory) * core->renderData.meshCount));

    PrepareVkFrameBuffers(renderer);
    PrepareVkGeometryBuffers(renderer, &core->renderData);
    PrepareVkCommandbuffers(renderer, &core->renderData);
    PrepareVkSyncObjects(renderer);
}

static void CleanupSwapChain(mpVkRenderer *renderer)
{
    vkDestroyImageView(renderer->device, renderer->depthImageView, nullptr);
    vkDestroyImage(renderer->device, renderer->depthImage, nullptr);
    vkFreeMemory(renderer->device, renderer->depthImageMemory, nullptr);

    for(uint32_t i = 0; i < renderer->swapChainImageCount; i++)
        vkDestroyFramebuffer(renderer->device, renderer->pFramebuffers[i], nullptr);

    vkFreeCommandBuffers(renderer->device, renderer->commandPool, renderer->swapChainImageCount, renderer->pCommandbuffers);
    vkDestroyPipeline(renderer->device, renderer->graphicsPipeline, nullptr);
    vkDestroyPipelineLayout(renderer->device, renderer->pipelineLayout, nullptr);
    vkDestroyRenderPass(renderer->device, renderer->renderPass, nullptr);

    for(uint32_t i = 0; i < renderer->swapChainImageCount; i++)
    {
        vkDestroyImageView(renderer->device, renderer->pSwapChainImageViews[i], nullptr);
        vkDestroyBuffer(renderer->device, renderer->pUniformbuffers[i], nullptr);
        vkFreeMemory(renderer->device, renderer->pUniformbufferMemories[i], nullptr);
    }

    vkDestroySwapchainKHR(renderer->device, renderer->swapChain, nullptr);
    vkDestroyDescriptorPool(renderer->device, renderer->descriptorPool, nullptr);
}

static void RecreateSwapChain(mpVkRenderer *renderer, const mpRenderData *renderData, uint32_t width, uint32_t height)
{
    vkDeviceWaitIdle(renderer->device);
    CleanupSwapChain(renderer);

    RebuildSwapChain(renderer, width, height);
    PrepareVkRenderPass(renderer);
    PrepareVkPipeline(renderer);
    PrepareVkFrameBuffers(renderer);
    PrepareVkCommandbuffers(renderer, renderData);
}
// Recreates geometry buffers and signals vulkanupdate to recreate swapchain
void mpVulkanRecreateGeometryBuffer(mpHandle rendererHandle, mpMesh *mesh, uint32_t index)
{
    mpVkRenderer *renderer = static_cast<mpVkRenderer*>(rendererHandle);

    vkDeviceWaitIdle(renderer->device);
    vkDestroyBuffer(renderer->device, renderer->vertexbuffers[index], nullptr);
    vkDestroyBuffer(renderer->device, renderer->indexbuffers[index], nullptr);
    vkFreeMemory(renderer->device, renderer->vertexbufferMemories[index], nullptr);
    vkFreeMemory(renderer->device, renderer->indexbufferMemories[index], nullptr);

    const uint32_t bufferSrcFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    const VkBufferUsageFlags vertUsageDstFlags = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    const VkBufferUsageFlags indexUsageDstFlags = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;

    const VkDeviceSize vertbufferSize = mesh->vertexCount * sizeof(mpVertex);
    const VkDeviceSize indexbufferSize = mesh->indexCount * sizeof(uint16_t);
    VkBuffer vertStagingbuffer, indexStagingbuffer;
    VkDeviceMemory vertStagingbufferMemory, indexStagingbufferMemory;
    void *vertData, *indexData;

    CreateBuffer(renderer, vertbufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, bufferSrcFlags, &vertStagingbuffer, &vertStagingbufferMemory);
    CreateBuffer(renderer, indexbufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, bufferSrcFlags, &indexStagingbuffer, &indexStagingbufferMemory);
    vkMapMemory(renderer->device, vertStagingbufferMemory, 0, vertbufferSize, 0, &vertData);
    vkMapMemory(renderer->device, indexStagingbufferMemory, 0, indexbufferSize, 0, &indexData);
    memcpy(vertData, mesh->vertices, static_cast<size_t>(vertbufferSize));
    memcpy(indexData, mesh->indices, static_cast<size_t>(indexbufferSize));
    vkUnmapMemory(renderer->device, vertStagingbufferMemory);
    vkUnmapMemory(renderer->device, indexStagingbufferMemory);

    CreateBuffer(renderer, vertbufferSize, vertUsageDstFlags, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &renderer->vertexbuffers[index], &renderer->vertexbufferMemories[index]);
    CreateBuffer(renderer, indexbufferSize, indexUsageDstFlags, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &renderer->indexbuffers[index], &renderer->indexbufferMemories[index]);

    VkCommandBuffer cmdBuffer = BeginSingleTimeCommands(renderer);
    VkBufferCopy copyRegion = {};
    copyRegion.size = vertbufferSize;
    vkCmdCopyBuffer(cmdBuffer, vertStagingbuffer, renderer->vertexbuffers[index], 1, &copyRegion);
    EndSingleTimeCommands(renderer, cmdBuffer);

    cmdBuffer = BeginSingleTimeCommands(renderer);
    copyRegion = {};
    copyRegion.size = indexbufferSize;
    vkCmdCopyBuffer(cmdBuffer, indexStagingbuffer, renderer->indexbuffers[index], 1, &copyRegion);
    EndSingleTimeCommands(renderer, cmdBuffer);

    vkDestroyBuffer(renderer->device, vertStagingbuffer, nullptr);
    vkDestroyBuffer(renderer->device, indexStagingbuffer, nullptr);
    vkFreeMemory(renderer->device, vertStagingbufferMemory, nullptr);
    vkFreeMemory(renderer->device, indexStagingbufferMemory, nullptr);
}

static void DrawMeshes(mpHandle rendererHandle, mpRenderData *renderData)
{
    mpVkRenderer *renderer = static_cast<mpVkRenderer*>(rendererHandle);

    for(uint32_t i = 0; i < renderer->swapChainImageCount; i++)
    {
        VkCommandBufferBeginInfo beginInfo = {};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

        VkResult error = vkBeginCommandBuffer(renderer->pCommandbuffers[i], &beginInfo);
        mp_assert(!error);

        VkRenderPassBeginInfo renderPassInfo = {};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = renderer->renderPass;
        renderPassInfo.framebuffer = renderer->pFramebuffers[i];
        renderPassInfo.renderArea.offset = { 0, 0 };
        renderPassInfo.renderArea.extent = renderer->swapChainExtent;

        VkClearValue clearValues[2];
        clearValues[0].color  = { 0.2f, 0.5f, 1.0f, 1.0f };
        clearValues[1].depthStencil = { 1.0f, 0 };
        renderPassInfo.clearValueCount = arraysize(clearValues);
        renderPassInfo.pClearValues = clearValues;

        vkCmdBeginRenderPass(renderer->pCommandbuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdBindPipeline(renderer->pCommandbuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, renderer->graphicsPipeline);

        const VkDeviceSize offsets[] = { 0 };

        vkCmdBindDescriptorSets(renderer->pCommandbuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, renderer->pipelineLayout, 0, 1, &renderer->pDescriptorSets[i], 0, nullptr);

        for(uint32_t k = 0; k < renderData->meshCount; k++)
        {
            if(renderData->meshes[k].vertexCount == 0)
                continue;

            vkCmdBindVertexBuffers(renderer->pCommandbuffers[i], 0, 1, &renderer->vertexbuffers[k], offsets);
            vkCmdBindIndexBuffer(renderer->pCommandbuffers[i], renderer->indexbuffers[k], 0, VK_INDEX_TYPE_UINT16);
            vkCmdDrawIndexed(renderer->pCommandbuffers[i], renderData->meshes[k].indexCount, 1, 0, 0, 0);
        }

        vkCmdEndRenderPass(renderer->pCommandbuffers[i]);

        error = vkEndCommandBuffer(renderer->pCommandbuffers[i]);
        mp_assert(!error);
    }
}

static void UpdateUBOs(mpVkRenderer *renderer, const mpCamera *camera, const mpGlobalLight *gLight, uint32_t imageIndex)
{
    renderer->ubo.Model = camera->model;
    renderer->ubo.View = camera->view;
    renderer->ubo.Proj = camera->projection;
    renderer->ubo.ambient = gLight->ambient;
    renderer->ubo.position = gLight->position;
    renderer->ubo.colour = gLight->colour;

    void* data;
    VkDeviceSize dataSize = sizeof(renderer->ubo);
    vkMapMemory(renderer->device, renderer->pUniformbufferMemories[imageIndex], 0, dataSize, 0, &data);
    memcpy(data, &renderer->ubo, dataSize);
    vkUnmapMemory(renderer->device, renderer->pUniformbufferMemories[imageIndex]);
}

void mpVulkanUpdate(mpCore *core, mpMemoryRegion *vulkanRegion)
{
    mpVkRenderer *renderer = static_cast<mpVkRenderer*>(core->rendererHandle);

    if(core->windowInfo.width == 0 || core->windowInfo.height == 0)
        return;

    vkWaitForFences(renderer->device, 1, &renderer->inFlightFences[renderer->currentFrame], VK_TRUE, UINT64MAX);

    uint32_t imageIndex = 0;
    VkResult imageResult = vkAcquireNextImageKHR(renderer->device, renderer->swapChain, UINT64MAX, renderer->imageAvailableSemaphores[renderer->currentFrame], VK_NULL_HANDLE, &imageIndex);
    if(imageResult == VK_ERROR_OUT_OF_DATE_KHR)
    {
        RecreateSwapChain(renderer, &core->renderData, core->windowInfo.width, core->windowInfo.height);
        return;
    }
    else if(imageResult != VK_SUCCESS && imageResult != VK_SUBOPTIMAL_KHR)
    {
        printf("Failed to acquire swap chain image!");
    }

    UpdateUBOs(renderer, &core->camera, &core->globalLight, imageIndex);

    if(renderer->pInFlightImageFences[imageIndex] != VK_NULL_HANDLE)
        vkWaitForFences(renderer->device, 1, &renderer->pInFlightImageFences[imageIndex], VK_TRUE, UINT64MAX);
    if(core->renderFlags & MP_RENDER_FLAG_REDRAW_MESHES)
        DrawMeshes(core->rendererHandle, &core->renderData);

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

    VkSwapchainKHR swapChains[] = { renderer->swapChain };

    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &imageIndex;

    VkResult presentResult = vkQueuePresentKHR(renderer->presentQueue, &presentInfo);
    if(presentResult == VK_ERROR_OUT_OF_DATE_KHR || presentResult == VK_SUBOPTIMAL_KHR || core->windowInfo.hasResized)
        RecreateSwapChain(renderer, &core->renderData, core->windowInfo.width, core->windowInfo.height);
    else if(presentResult != VK_SUCCESS)
        puts("Failed to present swap chain image!");

    renderer->currentFrame = (renderer->currentFrame + 1) % MP_MAX_IMAGES_IN_FLIGHT;
}

void mpVulkanCleanup(mpHandle *rendererHandle, uint32_t meshCount)
{
    mpVkRenderer *renderer = static_cast<mpVkRenderer*>(*rendererHandle);

    vkDeviceWaitIdle(renderer->device);
    CleanupSwapChain(renderer);

    vkDestroyShaderModule(renderer->device, renderer->vertShaderModule, nullptr);
    vkDestroyShaderModule(renderer->device, renderer->fragShaderModule, nullptr);
    vkDestroyDescriptorSetLayout(renderer->device, renderer->descriptorSetLayout, nullptr);

    for(uint32_t i = 0; i < meshCount; i++)
    {
        vkDestroyBuffer(renderer->device, renderer->indexbuffers[i], nullptr);
        vkFreeMemory(renderer->device, renderer->indexbufferMemories[i], nullptr);
        vkDestroyBuffer(renderer->device, renderer->vertexbuffers[i], nullptr);
        vkFreeMemory(renderer->device, renderer->vertexbufferMemories[i], nullptr);
    }

    for(uint32_t i = 0; i < MP_MAX_IMAGES_IN_FLIGHT; i++)
    {
        vkDestroySemaphore(renderer->device, renderer->imageAvailableSemaphores[i], nullptr);
        vkDestroySemaphore(renderer->device, renderer->renderFinishedSemaphores[i], nullptr);
        vkDestroyFence(renderer->device, renderer->inFlightFences[i], nullptr);
    }

    vkDestroyCommandPool(renderer->device, renderer->commandPool, nullptr);
    vkDestroyDevice(renderer->device, nullptr);
    vkDestroySurfaceKHR(renderer->instance, renderer->surface, nullptr);
    vkDestroyInstance(renderer->instance, nullptr);
}
