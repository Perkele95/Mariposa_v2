#include "renderer.hpp"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

static bool32 IsInitalised = false;
// public methods
void mpRenderer::LinkMemory(mpAllocator _mainAllocator, mpAllocator _tempAllocator)
{
    mainAllocator = _mainAllocator;
    tempAllocator = _tempAllocator;
}

void mpRenderer::LoadShaders(const mpCallbacks &callbacks)
{
    LoadShaderModule(callbacks, &scene.vertShaderModule, "../assets/shaders/vert.spv");
    LoadShaderModule(callbacks, &scene.fragShaderModule, "../assets/shaders/frag.spv");
    LoadShaderModule(callbacks, &gui.vertShaderModule, "../assets/shaders/gui_vert.spv");
    LoadShaderModule(callbacks, &gui.fragShaderModule, "../assets/shaders/gui_frag.spv");
}

void mpRenderer::InitDevice(mpCore &core, bool32 enableValidation)
{
    mp_assert(IsInitalised == false)
    IsInitalised = true;

    // Vulkan :: Check validation support if enabled
    if(enableValidation){
        uint32_t layerCount = 0;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
        VkLayerProperties *availableLayers = mpAllocate<VkLayerProperties>(tempAllocator, layerCount);
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
        if(layerFound == false){
            MP_PUTS_WARN("WARNING: Validation layer requested but not available");
        }
    }
    // Vulkan :: instance
    VkApplicationInfo appInfo = {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = core.name;
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
    VkExtensionProperties *extensionProperties = mpAllocate<VkExtensionProperties>(tempAllocator, extensionsCount);
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionsCount, extensionProperties);

    instanceInfo.enabledExtensionCount = arraysize(requiredExtensions);
    instanceInfo.ppEnabledExtensionNames = requiredExtensions;

    MP_PUTS_INFO("---------------------------------");
    MP_PUTS_INFO("Vulkan extensions:");
    for(uint32_t i = 0; i < extensionsCount; i++){
        MP_LOG_INFO("   %d: %s\n", i, extensionProperties[i].extensionName);
    }
    MP_PUTS_INFO("---------------------------------");

    VkResult error = vkCreateInstance(&instanceInfo, 0, &instance);
    mp_assert(!error)

    core.callbacks.GetSurface(instance, &surface);

    // Vulkan :: gpu
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
    mp_assert(deviceCount > 0)
    VkPhysicalDevice *physDevices = mpAllocate<VkPhysicalDevice>(tempAllocator, deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, physDevices);

    // Find suitable gpu
    for(uint32_t physDev = 0; physDev < deviceCount; physDev++){
        if(CheckPhysicalDeviceSuitability(physDevices[physDev])){
            gpu = physDevices[physDev];
            break;
        }
    }
    separatePresentQueue = graphicsQueueFamily != presentQueueFamily;
    // Find support depth format
    VkFormat depthFormats[] = {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT};
    VkFormatFeatureFlags features = VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT;
    for(uint32_t format = 0; format < arraysize(depthFormats); format++){
        VkFormatProperties props = {};
        vkGetPhysicalDeviceFormatProperties(gpu, depthFormats[format], &props);
        if(props.optimalTilingFeatures & features)
            depth.format = depthFormats[format];
    }
    mp_assert(depth.format != VK_FORMAT_UNDEFINED)

    if(gpu == VK_NULL_HANDLE){
        MP_PUTS_ERROR("CRITICAL ERROR: No suitable gpu")
    }
    // Vulkan :: logical device
    uint32_t uniqueQueueFamilies[2] = {};
    uint32_t queueCount = 0;

    if(separatePresentQueue){
        uniqueQueueFamilies[0] = graphicsQueueFamily;
        uniqueQueueFamilies[1] = presentQueueFamily;
        queueCount = 2;
    }
    else{
        uniqueQueueFamilies[0] = graphicsQueueFamily;
        queueCount = 1;
    }

    float queuePriority = 1.0f;
    VkDeviceQueueCreateInfo queueCreateInfos[2] = {};
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

    error = vkCreateDevice(gpu, &createInfo, nullptr, &device);
    mp_assert(!error);

    vkGetDeviceQueue(device, graphicsQueueFamily, 0, &graphicsQueue);
    vkGetDeviceQueue(device, presentQueueFamily, 0, &presentQueue);

    // Vulkan :: command pool
    VkCommandPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = graphicsQueueFamily;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    error = vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool);
    mp_assert(!error);
}

void mpRenderer::LoadTextures(const char **paths, uint32_t count)
{
    texture.count = count;
    // Allocate memory for texture images & views
    texture.pImages = mpAllocate<VkImage>(mainAllocator, texture.count);
    texture.pImageViews = mpAllocate<VkImageView>(mainAllocator, texture.count);
    texture.pImageMemories = mpAllocate<VkDeviceMemory>(mainAllocator, texture.count);

    // Prepare images & views
    for(uint32_t i = 0; i < count; i++)
    {
        PrepareTextureImage(texture.pImages[i], texture.pImageMemories[i], paths[i]);
        texture.pImageViews[i] = CreateImageView(texture.pImages[i], VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT);
    }
    // Allocate memory for texture buffers
    texture.pVertexbuffers = mpAllocate<VkBuffer>(mainAllocator, texture.count);
    texture.pVertexbufferMemories = mpAllocate<VkDeviceMemory>(mainAllocator, texture.count);
    texture.pIndexbuffers = mpAllocate<VkBuffer>(mainAllocator, texture.count);
    texture.pIndexbufferMemories = mpAllocate<VkDeviceMemory>(mainAllocator, texture.count);
}

