#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <limits.h>
#include <stddef.h>

#include <cglm/cglm.h>

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

const uint32_t validationLayerCount = 1;
const char *validationLayers[] = {"VK_LAYER_KHRONOS_validation"};

const uint32_t deviceExtensionCount = 1;
const char *deviceExtensions[] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

const int MAX_FRAMES_IN_FLIGHT = 2;

uint32_t currentFrame = 0;
bool frameBufferResized = false;

typedef struct SwapChainSupportDetails{
    VkSurfaceCapabilitiesKHR capabilities;
    uint32_t formatCount;
    VkSurfaceFormatKHR *formats;
    uint32_t presentModeCount;
    VkPresentModeKHR *presentModes;
}SwapChainSupportDetails;

typedef struct QueueFamilyIndices{
    uint32_t graphicsFamily;
    bool isGraphicsFamilySet;
    uint32_t presentFamily;
    bool isPresentFamilySet;
}QueueFamilyIndices;

typedef struct App{
    GLFWwindow *window;
    VkInstance instance;
    VkSurfaceKHR surface;
    VkPhysicalDevice physicalDevice;
    QueueFamilyIndices queueFamilyIndices;
    VkDevice logicalDevice;
    VkQueue graphicsQueue;
    VkQueue presentQueue;
    VkSwapchainKHR swapChain;
    uint32_t swapChainImageCount;
    VkImage *swapChainImages;
    VkFormat swapChainImageFormat;
    VkExtent2D swapChainExtent;
    VkImageView *swapChainImageViews;
    VkRenderPass renderPass;
    VkFramebuffer *swapChainFramebuffers;
    VkPipelineLayout pipelineLayout;
    VkPipeline graphicsPipeline;
    VkCommandPool commandPool;
    VkCommandBuffer *commandBuffers;
    VkSemaphore *imageAvailableSemaphores;
    VkSemaphore *renderFinishedSemaphores;
    VkFence *inFlightFences;
}App;

typedef struct ShaderFile{
    size_t size;
    char *code;
}ShaderFile;

void initWindow(App *app);
void initVulkan(App *app);
void mainLoop(App *app);
void cleanApp(App *app);

void createInstance(App *app);
bool checkValidationLayerSupport();
bool verifyExtensionSupport(uint32_t extensionCount,VkExtensionProperties *extensions,uint32_t glfwExtensionCount,const char **glfwExtensions);
void createSurface(App *app);
bool checkDeviceExtensionSupport(VkPhysicalDevice device);
QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device,VkSurfaceKHR surface);
uint32_t rateDeviceSuitability(VkPhysicalDevice device,VkSurfaceKHR surface);
void pickPhysicalDevice(App *app);

void cleanUpSwapChain(App *app);

SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device,VkSurfaceKHR surface);

void getFamilyDeviceQueues(VkDeviceQueueCreateInfo *queues,QueueFamilyIndices indices);
void createLogicalDevice(App *app);


VkSurfaceFormatKHR chooseSwapSurfaceFormat(uint32_t formatCount, VkSurfaceFormatKHR *availableFormats);
void createSwapChain(App *app);
VkPresentModeKHR chooseSwapPresentMode(uint32_t presentModeCount, VkPresentModeKHR *availablePresentModes);
VkExtent2D chooseSwapExtent(GLFWwindow *window, VkSurfaceCapabilitiesKHR capabilities);
uint32_t clamp_u32(uint32_t n, uint32_t min, uint32_t max);

void createImageViews(App *app);
void createRenderPass(App *app);

void createGraphicsPipeline(App *app);
void readFile(const char *filename, ShaderFile *shader);
VkShaderModule createShaderModule(App *app, ShaderFile *shaderFile);


void createFramebuffers(App *app);

void createCommandPool(App *app);
void createCommandBuffers(App *app);


void createSyncObjects(App *app);

void recreateSwapChain(App *app);
void recordCommandBuffer(App *app, VkCommandBuffer commandBuffer, uint32_t imageIndex);
void drawFrame(App *app);

int main(void){
    App app = {0};
    initWindow(&app);
    initVulkan(&app);
    mainLoop(&app);
    cleanApp(&app);
    return 0;
}


void initWindow(App *app){
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    app->window = glfwCreateWindow(800, 800, "Miguel", NULL,NULL);
}


void initVulkan(App *app){
    createInstance(app);
    createSurface(app);
    pickPhysicalDevice(app);
    createLogicalDevice(app);
    createSwapChain(app);
    createImageViews(app);
    createRenderPass(app);
    createGraphicsPipeline(app);
    createFramebuffers(app);
    createCommandPool(app);
    createCommandBuffers(app);
    createSyncObjects(app);
}

