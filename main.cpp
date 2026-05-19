#define VOLK_IMPLEMENTATION
#include<volk/volk.h>

#include<SDL3/SDL.h>
#include<SDL3/SDL_vulkan.h>

#define VMA_IMPLEMENTATION
#include<vma/vk_mem_alloc.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include<glm/glm.hpp>
#include<glm/gtc/matrix_transform.hpp>
#include<glm/gtc/quaternion.hpp>

#include "slang/slang.h"
#include "slang/slang-com-ptr.h"

#include<iostream>
#include<vector>
#include<array>

class Renderer {
public:

	struct Field;
	struct Constraints;

	void validateResult(VkResult result, std::string message = "ERROR!") {
		if (result != VK_SUCCESS) {
			std::cerr << "ERROR: " << message << std::endl;
			exit(result);
		}
	}

	void validateResult(bool result, std::string message = "ERROR!") {
		if (!result) {
			std::cerr << "ERROR " << message << std::endl;
			exit(result);
		}
	}

	void validateSwapchain(VkResult result) {
		if (result < VK_SUCCESS) {
			if (result == VK_ERROR_OUT_OF_DATE_KHR) {
				deviceIF.logical.swapchainConfiguration.updateSwapchain = true;
				return;
			}

			std::cerr << "ERROR: Swapchain Validation Failed" << std::endl;
			exit(result);
		}
	}

	void setupLibraries() {
		validateResult(SDL_Init(SDL_INIT_VIDEO));
		validateResult(SDL_Vulkan_LoadLibrary(NULL));
		volkInitialize();
	}

	void setupInstance() {
		VkApplicationInfo applicationInfo{
			.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
			.pApplicationName = "Rytutai",
			.apiVersion = VK_API_VERSION_1_4
		};

		extensionsIF.instance.extensions = SDL_Vulkan_GetInstanceExtensions(&extensionsIF.instance.count);

		VkInstanceCreateInfo instanceCreateInfo{
			.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
			.pApplicationInfo = &applicationInfo,
			.enabledExtensionCount = extensionsIF.instance.count,
			.ppEnabledExtensionNames = extensionsIF.instance.extensions
		};

		validateResult(vkCreateInstance(&instanceCreateInfo, nullptr, &windowIF.vkInstance));
		volkLoadInstance(windowIF.vkInstance);
	}

	void pickPhysicalDeviceAndQueueIndex() {
		validateResult(vkEnumeratePhysicalDevices(windowIF.vkInstance, &deviceIF.physical.count, nullptr));

		deviceIF.physical.devices.resize(deviceIF.physical.count);
		deviceIF.queue.properties.resize(deviceIF.physical.count);

		validateResult(vkEnumeratePhysicalDevices(windowIF.vkInstance, &deviceIF.physical.count, deviceIF.physical.devices.data()));

		for (uint32_t i = 0; i < deviceIF.physical.count; i++) {
			VkPhysicalDeviceProperties2 properties{
				.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2
			};

			vkGetPhysicalDeviceProperties2(deviceIF.physical.devices[i], &properties);
			deviceIF.physical.properties.push_back(properties);

			uint32_t queueFamiliesCount;
			vkGetPhysicalDeviceQueueFamilyProperties2(deviceIF.physical.devices[i], &queueFamiliesCount, nullptr);
			deviceIF.queue.properties[i].resize(queueFamiliesCount);

			for (uint32_t j = 0; j < queueFamiliesCount; j++) {
				deviceIF.queue.properties[i][j].sType = VK_STRUCTURE_TYPE_QUEUE_FAMILY_PROPERTIES_2;
			}

			vkGetPhysicalDeviceQueueFamilyProperties2(deviceIF.physical.devices[i], &queueFamiliesCount, deviceIF.queue.properties[i].data());
		}

		for (uint32_t i = 0; i < deviceIF.physical.count; i++) {

			bool found = false;
			for (uint32_t j = 0; j < deviceIF.queue.properties[i].size(); j++) {

				bool cond1 = deviceIF.physical.properties[i].properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;
				bool cond2 = deviceIF.queue.properties[i][j].queueFamilyProperties.queueFlags & VK_QUEUE_GRAPHICS_BIT;
				if (cond1 && cond2) {
					deviceIF.physical.index = i;
					deviceIF.physical.device = deviceIF.physical.devices[i];

					deviceIF.queue.index = j;

					found = true;
					break;
				}
			}

			if (found) {
				break;
			}
		}

		validateResult(SDL_Vulkan_GetPresentationSupport(windowIF.vkInstance, deviceIF.physical.device, deviceIF.queue.index));
	}