void mpRenderer::InitResources(mpCore &core)
{
    // Vulkan :: Descriptor set layouts
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

    VkResult error = vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &scene.descriptorSetLayout);
    mp_assert(!error);

    VkDescriptorSetLayoutBinding samplerLayoutBinding = {};
    samplerLayoutBinding.binding = 0;
    samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
    samplerLayoutBinding.descriptorCount = 1;
    samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    samplerLayoutBinding.pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutBinding textureArrayBinding = {};
    textureArrayBinding.binding = 1;
    textureArrayBinding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    textureArrayBinding.descriptorCount = texture.count;
    textureArrayBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    textureArrayBinding.pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutBinding guiSetLayoutBindings[] = {samplerLayoutBinding, textureArrayBinding};
    layoutInfo.pBindings = guiSetLayoutBindings;
    layoutInfo.bindingCount = 2;

    error = vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &gui.descriptorSetLayout);
    mp_assert(!error);

    // Vulkan :: query surface capabilities & formats
    uint32_t formatCount = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(gpu, surface, &formatCount, nullptr);
    mp_assert(formatCount > 0)
    VkSurfaceFormatKHR *pSurfaceFormats = mpAllocate<VkSurfaceFormatKHR>(tempAllocator, formatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(gpu, surface, &formatCount, pSurfaceFormats);

    surfaceFormat = pSurfaceFormats[0];
    for(uint32_t i = 0; i < formatCount; i++){
        if(pSurfaceFormats[i].format == VK_FORMAT_B8G8R8A8_SRGB && pSurfaceFormats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR){
            surfaceFormat = pSurfaceFormats[i];
            break;
        }
    }
    swapchainFormat = surfaceFormat.format;
    // Vulkan :: query present modes
    uint32_t presentModeCount = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(gpu, surface, &presentModeCount, nullptr);
    mp_assert(presentModeCount > 0)
    VkPresentModeKHR *pPresentModes = mpAllocate<VkPresentModeKHR>(tempAllocator, presentModeCount);
    vkGetPhysicalDeviceSurfacePresentModesKHR(gpu, surface, &presentModeCount, pPresentModes);
    /* NOTE:
        VK_PRESENT_MODE_IMMEDIATE_KHR   == Vsync OFF
        VK_PRESENT_MODE_FIFO_KHR        == Vsync ON, double buffering
        VK_PRESENT_MODE_MAILBOX_KHR     == Vsync ON, triple buffering
    */
    presentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
    for(uint32_t i = 0; i < presentModeCount; i++){
        if(pPresentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR){
            presentMode = pPresentModes[i];
            break;
        }
    }
    if(presentMode == VK_PRESENT_MODE_MAILBOX_KHR){
        MP_PUTS_INFO("Engine present mode: MAILBOX")
    }
    else if(presentMode == VK_PRESENT_MODE_FIFO_KHR){
        MP_PUTS_INFO("Engine present mode: FIFO")
    }
    else if(presentMode == VK_PRESENT_MODE_IMMEDIATE_KHR){
        MP_PUTS_INFO("Engine present mode: IMMEDIATE")
    }

    // Vulkan :: Initialise swapchain
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(gpu, surface, &surfaceCapabilities);
    if(surfaceCapabilities.currentExtent.width == 0xFFFFFFFF){
        VkExtent2D extent = {
            static_cast<uint32_t>(core.windowInfo.width),
            static_cast<uint32_t>(core.windowInfo.height)
        };
        swapchainExtent.width = uint32Clamp(extent.width, surfaceCapabilities.minImageExtent.width, surfaceCapabilities.maxImageExtent.width);
        swapchainExtent.height = uint32Clamp(extent.height, surfaceCapabilities.minImageExtent.height, surfaceCapabilities.maxImageExtent.height);
    }
    else{
        swapchainExtent = surfaceCapabilities.currentExtent;
    }

    uint32_t minImageCount = surfaceCapabilities.minImageCount + 1;
    if(surfaceCapabilities.maxImageCount > 0 && minImageCount > surfaceCapabilities.maxImageCount)
        minImageCount = surfaceCapabilities.maxImageCount;

    VkSwapchainCreateInfoKHR swapchainInfo = {};
    swapchainInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchainInfo.surface = surface;
    swapchainInfo.minImageCount = minImageCount;
    swapchainInfo.imageFormat = surfaceFormat.format;
    swapchainInfo.imageColorSpace = surfaceFormat.colorSpace;
    swapchainInfo.imageExtent = swapchainExtent;
    swapchainInfo.imageArrayLayers = 1;
    swapchainInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    uint32_t queueFamilyIndices[] = {graphicsQueueFamily, presentQueueFamily};

    if(separatePresentQueue){
        swapchainInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        swapchainInfo.queueFamilyIndexCount = 2;
        swapchainInfo.pQueueFamilyIndices = queueFamilyIndices;
    }
    else{
        swapchainInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        swapchainInfo.queueFamilyIndexCount = 0;
        swapchainInfo.pQueueFamilyIndices = nullptr;
    }

    swapchainInfo.preTransform = surfaceCapabilities.currentTransform;
    swapchainInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchainInfo.presentMode = presentMode;
    swapchainInfo.clipped = VK_TRUE;
    swapchainInfo.oldSwapchain = VK_NULL_HANDLE;

    error = vkCreateSwapchainKHR(device, &swapchainInfo, nullptr, &swapchain);
    mp_assert(!error)

    vkGetSwapchainImagesKHR(device, swapchain, &swapchainImageCount, nullptr);
    pSwapchainImages = mpAllocate<VkImage>(mainAllocator, swapchainImageCount);
    vkGetSwapchainImagesKHR(device, swapchain, &swapchainImageCount, pSwapchainImages);

    pSwapchainImageViews = mpAllocate<VkImageView>(mainAllocator, swapchainImageCount);
    for(uint32_t i = 0; i < swapchainImageCount; i++)
        pSwapchainImageViews[i] = CreateImageView(pSwapchainImages[i], swapchainFormat, VK_IMAGE_ASPECT_COLOR_BIT);

    // Vulkan :: renderpass
    PrepareRenderPass();

    // Vulkan :: shader modules
    LoadShaders(core.callbacks);

    // Vulkan :: pipelines
    PrepareScenePipeline();
    PrepareGuiPipeline();

    // Vulkan :: Allocate dynamic arrays
    pFramebuffers = mpAllocate<VkFramebuffer>(mainAllocator, swapchainImageCount);
    pUniformbuffers = mpAllocate<VkBuffer>(mainAllocator, swapchainImageCount);
    pUniformbufferMemories = mpAllocate<VkDeviceMemory>(mainAllocator, swapchainImageCount);
    pCommandbuffers = mpAllocate<VkCommandBuffer>(mainAllocator, swapchainImageCount);
    pInFlightImageFences = mpAllocate<VkFence>(mainAllocator, swapchainImageCount);
    scene.pDescriptorSets = mpAllocate<VkDescriptorSet>(mainAllocator, swapchainImageCount);
    gui.pDescriptorSets = mpAllocate<VkDescriptorSet>(mainAllocator, swapchainImageCount);

    scene.vertexbuffers = mpAllocate<VkBuffer>(mainAllocator, core.meshRegistry.meshArray.count);
    scene.vertexbufferMemories = mpAllocate<VkDeviceMemory>(mainAllocator, core.meshRegistry.meshArray.count);
    scene.indexbuffers = mpAllocate<VkBuffer>(mainAllocator, core.meshRegistry.meshArray.count);
    scene.indexbufferMemories = mpAllocate<VkDeviceMemory>(mainAllocator, core.meshRegistry.meshArray.count);
    scene.bufferCount = core.meshRegistry.meshArray.count;

    // Vulkan :: depth resources
    PrepareDepthResources();
    // Vulkan :: framebuffers
    PrepareFramebuffers();
    // Vulkan :: scenebuffers
    PrepareScenebuffers(core.meshRegistry.meshArray);

    // Vulkan :: sampler
    VkSamplerCreateInfo samplerInfo = {};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.anisotropyEnable = VK_FALSE;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_TRANSPARENT_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

    error = vkCreateSampler(device, &samplerInfo, nullptr, &texture.sampler);
    mp_assert(!error);

    // Vulkan :: uniformbuffers
    PrepareUniformbuffers();
    // Vulkan :: descriptor pool
    PrepareDescriptorPool();
    // Vulkan :: scene descriptor sets
    PrepareSceneDescriptorSets();
    // Vulkan :: gui descriptor sets
    PrepareGuiDescriptorSets();
    // Vulkan :: commandbuffers
    PrepareCommandbuffers();
    // Vulkan :: syncobjects
    PrepareSyncObjects();

    // Model matrix never changes but needs to be initalised at least once
    ubo.model = Mat4x4Identity();

    MP_PUTS_TRACE("Vulkan initialised")
}
// Helper function
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
// Helper function
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