void mainLoop(App *app){
    while(!glfwWindowShouldClose(app->window)){
        glfwPollEvents();
        drawFrame(app);
    }
}

void cleanApp(App *app){

    vkDeviceWaitIdle(app->logicalDevice);

    cleanUpSwapChain(app);

    for(uint32_t i =0;i<MAX_FRAMES_IN_FLIGHT;i++){
        vkDestroySemaphore(app->logicalDevice, app->imageAvailableSemaphores[i], NULL);
        vkDestroySemaphore(app->logicalDevice, app->renderFinishedSemaphores[i], NULL);
        vkDestroyFence(app->logicalDevice, app->inFlightFences[i], NULL);
    }
    vkDestroyCommandPool(app->logicalDevice, app->commandPool, NULL);
    vkDestroyPipeline(app->logicalDevice, app->graphicsPipeline, NULL);
    vkDestroyPipelineLayout(app->logicalDevice,app->pipelineLayout, NULL);
    vkDestroyRenderPass(app->logicalDevice, app->renderPass, NULL);
    vkDestroyDevice(app->logicalDevice, NULL);
    vkDestroySurfaceKHR(app->instance, app->surface, NULL);
    vkDestroyInstance(app->instance, NULL);
    glfwDestroyWindow(app->window);
    glfwTerminate();
}

void createInstance(App *app){
    if(enableValidationLayers && !checkValidationLayerSupport()){
        printf("validation layers requested, but not available!\n");
        exit(1);
    }
    VkApplicationInfo appInfo = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = "Miguel",
        .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
        .pEngineName = "No Engine",
        .engineVersion = VK_MAKE_VERSION(1, 0, 0),
        .apiVersion = VK_API_VERSION_1_0,
        .pNext = NULL
    };

    uint32_t glfwExtensionCount = 0;
    const char **glfwExtensions;

    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    const char **glfwExtensionsWithDebug = malloc(sizeof(char *)*(glfwExtensionCount+1));

    if(glfwExtensionsWithDebug == NULL){
        printf("Can't Allocate Memory!\n");
        free(glfwExtensionsWithDebug);
        exit(1);
    }

    for(uint32_t i = 0;i<glfwExtensionCount;i++){
        glfwExtensionsWithDebug[i] = glfwExtensions[i];
    }
    if(enableValidationLayers){
        glfwExtensionsWithDebug[glfwExtensionCount] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
    }
    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo = {0};
    VkInstanceCreateInfo createInfo = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &appInfo,
    };
    if(enableValidationLayers){
        createInfo.enabledLayerCount = validationLayerCount;
        createInfo.ppEnabledLayerNames = validationLayers;
        createInfo.enabledExtensionCount = glfwExtensionCount + 1;
        createInfo.ppEnabledExtensionNames = glfwExtensionsWithDebug;
        createInfo.pNext = NULL;
    }else{
        createInfo.enabledLayerCount = 0;
        createInfo.enabledExtensionCount = glfwExtensionCount;
        createInfo.ppEnabledExtensionNames = glfwExtensions;
        createInfo.pNext = NULL;
    }
    if(vkCreateInstance(&createInfo, NULL, &app->instance) != VK_SUCCESS){
        printf("failed to create instance!\n");
        free(glfwExtensionsWithDebug);
        exit(1);
    }
    uint32_t extensionCount = 0;

    vkEnumerateInstanceExtensionProperties(NULL, &extensionCount, NULL);

    VkExtensionProperties *extensions = malloc(sizeof(VkExtensionProperties)*extensionCount);
    if(extensions == NULL){
        printf("Can't Allocate Memory!\n");
        free(glfwExtensionsWithDebug);
        free(extensions);
        exit(1);
    }
    vkEnumerateInstanceExtensionProperties(NULL, &extensionCount, extensions);
    if(!(verifyExtensionSupport(extensionCount, extensions, glfwExtensionCount, glfwExtensions))){
        printf("Missing Extension Support!\n");
        free(extensions);
        free(glfwExtensionsWithDebug);
        exit(1);
    }
    free(extensions);
    free(glfwExtensionsWithDebug);
}