	void setupLogicalDevice() {

		float priorities = 1.0;

		VkDeviceQueueCreateInfo queueCreateInfo{
			.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
			.queueFamilyIndex = deviceIF.queue.index,
			.queueCount = 1,
			.pQueuePriorities = &priorities
		};

		VkPhysicalDeviceVulkan11Features enabledVk11Features{
			.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES,
			.shaderDrawParameters = VK_TRUE,
		};

		VkPhysicalDeviceVulkan13Features enabledVk13Features{
			.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
			.pNext = &enabledVk11Features,
			.synchronization2 = VK_TRUE,
			.dynamicRendering = VK_TRUE,
		};

		VkPhysicalDeviceFeatures deviceFeatures{
			.fillModeNonSolid = VK_TRUE
		};

		VkDeviceCreateInfo deviceCreateInfo{
			.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
			.pNext = &enabledVk13Features,
			.queueCreateInfoCount = 1,
			.pQueueCreateInfos = &queueCreateInfo,
			.enabledExtensionCount = static_cast<uint32_t>(extensionsIF.device.extensions.size()),
			.ppEnabledExtensionNames = extensionsIF.device.extensions.data(),
			.pEnabledFeatures = &deviceFeatures
		};

		validateResult(vkCreateDevice(deviceIF.physical.device, &deviceCreateInfo, nullptr, &deviceIF.logical.device));

		volkLoadDevice(deviceIF.logical.device);
		vkGetDeviceQueue(deviceIF.logical.device, deviceIF.queue.index, 0, &deviceIF.queue.queue);
	}

	void setupVMA() {
		VmaVulkanFunctions vkFunctions{
			.vkGetInstanceProcAddr = vkGetInstanceProcAddr,
			.vkGetDeviceProcAddr = vkGetDeviceProcAddr
		};

		VmaAllocatorCreateInfo allocatorCreateInfo{
			.physicalDevice = deviceIF.physical.device,
			.device = deviceIF.logical.device,
			.pVulkanFunctions = &vkFunctions,
			.instance = windowIF.vkInstance
		};

		validateResult(vmaCreateAllocator(&allocatorCreateInfo, &vmaAllocator));
	}

	void setupWindowAndSurface() {
		windowIF.window = SDL_CreateWindow(
			windowIF.windowName.c_str(),
			windowIF.windowWidth,
			windowIF.windowHeight,
			windowIF.windowFlags
		);

		validateResult(SDL_GetWindowSize(windowIF.window, &windowIF.dimensions.x, &windowIF.dimensions.y));

		validateResult(SDL_Vulkan_CreateSurface(windowIF.window, windowIF.vkInstance, nullptr, &windowIF.surface));
		validateResult(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(deviceIF.physical.device, windowIF.surface, &windowIF.surfaceCapabilities));
	}

	void setupSwapchain() {
		deviceIF.logical.swapchainConfiguration.swapchainCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
			.surface = windowIF.surface,
			.minImageCount = windowIF.surfaceCapabilities.minImageCount,
			.imageFormat = deviceIF.logical.swapchainConfiguration.imageFormat,
			.imageColorSpace = deviceIF.logical.swapchainConfiguration.imageColorSpace,
			.imageExtent = {
				.width = windowIF.surfaceCapabilities.currentExtent.width,
				.height = windowIF.surfaceCapabilities.currentExtent.height
			},
			.imageArrayLayers = 1,
			.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
			.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
			.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
			.presentMode = VK_PRESENT_MODE_FIFO_KHR
		};

		validateResult(vkCreateSwapchainKHR(deviceIF.logical.device, &deviceIF.logical.swapchainConfiguration.swapchainCreateInfo, nullptr, &deviceIF.logical.swapchainConfiguration.swapchain));

		validateResult(vkGetSwapchainImagesKHR(deviceIF.logical.device, deviceIF.logical.swapchainConfiguration.swapchain, &deviceIF.logical.swapchainConfiguration.imageCount, nullptr));
		deviceIF.logical.swapchainConfiguration.swapchainImages.resize(deviceIF.logical.swapchainConfiguration.imageCount);
		deviceIF.logical.swapchainConfiguration.swapchainImageViews.resize(deviceIF.logical.swapchainConfiguration.imageCount);

		validateResult(vkGetSwapchainImagesKHR(deviceIF.logical.device, deviceIF.logical.swapchainConfiguration.swapchain, &deviceIF.logical.swapchainConfiguration.imageCount, deviceIF.logical.swapchainConfiguration.swapchainImages.data()));