void mpRenderer::RecreateGuiBuffers(mpGUI &mpgui)
{
    for(uint32_t i = 0; i < texture.count; i++)
    {
        mpGuiMesh &mesh = mpgui.meshes[i];
        if(mesh.vertexCount == 0)
            continue;

        constexpr VkBufferUsageFlags vertUsageDstFlags = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        constexpr VkBufferUsageFlags iUsageDstFlags = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;

        vkDeviceWaitIdle(device);
        vkDestroyBuffer(device, texture.pVertexbuffers[i], nullptr);
        vkDestroyBuffer(device, texture.pIndexbuffers[i], nullptr);
        vkFreeMemory(device, texture.pVertexbufferMemories[i], nullptr);
        vkFreeMemory(device, texture.pIndexbufferMemories[i], nullptr);

        VkDeviceSize bufferSize = mesh.vertexCount * sizeof(mpVertex);
        void *rawBuffer = mesh.vertices;
        VkBuffer *buffer = &texture.pVertexbuffers[i];
        VkDeviceMemory *bufferMemory = &texture.pVertexbufferMemories[i];
        VkBufferUsageFlags usage = vertUsageDstFlags;
        mapBufferData(rawBuffer, bufferSize, buffer, bufferMemory, usage);

        bufferSize = mesh.indexCount * sizeof(uint16_t);
        rawBuffer = mesh.indices;
        buffer = &texture.pIndexbuffers[i];
        bufferMemory = &texture.pIndexbufferMemories[i];
        usage = iUsageDstFlags;
        mapBufferData(rawBuffer, bufferSize, buffer, bufferMemory, usage);
    }
}

void mpRenderer::RecreateSceneBuffer(mpMesh &mesh, uint32_t meshID)
{
    if(mesh.vertexCount == 0)
        return;

    constexpr VkBufferUsageFlags vertUsageDstFlags = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    constexpr VkBufferUsageFlags indexUsageDstFlags = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;

    vkDeviceWaitIdle(device);
    vkDestroyBuffer(device, scene.vertexbuffers[meshID], nullptr);
    vkDestroyBuffer(device, scene.indexbuffers[meshID], nullptr);
    vkFreeMemory(device, scene.vertexbufferMemories[meshID], nullptr);
    vkFreeMemory(device, scene.indexbufferMemories[meshID], nullptr);

    VkDeviceSize bufferSize = mesh.vertexCount * sizeof(mpVertex);
    void *rawBuffer = mesh.vertices;
    VkBuffer *buffer = &scene.vertexbuffers[meshID];
    VkDeviceMemory *bufferMemory = &scene.vertexbufferMemories[meshID];
    VkBufferUsageFlags usage = vertUsageDstFlags;
    mapBufferData(rawBuffer, bufferSize, buffer, bufferMemory, usage);

    bufferSize = mesh.indexCount * sizeof(uint16_t);
    rawBuffer = mesh.indices;
    buffer = &scene.indexbuffers[meshID];
    bufferMemory = &scene.indexbufferMemories[meshID];
    usage = indexUsageDstFlags;
    mapBufferData(rawBuffer, bufferSize, buffer, bufferMemory, usage);
}

void mpRenderer::Update(mpCore &core)
{
    if(core.windowInfo.width == 0 || core.windowInfo.height == 0)
        return;

    vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64MAX);
    uint32_t imageIndex = 0;
    VkResult result = vkAcquireNextImageKHR(device, swapchain, UINT64MAX, imageAvailableSPs[currentFrame], VK_NULL_HANDLE, &imageIndex);
    if(result == VK_ERROR_OUT_OF_DATE_KHR){
        ReCreateSwapchain(core.windowInfo.width, core.windowInfo.height);
        return;
    }
    else if(result == VK_SUBOPTIMAL_KHR){
        MP_PUTS_WARN("Suboptimal swapchain")
    }
    // Update UBOs
    ubo.view = core.camera.view;
    ubo.proj = core.camera.projection;
    ubo.lightPosition = core.pointLight.position;
    ubo.lightConstant = core.pointLight.constant;
    ubo.lightLinear = core.pointLight.linear;
    ubo.lightQuadratic = core.pointLight.quadratic;
    ubo.lightDiffuse = core.pointLight.diffuse;
    ubo.lightAmbient = core.pointLight.ambient;

    // Remap ubo buffers
    void *data;
    vkMapMemory(device, pUniformbufferMemories[imageIndex], 0, sizeof(mpUniformbufferObject), 0, &data);
    memcpy(data, &ubo, sizeof(mpUniformbufferObject));
    vkUnmapMemory(device, pUniformbufferMemories[imageIndex]);

    // Issue new draw commands
    DrawFrame(core.meshRegistry.meshArray, core.gui, imageIndex);

    // Check if the previous frame is using this image; it might have a fence to wait on
    if(pInFlightImageFences[imageIndex] != VK_NULL_HANDLE)
        vkWaitForFences(device, 1, &pInFlightImageFences[imageIndex], VK_TRUE, UINT64MAX);

    // Mark the imageIndex image as being in use by this frame
    pInFlightImageFences[imageIndex] = inFlightFences[currentFrame];

    // Submit
    const VkSemaphore waitSemaphores[] = {imageAvailableSPs[currentFrame]};
    const VkSemaphore signalSemaphores[] = {renderFinishedSPs[currentFrame]};
    const VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &pCommandbuffers[imageIndex];
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    vkResetFences(device, 1, &inFlightFences[currentFrame]);
    VkResult error = vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]);
    mp_assert(!error);

    const VkSwapchainKHR swapchains[] = {swapchain};
    // Present to swapchain
    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapchains;
    presentInfo.pImageIndices = &imageIndex;

    result = vkQueuePresentKHR(presentQueue, &presentInfo);
    if(result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || core.windowInfo.hasResized)
        ReCreateSwapchain(core.windowInfo.width, core.windowInfo.height);

    currentFrame = (currentFrame + 1) % MAX_IMAGES_IN_FLIGHT;
}