bool checkValidationLayerSupport(){
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, NULL);

    VkLayerProperties *availableLayers = (VkLayerProperties *)malloc(sizeof(VkLayerProperties)*layerCount);
    if(availableLayers == NULL){
        printf("Can't Allocate Memory!\n");
        free(availableLayers);
        exit(1);
    }
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers);

    for(uint32_t i = 0;i<validationLayerCount;i++){
        bool layerFound = false;
        for(uint32_t j = 0;j<layerCount;j++){
            if(strcmp(availableLayers[j].layerName, validationLayers[i]) == 0){
                layerFound = true;
                break;
            }
        }
        if(!layerFound){
            free(availableLayers);
            return false;
        }
    }
    free(availableLayers);
    return true;
}

bool verifyExtensionSupport(uint32_t extensionCount,VkExtensionProperties *extensions,uint32_t glfwExtensionCount,const char **glfwExtensions){
    for(uint32_t i = 0;i<glfwExtensionCount;i++){
        bool extensionFound = false;
        for(uint32_t j = 0;j<extensionCount;j++){
            if(strcmp(extensions[j].extensionName, glfwExtensions[i]) == 0){
                extensionFound = true;
                break;
            }
        }
        if(!extensionFound){
            return false;
        }
    }
    return true;
}

void createSurface(App *app){
   if(glfwCreateWindowSurface(app->instance, app->window,NULL, &app->surface)!=VK_SUCCESS){
       printf("Failed to Create Window Surface for Vulkan!\n");
       exit(1);
   }
}

QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device,VkSurfaceKHR surface){
    QueueFamilyIndices indices = {0};

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device,&queueFamilyCount, NULL);

    VkQueueFamilyProperties *queueFamilyProperties = malloc(sizeof(VkQueueFamilyProperties)*queueFamilyCount);
    if(queueFamilyProperties == NULL){
        printf("Can't Allocate Memory!\n");
        exit(1);
    }
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilyProperties);

    for(uint32_t i =0;i<queueFamilyCount;i++){
        if(queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT){
           indices.graphicsFamily = i;
           indices.isGraphicsFamilySet = true;
           free(queueFamilyProperties);
           break;
        }
        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
        if(presentSupport){
            indices.presentFamily = i;
            indices.isPresentFamilySet = true;
        }
    }
    return indices;
}

bool checkDeviceExtensionSupport(VkPhysicalDevice device){
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(device, NULL, &extensionCount, NULL);
    VkExtensionProperties *availableExtensions = malloc(sizeof(VkExtensionProperties)*extensionCount);
    if(availableExtensions == NULL){
        printf("Can't Allocate Memory!\n");
        exit(1);
    }
    vkEnumerateDeviceExtensionProperties(device, NULL, &extensionCount, availableExtensions);
    for(uint32_t i =0;i<deviceExtensionCount;i++){
        bool extensionFound = false;
        for(uint32_t j =0;j<extensionCount;j++){
            if(strcmp(deviceExtensions[i], availableExtensions[j].extensionName) == 0){
                extensionFound = true;
                free(availableExtensions);
                break;
            }
        }
        if(!extensionFound){
            free(availableExtensions);
            return false;
        }
    }
    return true;
}

SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device,VkSurfaceKHR surface){
    SwapChainSupportDetails details;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, NULL);
    details.formatCount = formatCount;
    VkSurfaceFormatKHR *formats = malloc(sizeof(VkSurfaceFormatKHR)*formatCount);
    details.formats = formats;

    if(formatCount != 0){
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats);
    }

    uint32_t presentCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentCount, NULL);
    details.presentModeCount = presentCount;
    VkPresentModeKHR *presentModes = malloc(sizeof(VkPresentModeKHR)*presentCount);
    details.presentModes = presentModes;

    if(presentCount != 0){
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentCount, details.presentModes);
    }

    return details;
}

void cleanUpSwapChain(App *app){
    if(app->swapChainFramebuffers!=NULL){
        for(uint32_t i =0;i<app->swapChainImageCount;i++){
            vkDestroyFramebuffer(app->logicalDevice, app->swapChainFramebuffers[i], NULL);
        }
        free(app->swapChainFramebuffers);
    }
    if(app->swapChainImageViews!=NULL){
        for(uint32_t i =0;i<app->swapChainImageCount;i++){
            vkDestroyImageView(app->logicalDevice, app->swapChainImageViews[i], NULL);
        }
        free(app->swapChainImageViews);
    }
    if(app->swapChain != VK_NULL_HANDLE){
        vkDestroySwapchainKHR(app->logicalDevice, app->swapChain, NULL);
    }
}