		for (uint32_t i = 0; i < deviceIF.logical.swapchainConfiguration.imageCount; i++) {
			VkImageViewCreateInfo imageViewCreateInfo{
				.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
				.image = deviceIF.logical.swapchainConfiguration.swapchainImages[i],
				.viewType = VK_IMAGE_VIEW_TYPE_2D,
				.format = deviceIF.logical.swapchainConfiguration.imageFormat,
				.subresourceRange {
					.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
					.levelCount = 1,
					.layerCount = 1
				}
			};

			validateResult(vkCreateImageView(deviceIF.logical.device, &imageViewCreateInfo, nullptr, &deviceIF.logical.swapchainConfiguration.swapchainImageViews[i]));
		}
	}

	void setupSync2() {
		VkSemaphoreCreateInfo semaphoreCreateInfo{
			.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO
		};

		VkFenceCreateInfo fenceCreateInfo{
			.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
			.flags = VK_FENCE_CREATE_SIGNALED_BIT
		};

		for (uint32_t i = 0; i < deviceIF.logical.swapchainConfiguration.maxFramesInFlight; i++) {
			validateResult(vkCreateFence(deviceIF.logical.device, &fenceCreateInfo, nullptr, &deviceIF.logical.swapchainConfiguration.fences[i]));
			validateResult(vkCreateSemaphore(deviceIF.logical.device, &semaphoreCreateInfo, nullptr, &deviceIF.logical.swapchainConfiguration.presentSemaphore[i]));
		}

		deviceIF.logical.swapchainConfiguration.renderSemaphore.resize(deviceIF.logical.swapchainConfiguration.imageCount);
		for (VkSemaphore& semaphore : deviceIF.logical.swapchainConfiguration.renderSemaphore) {
			validateResult(vkCreateSemaphore(deviceIF.logical.device, &semaphoreCreateInfo, nullptr, &semaphore));
		}
	}

	void setupCommandBuffers() {
		VkCommandPoolCreateInfo commandPoolCreateInfo{
			.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
			.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
			.queueFamilyIndex = deviceIF.queue.index
		};

		validateResult(vkCreateCommandPool(deviceIF.logical.device, &commandPoolCreateInfo, nullptr, &deviceIF.logical.commandPool));

		VkCommandBufferAllocateInfo commandBufferAllocateInfo{
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
			.commandPool = deviceIF.logical.commandPool,
			.commandBufferCount = deviceIF.logical.swapchainConfiguration.maxFramesInFlight
		};

		validateResult(vkAllocateCommandBuffers(deviceIF.logical.device, &commandBufferAllocateInfo, deviceIF.logical.swapchainConfiguration.commandBuffers.data()));
	}

	void loadAndCompileShaders() {
		slang::createGlobalSession(slangGlobalSession.writeRef());

		auto slangTargets{
			std::to_array< slang::TargetDesc >({{
				.format{SLANG_SPIRV},
				.profile{slangGlobalSession->findProfile("spirv_1_4")}
			}})
		};

		auto slangOptions{
			std::to_array < slang::CompilerOptionEntry>({{
				slang::CompilerOptionName::EmitSpirvDirectly,
				{
					slang::CompilerOptionValueKind::Int, 1
				}
			}})
		};

		slang::SessionDesc slangSessionDesc{
			.targets{
				slangTargets.data()
			},
			.targetCount{
				SlangInt(slangTargets.size())
			},
			.defaultMatrixLayoutMode = SLANG_MATRIX_LAYOUT_COLUMN_MAJOR,
			.compilerOptionEntries{
				slangOptions.data()
			},
			.compilerOptionEntryCount{
				uint32_t(slangOptions.size())
			}
		};

		Slang::ComPtr< slang::ISession > slangSession;
		slangGlobalSession->createSession(slangSessionDesc, slangSession.writeRef());

		Slang::ComPtr< slang::IModule > slangModule{
			slangSession->loadModuleFromSource("Modern Vulkan SLANG Shader", "assets/shaders/shader.slang", nullptr, nullptr)
		};

		Slang::ComPtr< ISlangBlob > spirv;
		slangModule->getTargetCode(0, spirv.writeRef());

		VkShaderModuleCreateInfo shaderModuleCreateInfo{
			.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
			.codeSize = spirv->getBufferSize(),
			.pCode = (uint32_t*)spirv->getBufferPointer()
		};

		validateResult(vkCreateShaderModule(deviceIF.logical.device, &shaderModuleCreateInfo, nullptr, &shaderModule));

	}

	void createPipeline(VkPipeline& pipeline, std::vector< VkPipelineShaderStageCreateInfo >& shaderStages, std::vector<VkDescriptorSetLayout>&layout, VkPipelineLayout& pipelineLayout, VkFormat format, VkColorComponentFlags flags, uint32_t pushConstantSize = 0) {

		VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
		if (pushConstantSize == 0) {
			pipelineLayoutCreateInfo = {
				.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
				.setLayoutCount = static_cast<uint32_t>(layout.size()),
				.pSetLayouts = layout.data()
			};
		}
		else {
			VkPushConstantRange pushConstantRange{
				.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
				.size = pushConstantSize
			};

			pipelineLayoutCreateInfo = {
				.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
				.setLayoutCount = static_cast<uint32_t>(layout.size()),
				.pSetLayouts = layout.data(),
				.pushConstantRangeCount = 1,
				.pPushConstantRanges = &pushConstantRange
			};
		}

		validateResult(vkCreatePipelineLayout(deviceIF.logical.device, &pipelineLayoutCreateInfo, nullptr, &pipelineLayout));

		VkPipelineVertexInputStateCreateInfo vertexInputState{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
			.vertexBindingDescriptionCount = 0,
			.pVertexBindingDescriptions = nullptr,
			.vertexAttributeDescriptionCount = 0,
			.pVertexAttributeDescriptions = nullptr,
		};

		VkPipelineInputAssemblyStateCreateInfo inputAssemblyState{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
			.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST
		};

		std::vector< VkDynamicState > dynamicStates{
			VK_DYNAMIC_STATE_VIEWPORT,
			VK_DYNAMIC_STATE_SCISSOR
		};

		VkPipelineDynamicStateCreateInfo dynamicState{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
			.dynamicStateCount = 2,
			.pDynamicStates = dynamicStates.data()
		};

		VkPipelineViewportStateCreateInfo viewportState{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
			.viewportCount = 1,
			.scissorCount = 1
		};

		VkPipelineDepthStencilStateCreateInfo depthStencilState{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
			.depthTestEnable = VK_FALSE,
			.depthWriteEnable = VK_FALSE
		};

		VkPipelineRenderingCreateInfo renderingCreateInfo{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
			.colorAttachmentCount = 1,
			.pColorAttachmentFormats = &format,
		};

		VkPipelineColorBlendAttachmentState blendAttachment{
			.colorWriteMask = flags
		};

		VkPipelineColorBlendStateCreateInfo colorBlendState{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
			.attachmentCount = 1,
			.pAttachments = &blendAttachment
		};

		VkPipelineRasterizationStateCreateInfo rasterizationState{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
			.polygonMode = VK_POLYGON_MODE_LINE,
			.cullMode = VK_CULL_MODE_NONE,
			.frontFace = VK_FRONT_FACE_CLOCKWISE,
			.lineWidth = 1.0f
		};

		VkPipelineMultisampleStateCreateInfo multisampleState{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
			.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT
		};

		VkGraphicsPipelineCreateInfo pipelineCreateInfo{
			.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
			.pNext = &renderingCreateInfo,
			.stageCount = 2,
			.pStages = shaderStages.data(),
			.pVertexInputState = &vertexInputState,
			.pInputAssemblyState = &inputAssemblyState,
			.pViewportState = &viewportState,
			.pRasterizationState = &rasterizationState,
			.pMultisampleState = &multisampleState,
			.pDepthStencilState = &depthStencilState,
			.pColorBlendState = &colorBlendState,
			.pDynamicState = &dynamicState,
			.layout = pipelineLayout
		};

		validateResult(vkCreateGraphicsPipelines(deviceIF.logical.device, nullptr, 1, &pipelineCreateInfo, nullptr, &pipeline));
	}

	void createComputePipeline(VkPipeline& pipeline, VkPipelineShaderStageCreateInfo& shaderStages, VkPipelineLayout& pipelineLayout) {
		VkComputePipelineCreateInfo pipelineInfo{
			.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
			.stage = shaderStages,
			.layout = pipelineLayout
		};

		vkCreateComputePipelines(deviceIF.logical.device, nullptr, 1, &pipelineInfo, nullptr, &pipeline);
	}

	void setupPipeline() {
		std::vector<VkDescriptorSetLayout> graphicsDescriptorSetLayouts;
		graphicsDescriptorSetLayouts.push_back( cloth.descriptorSetLayout );

		VkPipelineLayoutCreateInfo graphicsPipelineLayoutCI{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
			.setLayoutCount = static_cast< uint32_t >( graphicsDescriptorSetLayouts.size()),
			.pSetLayouts = graphicsDescriptorSetLayouts.data()
		};

		validateResult(vkCreatePipelineLayout(deviceIF.logical.device, &graphicsPipelineLayoutCI, nullptr, &graphicsPipeline.pipelineLayout ));

		std::vector< VkPipelineShaderStageCreateInfo > graphicsPipelineShaderStages{
			{
				.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
				.stage = VK_SHADER_STAGE_VERTEX_BIT,
				.module = shaderModule,
				.pName = "vertexShader"
			},
			{
				.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
				.stage = VK_SHADER_STAGE_FRAGMENT_BIT,
				.module = shaderModule,
				.pName = "fragmentShader"
			}
		};
		
		createPipeline( graphicsPipeline.pipeline, graphicsPipelineShaderStages, graphicsDescriptorSetLayouts, graphicsPipeline.pipelineLayout, VK_FORMAT_B8G8R8A8_SRGB, VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT);
	}

	void recreateSwapchain() {
		deviceIF.logical.swapchainConfiguration.updateSwapchain = false;
		vkDeviceWaitIdle(deviceIF.logical.device);
		validateResult(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(deviceIF.physical.device, windowIF.surface, &windowIF.surfaceCapabilities), "Failed to Get Surface Capabilities");
		deviceIF.logical.swapchainConfiguration.swapchainCreateInfo.oldSwapchain = deviceIF.logical.swapchainConfiguration.swapchain;
		deviceIF.logical.swapchainConfiguration.swapchainCreateInfo.imageExtent = { .width = static_cast<uint32_t>(windowIF.dimensions.x), .height = static_cast<uint32_t>(windowIF.dimensions.y) };
		validateResult(vkCreateSwapchainKHR(deviceIF.logical.device, &deviceIF.logical.swapchainConfiguration.swapchainCreateInfo, nullptr, &deviceIF.logical.swapchainConfiguration.swapchain), "Failed to Create Swap chain");
		for (uint32_t i = 0; i < deviceIF.logical.swapchainConfiguration.imageCount; i++) {
			vkDestroyImageView(deviceIF.logical.device, deviceIF.logical.swapchainConfiguration.swapchainImageViews[i], nullptr);
		}
		validateResult(vkGetSwapchainImagesKHR(deviceIF.logical.device, deviceIF.logical.swapchainConfiguration.swapchain, &deviceIF.logical.swapchainConfiguration.imageCount, nullptr), "Failed To Create Swap Chain Images");
		deviceIF.logical.swapchainConfiguration.swapchainImages.resize(deviceIF.logical.swapchainConfiguration.imageCount);
		validateResult(vkGetSwapchainImagesKHR(deviceIF.logical.device, deviceIF.logical.swapchainConfiguration.swapchain, &deviceIF.logical.swapchainConfiguration.imageCount, deviceIF.logical.swapchainConfiguration.swapchainImages.data()), "Failed To Get Swap Chain Images");
		deviceIF.logical.swapchainConfiguration.swapchainImageViews.resize(deviceIF.logical.swapchainConfiguration.imageCount);
		for (uint32_t i = 0; i < deviceIF.logical.swapchainConfiguration.imageCount; i++) {
			VkImageViewCreateInfo viewCI{
				.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
				.image = deviceIF.logical.swapchainConfiguration.swapchainImages[i],
				.viewType = VK_IMAGE_VIEW_TYPE_2D,
				.format = deviceIF.logical.swapchainConfiguration.imageFormat,
				.subresourceRange = {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .levelCount = 1, .layerCount = 1}
			};
			validateResult(vkCreateImageView(deviceIF.logical.device, &viewCI, nullptr, &deviceIF.logical.swapchainConfiguration.swapchainImageViews[i]), "Failed To Create Image View");
		}
		vkDestroySwapchainKHR(deviceIF.logical.device, deviceIF.logical.swapchainConfiguration.swapchainCreateInfo.oldSwapchain, nullptr);
	}

	struct RenderingAttachment {
		VkRenderingAttachmentInfo attachmentInfo;
		VkRenderingInfo renderingInfo;
	};

	RenderingAttachment setRenderingAttachment(VkImageView& imageView) {
		RenderingAttachment attachment{};
		attachment.attachmentInfo = {
			.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
			.imageView = imageView,
			.imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
			.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
			.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
			.clearValue{
				windowIF.clearColor
			}
		};

		attachment.renderingInfo = {
			.sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
			.renderArea{
				.extent{
					.width = static_cast<uint32_t>(windowIF.dimensions.x),
					.height = static_cast<uint32_t>(windowIF.dimensions.y)
				},
			},
			.layerCount = 1,
			.colorAttachmentCount = 1,
			.pColorAttachments = &attachment.attachmentInfo
		};

		return attachment;
	}

	void transitionImageUndefinedToAttachment(VkCommandBuffer& commandBuffer) {
		VkImageMemoryBarrier2 outputBarrier{
			.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
			.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			.srcAccessMask = 0,
			.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
			.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
			.newLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
			.image = deviceIF.logical.swapchainConfiguration.swapchainImages[imageIndex],
			.subresourceRange {
				.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
				.levelCount = 1,
				.layerCount = 1
			}
		};

		VkDependencyInfo barrierDependencyInfo{
			.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
			.imageMemoryBarrierCount = 1,
			.pImageMemoryBarriers = &outputBarrier
		};

		vkCmdPipelineBarrier2(commandBuffer, &barrierDependencyInfo);
	}

	void transitionImageAttachmentToPresent(VkCommandBuffer& commandBuffer) {
		VkImageMemoryBarrier2 barrierPresent{
			.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
			.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
			.dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
			.dstAccessMask = 0,
			.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
			.image = deviceIF.logical.swapchainConfiguration.swapchainImages[imageIndex],
			.subresourceRange{.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .levelCount = 1, .layerCount = 1 }
		};
		VkDependencyInfo barrierPresentDependencyInfo{
			.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
			.imageMemoryBarrierCount = 1,
			.pImageMemoryBarriers = &barrierPresent
		};
		vkCmdPipelineBarrier2(commandBuffer, &barrierPresentDependencyInfo);
	}


	void animate() {
		bool quit{ false };
		while (!quit) {
			SDL_Event event;
			while (SDL_PollEvent(&event)) {
				if (event.type == SDL_EVENT_QUIT) {
					quit = true;
				}

				if (event.type == SDL_EVENT_WINDOW_RESIZED) {
					deviceIF.logical.swapchainConfiguration.updateSwapchain = true;
				}
			}

			validateResult(vkWaitForFences(deviceIF.logical.device, 1, &deviceIF.logical.swapchainConfiguration.fences[frameIndex], true, UINT64_MAX));
			validateResult(vkResetFences(deviceIF.logical.device, 1, &deviceIF.logical.swapchainConfiguration.fences[frameIndex]));

			validateSwapchain(vkAcquireNextImageKHR(deviceIF.logical.device, deviceIF.logical.swapchainConfiguration.swapchain, UINT64_MAX, deviceIF.logical.swapchainConfiguration.presentSemaphore[frameIndex], VK_NULL_HANDLE, &imageIndex));

			auto commandBuffer = deviceIF.logical.swapchainConfiguration.commandBuffers[frameIndex];
			validateResult(vkResetCommandBuffer(commandBuffer, 0));

			VkCommandBufferBeginInfo commandBufferBeginInfo{
				.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
				.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
			};

			validateResult(vkBeginCommandBuffer(commandBuffer, &commandBufferBeginInfo));

			VkViewport viewport{
				.width = float(windowIF.dimensions.x),
				.height = float(windowIF.dimensions.y),
				.minDepth = 0.0f,
				.maxDepth = 1.0f
			};

			vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
			VkRect2D scissor{
				.extent {
					.width = static_cast<uint32_t>(windowIF.dimensions.x),
					.height = static_cast<uint32_t>(windowIF.dimensions.y),
				}
			};
			vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

			transitionImageUndefinedToAttachment( commandBuffer );
			RenderingAttachment renderingAttachment = setRenderingAttachment(deviceIF.logical.swapchainConfiguration.swapchainImageViews[imageIndex]);
			vkCmdBeginRendering(commandBuffer, &renderingAttachment.renderingInfo );
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline.pipeline );
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline.pipelineLayout, 0, 1, &cloth.descriptorSet, 0, nullptr);
			vkCmdBindIndexBuffer(commandBuffer, cloth.indices.buffer, 0, VK_INDEX_TYPE_UINT32);
			vkCmdDrawIndexed( commandBuffer, cloth.indexData.size(), 1, 0, 0, 0 );
			vkCmdEndRendering(commandBuffer);
			transitionImageAttachmentToPresent(commandBuffer);

			vkEndCommandBuffer(commandBuffer);

			VkPipelineStageFlags waitStages = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			VkSubmitInfo submitInfo{
				.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
				.waitSemaphoreCount = 1,
				.pWaitSemaphores = &deviceIF.logical.swapchainConfiguration.presentSemaphore[frameIndex],
				.pWaitDstStageMask = &waitStages,
				.commandBufferCount = 1,
				.pCommandBuffers = &commandBuffer,
				.signalSemaphoreCount = 1,
				.pSignalSemaphores = &deviceIF.logical.swapchainConfiguration.renderSemaphore[imageIndex],
			};
			validateResult(vkQueueSubmit(deviceIF.queue.queue, 1, &submitInfo, deviceIF.logical.swapchainConfiguration.fences[frameIndex]), "Failed to Submit Queue");

			frameIndex = (frameIndex + 1) % deviceIF.logical.swapchainConfiguration.maxFramesInFlight;

			VkPresentInfoKHR presentInfo{
				.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
				.waitSemaphoreCount = 1,
				.pWaitSemaphores = &deviceIF.logical.swapchainConfiguration.renderSemaphore[imageIndex],
				.swapchainCount = 1,
				.pSwapchains = &deviceIF.logical.swapchainConfiguration.swapchain,
				.pImageIndices = &imageIndex
			};
			validateResult(vkQueuePresentKHR(deviceIF.queue.queue, &presentInfo), "Failed To Present Queue");

			if (deviceIF.logical.swapchainConfiguration.updateSwapchain) {
				recreateSwapchain();
			}
		}
	}

	void setupDescriptorSet() {
		
		VkDescriptorPoolSize poolSize{
			.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			.descriptorCount = static_cast<uint32_t>(cloth.descriptorCount)
		};

		VkDescriptorPoolCreateInfo descriptorPoolCreateInfo{
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
			.maxSets = 1,
			.poolSizeCount = 1,
			.pPoolSizes = &poolSize
		};

		validateResult(vkCreateDescriptorPool(deviceIF.logical.device, &descriptorPoolCreateInfo, nullptr, &descriptorPool));

		std::vector< VkDescriptorSetLayoutBinding > bindings;
		for (uint32_t index = 0; index < cloth.descriptorCount; index++) {
			VkDescriptorSetLayoutBinding descriptorSetLayoutBinding{
				.binding = index,
				.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
				.descriptorCount = 1,
				.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_VERTEX_BIT
			};

			bindings.push_back(descriptorSetLayoutBinding);
		}

		VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCI{
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
			.bindingCount = static_cast< uint32_t >( bindings.size() ),
			.pBindings = bindings.data()
		};

		vkCreateDescriptorSetLayout(deviceIF.logical.device, &descriptorSetLayoutCI, nullptr, &cloth.descriptorSetLayout );

		VkDescriptorSetAllocateInfo descriptorAllocateInfo{
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
			.descriptorPool = descriptorPool,
			.descriptorSetCount = 1,
			.pSetLayouts = &cloth.descriptorSetLayout
		};

		validateResult(vkAllocateDescriptorSets(deviceIF.logical.device, &descriptorAllocateInfo, &cloth.descriptorSet ));
	}

	void setupBuffer( const void* data, size_t byteSize, Field& field, VkBufferUsageFlags flags ) {
		VkBufferCreateInfo bufferCreateInfo{
			.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
			.size = byteSize,
			.usage = flags | VK_BUFFER_USAGE_TRANSFER_DST_BIT
		};

		VmaAllocationCreateInfo allocationCreateInfo{
			.flags = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE,
			.usage = VMA_MEMORY_USAGE_AUTO
		};

		validateResult(vmaCreateBuffer(vmaAllocator, &bufferCreateInfo, &allocationCreateInfo, &field.buffer, &field.allocation, &field.allocationInfo));

		VkBuffer stagingBuffer{};
		VmaAllocation stagingBufferAllocation{};
		VkBufferCreateInfo stagingBufferCI{
			.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
			.size = byteSize,
			.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT
		};

		VmaAllocationCreateInfo stagingBufferAllocationCI{
			.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
			.usage = VMA_MEMORY_USAGE_AUTO
		};

		validateResult(vmaCreateBuffer(vmaAllocator, &stagingBufferCI, &stagingBufferAllocationCI, &stagingBuffer, &stagingBufferAllocation, nullptr), "Failed to Create VMA Buffer");

		VkFenceCreateInfo fenceOneTimeCreateInfo{
			.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO
		};
		VkFence fenceOneTime{};

		validateResult(vkCreateFence(deviceIF.logical.device, &fenceOneTimeCreateInfo, nullptr, &fenceOneTime), "Failed to Create one time Fence");

		VkCommandBuffer commandBufferOneTime{};
		VkCommandBufferAllocateInfo commandBufferOneTimeAllocationInfo{
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
			.commandPool = deviceIF.logical.commandPool,
			.commandBufferCount = 1
		};

		validateResult(vkAllocateCommandBuffers(deviceIF.logical.device, &commandBufferOneTimeAllocationInfo, &commandBufferOneTime), "Failed to Allocate one time command buffer");

		VkCommandBufferBeginInfo commandBufferOneTimeBeginInfo{
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
			.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
		};

		
		void* imageSourceBufferPtr{ nullptr };
		vmaMapMemory(vmaAllocator, stagingBufferAllocation, &imageSourceBufferPtr);
		memcpy(imageSourceBufferPtr, data, byteSize);
		vmaFlushAllocation(vmaAllocator, stagingBufferAllocation, 0, VK_WHOLE_SIZE);
		vmaUnmapMemory(vmaAllocator, stagingBufferAllocation);
		

		validateResult(vkBeginCommandBuffer(commandBufferOneTime, &commandBufferOneTimeBeginInfo), "Failed To Begin Command Buffer");

		VkBufferCopy copyRegion{};
		copyRegion.srcOffset = 0;
		copyRegion.dstOffset = 0;
		copyRegion.size = byteSize;

		vkCmdCopyBuffer(
			commandBufferOneTime,
			stagingBuffer,
			field.buffer,
			1,
			&copyRegion
		);

		VkBufferMemoryBarrier2 barrier{
			.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2,
			.srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
			.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
			.dstStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT,
			.dstAccessMask = VK_ACCESS_2_SHADER_STORAGE_READ_BIT | VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT | VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT,
			.buffer = field.buffer,
			.offset = 0,
			.size = VK_WHOLE_SIZE
		};

		VkDependencyInfo dependencyInfo{
			.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
			.bufferMemoryBarrierCount = 1,
			.pBufferMemoryBarriers = &barrier
		};

		vkCmdPipelineBarrier2( commandBufferOneTime,  &dependencyInfo );

		validateResult(vkEndCommandBuffer(commandBufferOneTime), "Failed to end Command buffer");

		VkSubmitInfo oneTimeSubmitInfo{
			.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
			.commandBufferCount = 1,
			.pCommandBuffers = &commandBufferOneTime
		};

		validateResult(vkQueueSubmit(deviceIF.queue.queue, 1, &oneTimeSubmitInfo, fenceOneTime), "Failed to submit to queue");
		validateResult(vkWaitForFences(deviceIF.logical.device, 1, &fenceOneTime, VK_TRUE, UINT64_MAX), "Failed to Wait for Fences");
	
		vmaDestroyBuffer(vmaAllocator, stagingBuffer, stagingBufferAllocation);
		vkDestroyFence( deviceIF.logical.device, fenceOneTime, nullptr );
		vkFreeCommandBuffers( deviceIF.logical.device, deviceIF.logical.commandPool, 1, &commandBufferOneTime );
	}

	void updateDescriptors() {
		Field* fields[] = {
			&cloth.position,
			&cloth.predictedPosition,
			&cloth.velocity,
			&cloth.mass,
			& cloth.lambda,
			&cloth.constraints
		};

		std::vector< VkDescriptorBufferInfo > writesInfo( cloth.descriptorCount );
		std::vector< VkWriteDescriptorSet > writes( cloth.descriptorCount );
		for (uint32_t index = 0; index < cloth.descriptorCount; index++) {
			writesInfo[index] = {
				.buffer = fields[index]->buffer,
				.offset = 0,
				.range = VK_WHOLE_SIZE
			};

			writes[index] = {
				.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
				.dstSet = cloth.descriptorSet,
				.dstBinding = index,
				.descriptorCount = 1,
				.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
				.pBufferInfo = &writesInfo[index]
			};
		}

		vkUpdateDescriptorSets(deviceIF.logical.device, writes.size(), writes.data(), 0, nullptr);
	}

	void populateCloth() {
		int subDivision = 20;
		float length = 1.0;
		float width = 0.5;

		float segmentLength = length / subDivision;
		float segmentWidth = width / subDivision;

		for( uint32_t i = 0; i <= subDivision; i++ ){
			for (uint32_t j = 0; j <= subDivision; j++) {
				cloth.positionData.push_back(glm::vec4{
					segmentWidth * i - width * 0.5,
					segmentLength * j - length * 0.5,
					0.0,
					1.0
				});

				if (i == 0) {
					cloth.massData.push_back(0.0f);
				}
				else {
					cloth.massData.push_back(1.0f);
				}
			}
		}

		for (uint32_t i = 0; i < subDivision; i++) {
			for (uint32_t j = 0; j < subDivision; j++) {

				uint32_t rowStart = i * (subDivision + 1);
				uint32_t nextRowStart = (i + 1) * (subDivision + 1);

				uint32_t topLeft = rowStart + j;
				uint32_t topRight = topLeft + 1;
				uint32_t bottomLeft = nextRowStart + j;
				uint32_t bottomRight = bottomLeft + 1;

				cloth.indexData.push_back(topLeft);
				cloth.indexData.push_back(topRight);
				cloth.indexData.push_back(bottomLeft);

				cloth.indexData.push_back(topRight);
				cloth.indexData.push_back(bottomRight);
				cloth.indexData.push_back(bottomLeft);

				cloth.constraintsData.push_back({
					topLeft, topRight, segmentWidth
				});

				cloth.constraintsData.push_back({
					topLeft, bottomLeft, segmentLength
				});

				if (j == subDivision - 1) {
					cloth.constraintsData.push_back({
						topRight, bottomRight, segmentLength
					});
				}

				if (i == subDivision - 1) {
					cloth.constraintsData.push_back({
						bottomLeft, bottomRight, segmentWidth
					});
				}
			}
		}

		cloth.velocityData.resize(cloth.positionData.size(), glm::vec4(0, 0, 0, 0));
		cloth.lambdaData.resize(cloth.positionData.size(), 0.0);
	}

	void setupCloth() {
		setupDescriptorSet();
		populateCloth();

		setupBuffer( cloth.positionData.data(), cloth.positionData.size() * sizeof(glm::vec4), cloth.position, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
		setupBuffer( cloth.positionData.data(), cloth.positionData.size() * sizeof(glm::vec4), cloth.predictedPosition, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
		setupBuffer( cloth.massData.data(), cloth.massData.size() * sizeof(float), cloth.mass, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
		setupBuffer( cloth.velocityData.data(), cloth.velocityData.size() * sizeof(glm::vec4), cloth.velocity, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
		setupBuffer( cloth.lambdaData.data(), cloth.lambdaData.size() * sizeof(float), cloth.lambda, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
		setupBuffer( cloth.constraintsData.data(), cloth.constraintsData.size() * sizeof(Constraints), cloth.constraints, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
		setupBuffer(cloth.indexData.data(), cloth.indexData.size() * sizeof( uint32_t ), cloth.indices, VK_BUFFER_USAGE_INDEX_BUFFER_BIT );

		updateDescriptors();
	}
	
	void setup() {
		setupLibraries();
		setupInstance();
		pickPhysicalDeviceAndQueueIndex();
		setupLogicalDevice();
		setupVMA();
		setupWindowAndSurface();
		setupSwapchain();
		setupSync2();
		setupCommandBuffers();
		loadAndCompileShaders();
		setupCloth();
		setupPipeline();
		animate();
	}

private:
	struct ExtensionsIF {
		struct Instance {
			uint32_t count;
			const char* const* extensions;
		} instance;

		struct Device {
			const std::vector< const char* > extensions{
				VK_KHR_SWAPCHAIN_EXTENSION_NAME
			};
		} device;
	} extensionsIF;

	struct WindowIF {
		VkInstance vkInstance;

		SDL_Window* window;
		std::string windowName = "Nuno";
		int windowWidth = 100;
		int windowHeight = 100;
		SDL_WindowFlags windowFlags{
			SDL_WINDOW_VULKAN | SDL_WINDOW_FULLSCREEN
		};

		glm::ivec2 dimensions{};

		VkSurfaceKHR surface;
		VkSurfaceCapabilitiesKHR surfaceCapabilities;

		VkClearColorValue clearColor{ 0.0, 0.0, 0.0 };
	} windowIF;

	struct DeviceIF {
		struct physical {
			uint32_t count;
			std::vector< VkPhysicalDevice > devices;
			std::vector< VkPhysicalDeviceProperties2 > properties;
			uint32_t index;
			VkPhysicalDevice device;
		} physical;

		struct Queue {
			VkQueue queue;
			std::vector < std::vector< VkQueueFamilyProperties2 > > properties;
			uint32_t index;
		} queue;

		struct Logical {
			VkDevice device;
			VkCommandPool commandPool;

			struct SwapchainConfiguration {
				VkSwapchainCreateInfoKHR swapchainCreateInfo;
				VkSwapchainKHR swapchain;
				std::vector<VkImage> swapchainImages;
				std::vector<VkImageView> swapchainImageViews;
				VkColorSpaceKHR imageColorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR;
				VkFormat imageFormat = VK_FORMAT_B8G8R8A8_SRGB;

				bool updateSwapchain{ false };
				uint32_t imageCount;

				static constexpr uint32_t maxFramesInFlight{ 2 };
				std::array< VkFence, maxFramesInFlight > fences;
				std::array< VkSemaphore, maxFramesInFlight > presentSemaphore;
				std::vector< VkSemaphore > renderSemaphore;
				std::array< VkCommandBuffer, maxFramesInFlight > commandBuffers{};
			} swapchainConfiguration;
		} logical;
	} deviceIF;

	VmaAllocator vmaAllocator;

	Slang::ComPtr< slang::IGlobalSession > slangGlobalSession;
	VkShaderModule shaderModule{};

	int frameIndex = 0;
	uint32_t imageIndex = 0;

	struct Pipeline {
		VkPipelineLayout pipelineLayout;
		VkPipeline pipeline;
	} graphicsPipeline, predictionPipeline;

	VkDescriptorPool descriptorPool;

	struct Field {
		VkBuffer buffer;
		VmaAllocation allocation;
		VmaAllocationInfo allocationInfo;
	};

	struct Constraints {
		uint32_t a;
		uint32_t b;

		float restLength;
		float padding;
	};

	struct Cloth {
		Field position;
		Field predictedPosition;
		Field velocity;
		Field mass;
		Field lambda;

		Field constraints;
		Field indices;

		int descriptorCount = 3;
		VkDescriptorSet descriptorSet;
		VkDescriptorSetLayout descriptorSetLayout;

		//cpu side data initialization
		std::vector< glm::vec4 > positionData;
		std::vector< uint32_t > indexData;
		std::vector< float > massData;
		std::vector< Constraints > constraintsData;
		std::vector< glm::vec4 > velocityData;
		std::vector< float > lambdaData;
	} cloth;
};

int main() {
	Renderer renderer;
	renderer.setup();
}