void mpRenderer::Cleanup()
{
    vkDeviceWaitIdle(device);
    CleanUpSwapchain();

    vkDestroyShaderModule(device, scene.vertShaderModule, nullptr);
    vkDestroyShaderModule(device, scene.fragShaderModule, nullptr);
    vkDestroyShaderModule(device, gui.vertShaderModule, nullptr);
    vkDestroyShaderModule(device, gui.fragShaderModule, nullptr);

    vkDestroyDescriptorSetLayout(device, gui.descriptorSetLayout, nullptr);
    vkDestroyDescriptorSetLayout(device, scene.descriptorSetLayout, nullptr);

    vkDestroySampler(device, texture.sampler, nullptr);
    for(uint32_t i = 0; i < texture.count; i++){
        vkDestroyImageView(device, texture.pImageViews[i], nullptr);
        vkDestroyImage(device, texture.pImages[i], nullptr);
        vkFreeMemory(device, texture.pImageMemories[i], nullptr);

        vkDestroyBuffer(device, texture.pVertexbuffers[i], nullptr);
        vkFreeMemory(device, texture.pVertexbufferMemories[i], nullptr);
        vkDestroyBuffer(device, texture.pIndexbuffers[i], nullptr);
        vkFreeMemory(device, texture.pIndexbufferMemories[i], nullptr);
    }
    for(uint32_t meshID = 0; meshID < scene.bufferCount; meshID++){
        vkDestroyBuffer(device, scene.indexbuffers[meshID], nullptr);
        vkFreeMemory(device, scene.indexbufferMemories[meshID], nullptr);
        vkDestroyBuffer(device, scene.vertexbuffers[meshID], nullptr);
        vkFreeMemory(device, scene.vertexbufferMemories[meshID], nullptr);
    }
    for(uint32_t i = 0; i < MAX_IMAGES_IN_FLIGHT; i++) {
        vkDestroySemaphore(device, imageAvailableSPs[i], nullptr);
        vkDestroySemaphore(device, renderFinishedSPs[i], nullptr);
        vkDestroyFence(device, inFlightFences[i], nullptr);
    }
    vkDestroyCommandPool(device, commandPool, nullptr);
    vkDestroyDevice(device, nullptr);
    vkDestroySurfaceKHR(instance, surface, nullptr);
    vkDestroyInstance(instance, nullptr);
}

// private methods
void mpRenderer::LoadShaderModule(const mpCallbacks &callbacks, VkShaderModule *pModule, const char *path)
{
    mpFile shader = callbacks.mpReadFile(path);
    VkShaderModuleCreateInfo shaderModuleInfo = {};
    shaderModuleInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shaderModuleInfo.codeSize = shader.size;
    shaderModuleInfo.pCode = (const uint32_t*)shader.handle;
    VkResult error = vkCreateShaderModule(device, &shaderModuleInfo, nullptr, pModule);
    mp_assert(!error);
    callbacks.mpFreeFileMemory(&shader);
}