uint32_t rateDeviceSuitability(VkPhysicalDevice device,VkSurfaceKHR surface){
    VkPhysicalDeviceProperties deviceProperties;
    VkPhysicalDeviceFeatures deviceFeatures;
    vkGetPhysicalDeviceProperties(device, &deviceProperties);
    vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

    uint32_t score = 0;

    if(deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU){
        score+=100;
    }

    score += deviceProperties.limits.maxImageDimension2D;

    if(!deviceFeatures.geometryShader){
        return 0;
    }
    QueueFamilyIndices indices = findQueueFamilies(device, surface);
    if(!indices.isGraphicsFamilySet){
        printf("Queue Family not supported!\n");
        return 0;
    }
    bool extensionsSupported = checkDeviceExtensionSupport(device);
    if(!extensionsSupported){
        printf("Required device Extensions not Supported!\n");
        return 0;
    }
    SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device, surface);
    if(swapChainSupport.formatCount == 0 || swapChainSupport.presentModeCount == 0){
        printf("Swap Chain not adequately Supported!\n");
        return 0;
    }
    return score;
}
void pickPhysicalDevice(App *app){
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(app->instance, &deviceCount, NULL);

    if(deviceCount == 0){
        printf("Failed to Find any GPU Devices!\n");
        exit(1);
    }
    VkPhysicalDevice *devices = malloc(sizeof(VkPhysicalDevice)*deviceCount);

    if(devices == NULL){
        printf("Could Not Allocate Memory!\n");
        free(devices);
        exit(1);
    }
    vkEnumeratePhysicalDevices(app->instance, &deviceCount, devices);

    VkPhysicalDevice device;
    uint32_t deviceScore = 0;

    for(uint32_t i = 0;i<deviceCount;i++){
        uint32_t score = rateDeviceSuitability(devices[i], app->surface);
        if(score>deviceScore){
            deviceScore = score;
            device = devices[i];
        }
    }
    if(device == NULL){
        printf("Failed to find a Suitable GPU!\n");
        free(devices);
        exit(1);
    }
    app->physicalDevice = device;
    printf("GPU Selected\n");
    app->queueFamilyIndices = findQueueFamilies(device, app->surface);
    free(devices);
}

void getFamilyDeviceQueues(VkDeviceQueueCreateInfo *queues,QueueFamilyIndices indices){
    VkDeviceQueueCreateInfo graphicsQueueCreateInfo = (VkDeviceQueueCreateInfo){
      .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
      .queueFamilyIndex = indices.graphicsFamily,
      .queueCount = 1,
      .pNext = NULL,
    };
    static float graphicsQueuePriority = 1.0f;
    graphicsQueueCreateInfo.pQueuePriorities = &graphicsQueuePriority;
    queues[0] = graphicsQueueCreateInfo;

    VkDeviceQueueCreateInfo presentQueueCreateInfo = (VkDeviceQueueCreateInfo){
      .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
      .queueFamilyIndex = indices.presentFamily,
      .queueCount = 1,
      .pNext = NULL,
    };
    static float presentQueuePriority = 1.0f;
    presentQueueCreateInfo.pQueuePriorities = &presentQueuePriority;
    queues[1] = presentQueueCreateInfo;
}

void createLogicalDevice(App *app){
    QueueFamilyIndices indices = findQueueFamilies(app->physicalDevice, app->surface);

    VkPhysicalDeviceFeatures deviceFeatures;
    vkGetPhysicalDeviceFeatures(app->physicalDevice, &deviceFeatures);

    VkDeviceQueueCreateInfo queues[2];
    getFamilyDeviceQueues(queues, indices);

    VkDeviceCreateInfo createInfo = {
      .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
      .pQueueCreateInfos = queues,
      .queueCreateInfoCount = 1,
      .pEnabledFeatures = &deviceFeatures,
      .enabledExtensionCount = deviceExtensionCount,
      .ppEnabledExtensionNames = deviceExtensions,
    };
    if(enableValidationLayers){
        createInfo.enabledLayerCount = validationLayerCount;
        createInfo.ppEnabledLayerNames = validationLayers;
    }else{
        createInfo.enabledLayerCount = 0;
    }
    if(vkCreateDevice(app->physicalDevice, &createInfo, NULL, &app->logicalDevice)!=VK_SUCCESS){
        printf("Failed To Create Logical Device!\n");
        exit(1);
    }
    vkGetDeviceQueue(app->logicalDevice, app->queueFamilyIndices.graphicsFamily, 0, &app->graphicsQueue);
    vkGetDeviceQueue(app->logicalDevice, app->queueFamilyIndices.presentFamily, 0, &app->presentQueue);

}