void mpRenderer::PrepareTextureImage(VkImage &image, VkDeviceMemory &imageMemory, const char *filePath)
{
    int32_t texWidth, texHeight, texChannels;
    stbi_uc *pixels = stbi_load(filePath, &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

    VkDeviceSize imageSize = texWidth * texHeight * 4;
    if(!pixels) {
        MP_LOG_WARN("Failed to load image from file %s\n", filePath);
        mp_assert(false)
    }
    VkBuffer stagingbuffer;
    VkDeviceMemory stagingbufferMemory;
    VkBufferUsageFlags usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    VkMemoryPropertyFlags propertyFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    CreateBuffer(imageSize, usage, propertyFlags, stagingbuffer, stagingbufferMemory);

    void *data;
    vkMapMemory(device, stagingbufferMemory, 0, imageSize, 0, &data);
    memcpy(data, pixels, static_cast<size_t>(imageSize));
    vkUnmapMemory(device, stagingbufferMemory);
    stbi_image_free(pixels);

    VkExtent2D imageExtent = {static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight)};
    constexpr VkImageUsageFlags imageUsage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    image = CreateImage(imageExtent, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL, imageUsage);
    BindImageMemory(image, imageMemory, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    TransitionImageLayout(image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    // Copy buffer to image begin
    VkCommandBuffer commandBuffer = BeginSingleTimeCommands(device, commandPool);
    VkBufferImageCopy region = {};
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = {0, 0, 0};
    region.imageExtent = {static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight), 1};
    vkCmdCopyBufferToImage(commandBuffer, stagingbuffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
    EndSingleTimeCommands(device, commandPool, graphicsQueue, commandBuffer);
    // Copy buffer to image end
    TransitionImageLayout(image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    vkDestroyBuffer(device, stagingbuffer, nullptr);
    vkFreeMemory(device, stagingbufferMemory, nullptr);
}

void mpRenderer::TransitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout)
{
    VkCommandBuffer commandBuffer = BeginSingleTimeCommands(device, commandPool);
    VkImageMemoryBarrier barrier = {};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    VkPipelineStageFlags sourceStage = 0;
    VkPipelineStageFlags destStage = 0;
    if(oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL){
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    else if(oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL){
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    else{
        MP_LOG_ERROR("RENDERER ERROR: Unsupported layout transition")
    }
    vkCmdPipelineBarrier(commandBuffer, sourceStage, destStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
    EndSingleTimeCommands(device, commandPool, graphicsQueue, commandBuffer);
}

bool32 mpRenderer::CheckPhysicalDeviceSuitability(VkPhysicalDevice &checkedPhysDevice)
{
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(checkedPhysDevice, &queueFamilyCount, nullptr);
    VkQueueFamilyProperties* queueFamilyProperties = mpAllocate<VkQueueFamilyProperties>(tempAllocator, queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(checkedPhysDevice, &queueFamilyCount, queueFamilyProperties);

    bool32 hasGraphicsFamily = false, hasPresentFamily = false;
    for(uint32_t i = 0; i < queueFamilyCount; i++){
        if(queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT){
            graphicsQueueFamily = i;
            hasGraphicsFamily = true;
        }

        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(checkedPhysDevice, i, surface, &presentSupport);
        if(presentSupport){
            presentQueueFamily = i;
            hasPresentFamily = true;
        }
    }

    uint32_t extensionCount = 0;
    vkEnumerateDeviceExtensionProperties(checkedPhysDevice, nullptr, &extensionCount, nullptr);
    VkExtensionProperties* availableExtensions = mpAllocate<VkExtensionProperties>(tempAllocator, extensionCount);
    vkEnumerateDeviceExtensionProperties(checkedPhysDevice, nullptr, &extensionCount, availableExtensions);

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
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(checkedPhysDevice, surface, &capabilities);
        uint32_t formatCount = 0, presentModeCount = 0;
        vkGetPhysicalDeviceSurfaceFormatsKHR(checkedPhysDevice, surface, &formatCount, nullptr);
        vkGetPhysicalDeviceSurfacePresentModesKHR(checkedPhysDevice, surface, &presentModeCount, nullptr);

        if((formatCount > 0) && (presentModeCount > 0))
            swapChainAdequate = true;
    }
    return hasGraphicsFamily && hasPresentFamily && extensionsSupported && swapChainAdequate;
}

VkImageView mpRenderer::CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags)
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

void mpRenderer::PrepareRenderPass()
{
    VkAttachmentDescription colorAttachment = {};
    colorAttachment.format = swapchainFormat;
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
    depthAttachment.format = depth.format;
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

    VkResult error = vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass);
    mp_assert(!error);
}
// Pipeline creation :: helper function
inline static VkPipelineShaderStageCreateInfo getShaderStageCreateInfo(VkShaderStageFlagBits stage, VkShaderModule &module)
{
    VkPipelineShaderStageCreateInfo result = {};
    result.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    result.stage = stage;
    result.module = module;
    result.pName = "main";
    return result;
}
// Pipeline creation :: helper function
inline static VkPipelineInputAssemblyStateCreateInfo getInputAssembly()
{
    VkPipelineInputAssemblyStateCreateInfo result = {};
    result.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    result.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    result.primitiveRestartEnable = VK_FALSE;
    return result;
}
// Pipeline creation :: helper function
inline static VkViewport getViewport(VkExtent2D extent)
{
    VkViewport result = {};
    result.width = static_cast<float>(extent.width);
    result.height = static_cast<float>(extent.height);
    result.minDepth = 0.0f;
    result.maxDepth = 1.0f;
    return result;
}
// Pipeline creation :: helper function
inline static VkRect2D getScissor(VkExtent2D extent)
{
    VkRect2D result = {};
    result.offset = { 0, 0 };
    result.extent = extent;
    return result;
}
// Pipeline creation :: helper function
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
// Pipeline creation :: helper function
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

void mpRenderer::PrepareScenePipeline()
{
    VkPipelineShaderStageCreateInfo vertShaderStageInfo = getShaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, scene.vertShaderModule);
    VkPipelineShaderStageCreateInfo fragShaderStageInfo = getShaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, scene.fragShaderModule);
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

    VkViewport viewport = getViewport(swapchainExtent);
    VkRect2D scissor = getScissor(swapchainExtent);

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
    pipelineLayoutInfo.pSetLayouts = &scene.descriptorSetLayout;
    pipelineLayoutInfo.setLayoutCount = 1;

    VkResult error = vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &scene.pipelineLayout);
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
    pipelineInfo.layout = scene.pipelineLayout;
    pipelineInfo.renderPass = renderPass;

    error = vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &scene.pipeline);
    mp_assert(!error);
}

void mpRenderer::PrepareGuiPipeline()
{
    VkPipelineShaderStageCreateInfo vertShaderStageInfo = getShaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, gui.vertShaderModule);
    VkPipelineShaderStageCreateInfo fragShaderStageInfo = getShaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, gui.fragShaderModule);
    VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };
    constexpr uint32_t shaderStagesCount = arraysize(shaderStages);

    VkVertexInputBindingDescription bindingDescription = {};
    bindingDescription.binding = 0;
    bindingDescription.stride = sizeof(mpGuiVertex);
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    VkVertexInputAttributeDescription attributeDescs[3] = {};
    attributeDescs[0].binding = 0;
    attributeDescs[0].location = 0;
    attributeDescs[0].format = VK_FORMAT_R32G32_SFLOAT;
    attributeDescs[0].offset = offsetof(mpGuiVertex, position);

    attributeDescs[1].binding = 0;
    attributeDescs[1].location = 1;
    attributeDescs[1].format = VK_FORMAT_R32G32_SFLOAT;
    attributeDescs[1].offset = offsetof(mpGuiVertex, texCoord);

    attributeDescs[2].binding = 0;
    attributeDescs[2].location = 2;
    attributeDescs[2].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    attributeDescs[2].offset = offsetof(mpGuiVertex, colour);

    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputInfo.vertexAttributeDescriptionCount = arraysize(attributeDescs);
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescs;

    VkPipelineInputAssemblyStateCreateInfo inputAssembly = getInputAssembly();

    VkViewport viewport = getViewport(swapchainExtent);
    VkRect2D scissor = getScissor(swapchainExtent);

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

    VkPushConstantRange pushConstantRange = {};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    pushConstantRange.size = sizeof(int32_t);

    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &gui.descriptorSetLayout;
    pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
    pipelineLayoutInfo.pushConstantRangeCount = 1;

    VkResult error = vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &gui.pipelineLayout);
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
    pipelineInfo.layout = gui.pipelineLayout;
    pipelineInfo.renderPass = renderPass;

    error = vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &gui.pipeline);
    mp_assert(!error);
}

inline static uint32_t FindMemoryType(VkPhysicalDevice physDevice, uint32_t typeFilter, VkMemoryPropertyFlags propertyFlags)
{
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(physDevice, &memProperties);

    for(uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
        if(typeFilter & (1 << i) && (memProperties.memoryTypes[i].propertyFlags & propertyFlags) == propertyFlags)
            return i;

    MP_PUTS_ERROR("Failed to find suitable memory type!");
    return 0;
}

VkImage mpRenderer::CreateImage(VkExtent2D extent, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage)
{
    VkImageCreateInfo imageInfo = {};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent = VkExtent3D{extent.width, extent.height, 1};
    imageInfo.format = format;
    imageInfo.tiling = tiling;
    imageInfo.usage = usage;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    VkImage image;
    VkResult error = vkCreateImage(device, &imageInfo, nullptr, &image);
    mp_assert(!error);
    return image;
}

void mpRenderer::BindImageMemory(VkImage &image, VkDeviceMemory &imageMemory, VkMemoryPropertyFlags memProps)
{
    VkMemoryRequirements memReqs;
    vkGetImageMemoryRequirements(device, image, &memReqs);
    VkMemoryAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memReqs.size;
    allocInfo.memoryTypeIndex = FindMemoryType(gpu, memReqs.memoryTypeBits, memProps);

    VkResult error = vkAllocateMemory(device, &allocInfo, nullptr, &imageMemory);
    mp_assert(!error);
    vkBindImageMemory(device, image, imageMemory, 0);
}

void mpRenderer::PrepareDepthResources()
{
    depth.image = CreateImage(swapchainExtent, depth.format, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);
    BindImageMemory(depth.image, depth.imageMemory, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    depth.imageView = CreateImageView(depth.image, depth.format, VK_IMAGE_ASPECT_DEPTH_BIT);
}

void mpRenderer::PrepareFramebuffers()
{
    for(uint32_t i = 0; i < swapchainImageCount; i++){
        VkImageView attachments[] = {pSwapchainImageViews[i], depth.imageView};
        constexpr uint32_t attachmentsSize = arraysize(attachments);

        VkFramebufferCreateInfo framebufferInfo = {};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = renderPass;
        framebufferInfo.attachmentCount = attachmentsSize;
        framebufferInfo.pAttachments = attachments;
        framebufferInfo.width = swapchainExtent.width;
        framebufferInfo.height = swapchainExtent.height;
        framebufferInfo.layers = 1;

        VkResult error = vkCreateFramebuffer(device, &framebufferInfo, nullptr, &pFramebuffers[i]);
        mp_assert(!error);
    }
}

void mpRenderer::CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags propertyFlags, VkBuffer &buffer, VkDeviceMemory &bufferMemory)
{
    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VkResult error = vkCreateBuffer(device, &bufferInfo, nullptr, &buffer);
    mp_assert(!error);

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = FindMemoryType(gpu, memRequirements.memoryTypeBits, propertyFlags);

    error = vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory);
    mp_assert(!error);

    vkBindBufferMemory(device, buffer, bufferMemory, 0);
}

void mpRenderer::mapBufferData(void *rawBuffer, VkDeviceSize bufferSize, VkBuffer *buffer, VkDeviceMemory *bufferMemory, VkBufferUsageFlags usage)
{
    constexpr uint32_t bufferSrcFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    VkBuffer stagingbuffer;
    VkDeviceMemory stagingbufferMemory;
    void *vertData;
    CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, bufferSrcFlags, stagingbuffer, stagingbufferMemory);
    vkMapMemory(device, stagingbufferMemory, 0, bufferSize, 0, &vertData);
    memcpy(vertData, rawBuffer, static_cast<size_t>(bufferSize));
    vkUnmapMemory(device, stagingbufferMemory);
    CreateBuffer(bufferSize, usage, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, *buffer, *bufferMemory);

    VkCommandBuffer cmdBuffer = BeginSingleTimeCommands(device, commandPool);
    VkBufferCopy copyRegion = {};
    copyRegion.size = bufferSize;
    vkCmdCopyBuffer(cmdBuffer, stagingbuffer, *buffer, 1, &copyRegion);
    EndSingleTimeCommands(device, commandPool, graphicsQueue, cmdBuffer);

    vkDestroyBuffer(device, stagingbuffer, nullptr);
    vkFreeMemory(device, stagingbufferMemory, nullptr);
}

void mpRenderer::PrepareScenebuffers(const mpMeshArray &meshArray)
{
    constexpr VkBufferUsageFlags vertUsageDstFlags = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    constexpr VkBufferUsageFlags indexUsageDstFlags = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;

    for(uint32_t meshID = 0; meshID < scene.bufferCount; meshID++){
        const mpMesh &mesh = meshArray.data[meshID];
        if(mesh.vertexCount == 0)
            continue;

        VkDeviceSize bufferSize = mesh.vertexCount * sizeof(mpVertex);
        void *rawBuffer = mesh.vertices;
        VkBuffer *buffer = &scene.vertexbuffers[meshID];
        VkDeviceMemory *bufferMemory = &scene.vertexbufferMemories[meshID];
        VkBufferUsageFlags usage = vertUsageDstFlags;
        mapBufferData(rawBuffer, bufferSize, buffer, bufferMemory, usage);

        bufferSize = mesh.indexCount * sizeof(uint16_t);
        rawBuffer = mesh.indices;
        buffer = &scene.indexbuffers[meshID];
        bufferMemory = &scene.indexbufferMemories[meshID];
        usage = indexUsageDstFlags;
        mapBufferData(rawBuffer, bufferSize, buffer, bufferMemory, usage);
    }
}

void mpRenderer::PrepareUniformbuffers()
{
    constexpr VkDeviceSize bufferSize = sizeof(mpUniformbufferObject);
    const uint32_t props = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

    for(uint32_t i = 0; i < swapchainImageCount; i++)
        CreateBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, props, pUniformbuffers[i], pUniformbufferMemories[i]);
}