VkSurfaceFormatKHR chooseSwapSurfaceFormat(uint32_t formatCount, VkSurfaceFormatKHR *availableFormats){
    for (uint32_t i = 0; i < formatCount; i++) {
      if (availableFormats[i].format == VK_FORMAT_B8G8R8A8_SRGB && availableFormats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
        return availableFormats[i];
      }
    }
    return availableFormats[0];
}

VkPresentModeKHR chooseSwapPresentMode(uint32_t presentModeCount, VkPresentModeKHR *availablePresentModes){
    for (uint32_t i = 0; i < presentModeCount; i++) {
      if (availablePresentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
        return availablePresentModes[i];
      }
    }
    return VK_PRESENT_MODE_FIFO_KHR;
}

uint32_t clamp_u32(uint32_t n, uint32_t min, uint32_t max){
    if (n < min) return min;
    if (n > max) return max;
    return n;
}

VkExtent2D chooseSwapExtent(GLFWwindow *window, VkSurfaceCapabilitiesKHR capabilities){
    if (capabilities.currentExtent.width != UINT_MAX) {
      return capabilities.currentExtent;
    } else {
      int width, height;
      glfwGetFramebufferSize(window, &width, &height);

      VkExtent2D actualExtent = { (uint32_t)width, (uint32_t)height };

      actualExtent.width = clamp_u32(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
      actualExtent.height = clamp_u32(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

      return actualExtent;
    }
}

void createSwapChain(App *app) {
  SwapChainSupportDetails swapChainSupport = querySwapChainSupport(app->physicalDevice, app->surface);

  VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formatCount, swapChainSupport.formats);
  printf("format = %d\n", surfaceFormat.format);
  printf("colorSpace = %d\n", surfaceFormat.colorSpace);

  VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModeCount, swapChainSupport.presentModes);
  VkExtent2D extent = chooseSwapExtent(app->window, swapChainSupport.capabilities);

  uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
  if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
    imageCount = swapChainSupport.capabilities.maxImageCount;
  }

  VkSwapchainCreateInfoKHR createInfo = {
    .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
    .surface = app->surface,
    .minImageCount = imageCount,
    .imageFormat = surfaceFormat.format,
    .imageColorSpace = surfaceFormat.colorSpace,
    .imageExtent = extent,
    .imageArrayLayers = 1,
    .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT
  };

  QueueFamilyIndices indices = findQueueFamilies(app->physicalDevice, app->surface);
  uint32_t queueFamilyIndices[] = { indices.graphicsFamily, indices.presentFamily };

  if (indices.graphicsFamily != indices.presentFamily) {
    createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
    createInfo.queueFamilyIndexCount = 2;
    createInfo.pQueueFamilyIndices = queueFamilyIndices;
  } else {
    createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    createInfo.queueFamilyIndexCount = 0; // Optional
    createInfo.pQueueFamilyIndices = NULL; // Optional
  }

  createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
  createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  createInfo.presentMode = presentMode;
  createInfo.clipped = VK_TRUE;
  createInfo.oldSwapchain = VK_NULL_HANDLE;

  if (vkCreateSwapchainKHR(app->logicalDevice, &createInfo, NULL, &app->swapChain) != VK_SUCCESS) {
    printf("Failed to create Swap Chain!\n");
    exit(1);
  }

  vkGetSwapchainImagesKHR(app->logicalDevice,app->swapChain, &imageCount, NULL);
  app->swapChainImages = (VkImage*)malloc(sizeof(VkImage) * imageCount);

  vkGetSwapchainImagesKHR(app->logicalDevice, app->swapChain, &imageCount, app->swapChainImages);
  app->swapChainImageCount = imageCount;

  app->swapChainImageFormat = surfaceFormat.format;
  app->swapChainExtent = extent;
  free(swapChainSupport.formats);
  free(swapChainSupport.presentModes);
}

void createImageViews(App *app){
    app->swapChainImageViews = (VkImageView*)malloc(sizeof(VkImageView) * app->swapChainImageCount);

    for (uint32_t i = 0; i < app->swapChainImageCount; i++) {
      VkImageViewCreateInfo createInfo = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = app->swapChainImages[i],
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = app->swapChainImageFormat,
        .components.r = VK_COMPONENT_SWIZZLE_IDENTITY,
        .components.g = VK_COMPONENT_SWIZZLE_IDENTITY,
        .components.b = VK_COMPONENT_SWIZZLE_IDENTITY,
        .components.a = VK_COMPONENT_SWIZZLE_IDENTITY,
        .subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .subresourceRange.baseMipLevel = 0,
        .subresourceRange.levelCount = 1,
        .subresourceRange.baseArrayLayer = 0,
        .subresourceRange.layerCount = 1
      };

      if (vkCreateImageView(app->logicalDevice, &createInfo, NULL, &app->swapChainImageViews[i]) != VK_SUCCESS) {
        printf("Failed to create image views!\n");
        free(app->swapChainImageViews);
        exit(6);
      }
    }
}