void mpRenderer::PrepareDescriptorPool()
{
    VkDescriptorPoolSize scenePoolSize = {};
    scenePoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    scenePoolSize.descriptorCount = swapchainImageCount;
    VkDescriptorPoolSize guiPoolSize = {};
    guiPoolSize.type = VK_DESCRIPTOR_TYPE_SAMPLER;
    guiPoolSize.descriptorCount = swapchainImageCount;
    VkDescriptorPoolSize texturesPoolSize = {};
    texturesPoolSize.type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    texturesPoolSize.descriptorCount = texture.count * swapchainImageCount;

    VkDescriptorPoolSize poolSizes[] = {scenePoolSize, guiPoolSize, texturesPoolSize};
    constexpr uint32_t poolSizeCount = arraysize(poolSizes);

    uint32_t maxSets = 0;
    for(uint32_t i = 0; i < poolSizeCount; i++)
        maxSets += poolSizes[i].descriptorCount;

    VkDescriptorPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = poolSizeCount;
    poolInfo.pPoolSizes = poolSizes;
    poolInfo.maxSets = maxSets;

    VkResult error = vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool);
    mp_assert(!error);
}

void mpRenderer::PrepareSceneDescriptorSets()
{
    VkDescriptorSetLayout *pLayouts = mpAllocate<VkDescriptorSetLayout>(tempAllocator, swapchainImageCount);
    for(uint32_t i = 0; i < swapchainImageCount; i++)
        pLayouts[i] = scene.descriptorSetLayout;

    VkDescriptorSetAllocateInfo descriptorSetAllocInfo = {};
    descriptorSetAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    descriptorSetAllocInfo.descriptorPool = descriptorPool;
    descriptorSetAllocInfo.descriptorSetCount = swapchainImageCount;
    descriptorSetAllocInfo.pSetLayouts = pLayouts;

    VkResult error = vkAllocateDescriptorSets(device, &descriptorSetAllocInfo, scene.pDescriptorSets);
    mp_assert(!error);

    for(uint32_t i = 0; i < swapchainImageCount; i++){
        VkDescriptorBufferInfo bufferInfo = {};
        bufferInfo.buffer = pUniformbuffers[i];
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(mpUniformbufferObject);

        VkWriteDescriptorSet descriptorWrite = {};
        descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.dstSet = scene.pDescriptorSets[i];
        descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrite.descriptorCount = 1;
        descriptorWrite.pBufferInfo = &bufferInfo;

        vkUpdateDescriptorSets(device, 1, &descriptorWrite, 0, nullptr);
    }
}

void mpRenderer::PrepareGuiDescriptorSets()
{
    VkDescriptorSetLayout *pLayouts = mpAllocate<VkDescriptorSetLayout>(tempAllocator, swapchainImageCount);
    for(uint32_t i = 0; i < swapchainImageCount; i++)
        pLayouts[i] = gui.descriptorSetLayout;

    VkDescriptorSetAllocateInfo descriptorSetAllocInfo = {};
    descriptorSetAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    descriptorSetAllocInfo.descriptorPool = descriptorPool;
    descriptorSetAllocInfo.descriptorSetCount = swapchainImageCount;
    descriptorSetAllocInfo.pSetLayouts = pLayouts;

    VkResult error = vkAllocateDescriptorSets(device, &descriptorSetAllocInfo, gui.pDescriptorSets);
    mp_assert(!error);

    // Per texture
    VkDescriptorImageInfo *imageInfos = mpAllocate<VkDescriptorImageInfo>(tempAllocator, texture.count);
    for(uint32_t i = 0; i < texture.count; i++){
        imageInfos[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfos[i].sampler = texture.sampler;
        imageInfos[i].imageView = texture.pImageViews[i];
    }
    // Pre swapchain image
    for(uint32_t i = 0; i < swapchainImageCount; i++){
        VkDescriptorImageInfo samplerInfo = {};
        samplerInfo.sampler = texture.sampler;

        VkWriteDescriptorSet samplerWrite = {};
        samplerWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        samplerWrite.dstSet = gui.pDescriptorSets[i];
        samplerWrite.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
        samplerWrite.descriptorCount = 1;
        samplerWrite.pImageInfo = &samplerInfo;
        samplerWrite.dstBinding = 0;

        VkWriteDescriptorSet textureArrayWrite = {};
        textureArrayWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        textureArrayWrite.dstSet = gui.pDescriptorSets[i];
        textureArrayWrite.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        textureArrayWrite.descriptorCount = texture.count;
        textureArrayWrite.pImageInfo = imageInfos;
        textureArrayWrite.dstBinding = 1;

        VkWriteDescriptorSet descriptorWrites[] = {samplerWrite, textureArrayWrite};
        constexpr uint32_t descriptorWriteCount = arraysize(descriptorWrites);
        vkUpdateDescriptorSets(device, descriptorWriteCount, descriptorWrites, 0, nullptr);
    }
}

void mpRenderer::PrepareCommandbuffers()
{
    VkCommandBufferAllocateInfo cmdBufferAllocInfo = {};
    cmdBufferAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmdBufferAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmdBufferAllocInfo.commandBufferCount = swapchainImageCount;
    cmdBufferAllocInfo.commandPool = commandPool;

    VkResult error = vkAllocateCommandBuffers(device, &cmdBufferAllocInfo, pCommandbuffers);
    mp_assert(!error);
}

void mpRenderer::PrepareSyncObjects()
{
    VkSemaphoreCreateInfo semaphoreInfo = {};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo = {};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for(uint32_t i = 0; i < MAX_IMAGES_IN_FLIGHT; i++){
        VkResult error = vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSPs[i]);
        mp_assert(!error);
        error = vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSPs[i]);
        mp_assert(!error);
        error = vkCreateFence(device, &fenceInfo, nullptr, &inFlightFences[i]);
        mp_assert(!error);
    }
}

void mpRenderer::CleanUpSwapchain()
{
    vkDestroyImageView(device, depth.imageView, nullptr);
    vkDestroyImage(device, depth.image, nullptr);
    vkFreeMemory(device, depth.imageMemory, nullptr);

    for(uint32_t i = 0; i < swapchainImageCount; i++)
        vkDestroyFramebuffer(device, pFramebuffers[i], nullptr);

    vkFreeCommandBuffers(device, commandPool, swapchainImageCount, pCommandbuffers);
    vkDestroyPipeline(device, scene.pipeline, nullptr);
    vkDestroyPipelineLayout(device, scene.pipelineLayout, nullptr);
    vkDestroyPipeline(device, gui.pipeline, nullptr);
    vkDestroyPipelineLayout(device, gui.pipelineLayout, nullptr);
    vkDestroyRenderPass(device, renderPass, nullptr);

    for(uint32_t i = 0; i < swapchainImageCount; i++){
        vkDestroyImageView(device, pSwapchainImageViews[i], nullptr);
        vkDestroyBuffer(device, pUniformbuffers[i], nullptr);
        vkFreeMemory(device, pUniformbufferMemories[i], nullptr);
    }
    vkDestroySwapchainKHR(device, swapchain, nullptr);
    vkDestroyDescriptorPool(device, descriptorPool, nullptr);
}