void createRenderPass(App *app){
    VkAttachmentDescription colorAttachment = {};
    colorAttachment.format = app->swapChainImageFormat;
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

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;

    VkSubpassDependency dependency = {};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    if (vkCreateRenderPass(app->logicalDevice, &renderPassInfo, NULL, &app->renderPass) != VK_SUCCESS) {
      printf("failed to create render pass!\n");
      exit(8);
    }
}

void readFile(const char *filename, ShaderFile *shader){
    FILE *pFile;

    pFile = fopen(filename, "rb");
    if (pFile == NULL) {
      printf("Failed to open %s\n", filename);
      exit(7);
    }

    fseek(pFile, 0L, SEEK_END);
    shader->size = ftell(pFile);

    fseek(pFile, 0L, SEEK_SET);

    shader->code = (char*)malloc(sizeof(char) * shader->size);
    size_t readCount = fread(shader->code, shader->size, sizeof(char), pFile);
    printf("ReadCount: %ld\n", readCount);

    fclose(pFile);
}

VkShaderModule createShaderModule(App *app, ShaderFile *shaderFile){
    VkShaderModuleCreateInfo createInfo = {
      .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
      .codeSize = shaderFile->size,
      .pCode = (uint32_t*)shaderFile->code
    };

    VkShaderModule shaderModule;
    if (vkCreateShaderModule(app->logicalDevice, &createInfo, NULL, &shaderModule) != VK_SUCCESS) {
      printf("failed to create shader module!\n");
      exit(7);
    }
    return shaderModule;
}

void createGraphicsPipeline(App *app){
    ShaderFile vertShader = {0};
    ShaderFile fragShader = {0};
    readFile("shaders/vert.spv", &vertShader);
    readFile("shaders/frag.spv", &fragShader);

    VkShaderModule vertShaderModule = createShaderModule(app, &vertShader);
    VkShaderModule fragShaderModule = createShaderModule(app, &fragShader);

    VkPipelineShaderStageCreateInfo vertShaderStageInfo = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
      .stage = VK_SHADER_STAGE_VERTEX_BIT,
      .module = vertShaderModule,
      .pName = "main"
      //.pSpecializationInfo = NULL
    };

    VkPipelineShaderStageCreateInfo fragShaderStageInfo = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
      .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
      .module = fragShaderModule,
      .pName = "main"
    };

    VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
      .vertexBindingDescriptionCount = 0,
      .pVertexBindingDescriptions = NULL, // Optional
      .vertexAttributeDescriptionCount = 0,
      .pVertexAttributeDescriptions = NULL // Optional
    };

    uint32_t dynamicStatesSize = 2;
    VkDynamicState dynamicStates[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };

    VkPipelineDynamicStateCreateInfo dynamicState = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
      .dynamicStateCount = dynamicStatesSize,
      .pDynamicStates = dynamicStates
    };

    VkPipelineInputAssemblyStateCreateInfo inputAssembly = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
      .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
      .primitiveRestartEnable = VK_FALSE
    };

    VkViewport viewport = {
      .x = 0.0f,
      .y = 0.0f,
      .width = (float)app->swapChainExtent.width,
      .height = (float)app->swapChainExtent.height,
      .minDepth = 0.0f,
      .maxDepth = 1.0f
    };

    VkRect2D scissor = {
      .offset = {0, 0},
      .extent = app->swapChainExtent
    };

    VkPipelineViewportStateCreateInfo viewportState = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
      .viewportCount = 1,
      .pViewports = &viewport,
      .scissorCount = 1,
      .pScissors = &scissor
    };

    VkPipelineRasterizationStateCreateInfo rasterizer = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
      .depthClampEnable = VK_FALSE,
      .rasterizerDiscardEnable = VK_FALSE,
      .polygonMode = VK_POLYGON_MODE_FILL,
      .lineWidth = 1.0f,
      .cullMode = VK_CULL_MODE_BACK_BIT,
      .frontFace = VK_FRONT_FACE_CLOCKWISE,
      .depthBiasEnable = VK_FALSE,
      .depthBiasConstantFactor = 0.0f, // Optional
      .depthBiasClamp = 0.0f, // Optional
      .depthBiasSlopeFactor = 0.0f // Optional
    };

    VkPipelineMultisampleStateCreateInfo multisampling = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
      .sampleShadingEnable = VK_FALSE,
      .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
      .minSampleShading = 1.0f, // Optional
      .pSampleMask = NULL, // Optional
      .alphaToCoverageEnable = VK_FALSE, // Optional
      .alphaToOneEnable = VK_FALSE // Optional
    };

    VkPipelineColorBlendAttachmentState colorBlendAttachment = {
      .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
      .blendEnable = VK_FALSE,
      .srcColorBlendFactor = VK_BLEND_FACTOR_ONE, // Optional
      .dstColorBlendFactor = VK_BLEND_FACTOR_ZERO, // Optional
      .colorBlendOp = VK_BLEND_OP_ADD, // Optional
      .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE, // Optional
      .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO, // Optional
      .alphaBlendOp = VK_BLEND_OP_ADD // Optional
    };

    VkPipelineColorBlendStateCreateInfo colorBlending = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
      .logicOpEnable = VK_FALSE,
      .logicOp = VK_LOGIC_OP_COPY, // Optional
      .attachmentCount = 1,
      .pAttachments = &colorBlendAttachment,
      .blendConstants[0] = 0.0f, // Optional
      .blendConstants[1] = 0.0f, // Optional
      .blendConstants[2] = 0.0f, // Optional
      .blendConstants[3] = 0.0f // Optional
    };

    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
      .setLayoutCount = 0, // Optional
      .pSetLayouts = NULL, // Optional
      .pushConstantRangeCount = 0, // Optional
      .pPushConstantRanges = NULL // Optional
    };

    if (vkCreatePipelineLayout(app->logicalDevice, &pipelineLayoutInfo, NULL, &app->pipelineLayout) != VK_SUCCESS) {
      printf("failed to create pipeline layout!");
      exit(7);
    }

    VkGraphicsPipelineCreateInfo pipelineInfo = {};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.flags = 0;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = NULL; // Optional
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = app->pipelineLayout;
    pipelineInfo.renderPass = app->renderPass;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
    pipelineInfo.basePipelineIndex = -1; // Optional

    if (vkCreateGraphicsPipelines(app->logicalDevice, VK_NULL_HANDLE, 1, &pipelineInfo, NULL, &app->graphicsPipeline) != VK_SUCCESS) {
      printf("Failed to create graphics pipeline!\n");
      exit(9);
    }

    free(vertShader.code);
    free(fragShader.code);
    vkDestroyShaderModule(app->logicalDevice, fragShaderModule, NULL);
    vkDestroyShaderModule(app->logicalDevice, vertShaderModule, NULL);
}

void createFramebuffers(App *app){
    app->swapChainFramebuffers = (VkFramebuffer*)malloc(app->swapChainImageCount * sizeof(VkFramebuffer));

    for (uint32_t i = 0; i < app->swapChainImageCount; i++) {
      VkImageView attachments[] = { app->swapChainImageViews[i] };

      VkFramebufferCreateInfo framebufferInfo = {};
      framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
      framebufferInfo.renderPass = app->renderPass;
      framebufferInfo.attachmentCount = 1;
      framebufferInfo.pAttachments = attachments;
      framebufferInfo.width = app->swapChainExtent.width;
      framebufferInfo.height = app->swapChainExtent.height;
      framebufferInfo.layers = 1;

      if (vkCreateFramebuffer(app->logicalDevice, &framebufferInfo, NULL, &app->swapChainFramebuffers[i]) != VK_SUCCESS) {
        printf("failed to create framebuffer!\n");
        exit(10);
      }
    }
}

void createCommandPool(App *app){
    QueueFamilyIndices indices = findQueueFamilies(app->physicalDevice, app->surface);

    VkCommandPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = indices.graphicsFamily;

    if (vkCreateCommandPool(app->logicalDevice, &poolInfo, NULL, &app->commandPool) != VK_SUCCESS) {
      printf("failed to create command pool!\n");
      exit(11);
    }
}

void createCommandBuffers(App *app){
    app->commandBuffers = (VkCommandBuffer*)malloc(sizeof(VkCommandBuffer) * MAX_FRAMES_IN_FLIGHT);

    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = app->commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = MAX_FRAMES_IN_FLIGHT;

    if (vkAllocateCommandBuffers(app->logicalDevice, &allocInfo, app->commandBuffers) != VK_SUCCESS) {
      printf("failed to allocate command buffers!\n");
      exit(12);
    }
}