void mpRenderer::ReCreateSwapchain(int32_t width, int32_t height)
{
    vkDeviceWaitIdle(device);
    CleanUpSwapchain();

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(gpu, surface, &surfaceCapabilities);
    if(surfaceCapabilities.currentExtent.width == 0xFFFFFFFF){
        VkExtent2D extent = {
            static_cast<uint32_t>(width),
            static_cast<uint32_t>(height)
        };
        swapchainExtent.width = uint32Clamp(extent.width, surfaceCapabilities.minImageExtent.width, surfaceCapabilities.maxImageExtent.width);
        swapchainExtent.height = uint32Clamp(extent.height, surfaceCapabilities.minImageExtent.height, surfaceCapabilities.maxImageExtent.height);
    }
    else{
        swapchainExtent = surfaceCapabilities.currentExtent;
    }

    uint32_t minImageCount = surfaceCapabilities.minImageCount + 1;
    if(surfaceCapabilities.maxImageCount > 0 && minImageCount > surfaceCapabilities.maxImageCount)
        minImageCount = surfaceCapabilities.maxImageCount;

    VkSwapchainCreateInfoKHR swapchainInfo = {};
    swapchainInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchainInfo.surface = surface;
    swapchainInfo.minImageCount = minImageCount;
    swapchainInfo.imageFormat = surfaceFormat.format;
    swapchainInfo.imageColorSpace = surfaceFormat.colorSpace;
    swapchainInfo.imageExtent = swapchainExtent;
    swapchainInfo.imageArrayLayers = 1;
    swapchainInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    uint32_t queueFamilyIndices[] = {graphicsQueueFamily, presentQueueFamily};

    if(separatePresentQueue){
        swapchainInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        swapchainInfo.queueFamilyIndexCount = 2;
        swapchainInfo.pQueueFamilyIndices = queueFamilyIndices;
    }
    else{
        swapchainInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        swapchainInfo.queueFamilyIndexCount = 0;
        swapchainInfo.pQueueFamilyIndices = nullptr;
    }

    swapchainInfo.preTransform = surfaceCapabilities.currentTransform;
    swapchainInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchainInfo.presentMode = presentMode;
    swapchainInfo.clipped = VK_TRUE;
    swapchainInfo.oldSwapchain = VK_NULL_HANDLE;

    VkResult error = vkCreateSwapchainKHR(device, &swapchainInfo, nullptr, &swapchain);
    mp_assert(!error)

    vkGetSwapchainImagesKHR(device, swapchain, &swapchainImageCount, nullptr);
    vkGetSwapchainImagesKHR(device, swapchain, &swapchainImageCount, pSwapchainImages);

    for(uint32_t i = 0; i < swapchainImageCount; i++)
        pSwapchainImageViews[i] = CreateImageView(pSwapchainImages[i], swapchainFormat, VK_IMAGE_ASPECT_COLOR_BIT);

    PrepareRenderPass();
    PrepareScenePipeline();
    PrepareGuiPipeline();
    PrepareDepthResources();
    PrepareFramebuffers();
    PrepareUniformbuffers();
    PrepareDescriptorPool();
    PrepareSceneDescriptorSets();
    PrepareGuiDescriptorSets();
    PrepareCommandbuffers();
}

void mpRenderer::DrawFrame(const mpMeshArray &meshArray, const mpGUI &mpgui, uint32_t imageIndex)
{
    vkDeviceWaitIdle(device);
    VkCommandBuffer &commandbuffer = pCommandbuffers[imageIndex];

    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
    VkResult error = vkBeginCommandBuffer(commandbuffer, &beginInfo);
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
    renderPassInfo.framebuffer = pFramebuffers[imageIndex];
    renderPassInfo.renderArea.offset = { 0, 0 };
    renderPassInfo.renderArea.extent = swapchainExtent;
    renderPassInfo.clearValueCount = clearValueCount;
    renderPassInfo.pClearValues = clearValues;

    vkCmdBeginRenderPass(commandbuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(commandbuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, scene.pipeline);
    vkCmdBindDescriptorSets(commandbuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, scene.pipelineLayout, 0, 1, &scene.pDescriptorSets[imageIndex], 0, nullptr);

    constexpr VkDeviceSize offsets[] = { 0 };

    for(uint32_t meshID = 0; meshID < meshArray.count; meshID++){
        const mpMesh &mesh = meshArray.data[meshID];
        if(mesh.vertexCount == 0)
            continue;

        vkCmdBindVertexBuffers(commandbuffer, 0, 1, &scene.vertexbuffers[meshID], offsets);
        vkCmdBindIndexBuffer(commandbuffer, scene.indexbuffers[meshID], 0, VK_INDEX_TYPE_UINT16);
        vkCmdDrawIndexed(commandbuffer, mesh.indexCount, 1, 0, 0, 0);
    }

    vkCmdBindPipeline(commandbuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, gui.pipeline);
    vkCmdBindDescriptorSets(commandbuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, gui.pipelineLayout, 0, 1, &gui.pDescriptorSets[imageIndex], 0, nullptr);
    for(uint32_t i = 0; i < texture.count; i++){
        if(mpgui.meshes[i].indexCount == 0)
            continue;

        int32_t textureIndex = i;
        vkCmdPushConstants(commandbuffer,gui.pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(int32_t), &textureIndex);

        vkCmdBindVertexBuffers(commandbuffer, 0, 1, &texture.pVertexbuffers[i], offsets);
        vkCmdBindIndexBuffer(commandbuffer, texture.pIndexbuffers[i], 0, VK_INDEX_TYPE_UINT16);
        vkCmdDrawIndexed(commandbuffer, mpgui.meshes[i].indexCount, 1, 0, 0, 0);
    }

    vkCmdEndRenderPass(commandbuffer);
    error = vkEndCommandBuffer(commandbuffer);
    mp_assert(!error);
}