void createSyncObjects(App *app){
    app->imageAvailableSemaphores = (VkSemaphore*)malloc(sizeof(VkSemaphore) * MAX_FRAMES_IN_FLIGHT);
    app->renderFinishedSemaphores = (VkSemaphore*)malloc(sizeof(VkSemaphore) * MAX_FRAMES_IN_FLIGHT);
    app->inFlightFences = (VkFence*)malloc(sizeof(VkFence) * MAX_FRAMES_IN_FLIGHT);

    VkSemaphoreCreateInfo semaphoreInfo = {};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo = {};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
      if (vkCreateSemaphore(app->logicalDevice, &semaphoreInfo, NULL, &app->imageAvailableSemaphores[i]) != VK_SUCCESS) {
        printf("Failed to create imageAvailableSemaphore!\n");
        exit(15);
      }
      if (vkCreateSemaphore(app->logicalDevice, &semaphoreInfo, NULL, &app->renderFinishedSemaphores[i]) != VK_SUCCESS) {
        printf("Failed to create renderFinishedSemaphore!\n");
        exit(15);
      }
      if (vkCreateFence(app->logicalDevice, &fenceInfo, NULL, &app->inFlightFences[i]) != VK_SUCCESS) {
        printf("Failed to create fence!\n");
        exit(15);
      }
    }
}

void recreateSwapChain(App *app){
    int width = 0, height = 0;
    glfwGetFramebufferSize(app->window, &width, &height);
    while (width == 0 || height == 0) {
      glfwGetFramebufferSize(app->window, &width, &height);
      glfwWaitEvents();
    }

    vkDeviceWaitIdle(app->logicalDevice);

    cleanUpSwapChain(app);

    createSwapChain(app);
    createImageViews(app);
    createFramebuffers(app);
}
void recordCommandBuffer(App *app, VkCommandBuffer commandBuffer, uint32_t imageIndex){
    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = 0; // Optional
    beginInfo.pInheritanceInfo = NULL; // Optional

    if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
      printf("failed to begin recording command buffer!\n");
      exit(13);
    }

    VkRenderPassBeginInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = app->renderPass;
    renderPassInfo.framebuffer = app->swapChainFramebuffers[imageIndex];
    renderPassInfo.renderArea.offset.x = 0;
    renderPassInfo.renderArea.offset.y = 0;
    renderPassInfo.renderArea.extent = app->swapChainExtent;

    VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clearColor;

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, app->graphicsPipeline);


    VkViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)app->swapChainExtent.width;
    viewport.height = (float)app->swapChainExtent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    VkRect2D scissor= {};
    scissor.offset.x = 0;
    scissor.offset.y = 0;
    scissor.extent = app->swapChainExtent;
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    vkCmdDraw(commandBuffer, 3, 1, 0, 0);

    vkCmdEndRenderPass(commandBuffer);

    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
      printf("failed to record command buffer!\n");
      exit(14);
    }
}
void drawFrame(App *app){
    vkWaitForFences(app->logicalDevice, 1, &app->inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

    vkResetFences(app->logicalDevice, 1, &app->inFlightFences[currentFrame]);

    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(app->logicalDevice, app->swapChain, UINT64_MAX, app->imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
      recreateSwapChain(app);
      return;
    } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
      printf("Failed to acquire swap chain image!\n");
      exit(17);
    }

    // Only reset the fence if we are submitting work
    vkResetFences(app->logicalDevice, 1, &app->inFlightFences[currentFrame]);

    vkResetCommandBuffer(app->commandBuffers[currentFrame], 0);
    recordCommandBuffer(app, app->commandBuffers[currentFrame], imageIndex);

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = { app->imageAvailableSemaphores[currentFrame] };
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &app->commandBuffers[currentFrame];

    VkSemaphore signalSemaphores[] = { app->renderFinishedSemaphores[currentFrame] };
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    if (vkQueueSubmit(app->graphicsQueue, 1, &submitInfo, app->inFlightFences[currentFrame]) != VK_SUCCESS) {
      printf("Failed to submit draw command buffer!\n");
      exit(16);
    }

    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    VkSwapchainKHR swapChains[] = { app->swapChain };
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &imageIndex;

    presentInfo.pResults = NULL; // Optional

    VkResult queueResult = vkQueuePresentKHR(app->presentQueue, &presentInfo);

    if (queueResult == VK_ERROR_OUT_OF_DATE_KHR || queueResult == VK_SUBOPTIMAL_KHR || frameBufferResized) {
      frameBufferResized = false;
      recreateSwapChain(app);
    } else if (queueResult != VK_SUCCESS) {
      printf("Failed to present swap chain image!\n");
      exit(17);
    }

    currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}
