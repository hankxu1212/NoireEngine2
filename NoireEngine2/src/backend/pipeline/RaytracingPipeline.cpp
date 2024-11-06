#include "RaytracingPipeline.hpp"

#include "backend/VulkanContext.hpp"
#include "backend/shader/VulkanShader.h"
#include "renderer/scene/SceneManager.hpp"
#include "backend/pipeline/VulkanGraphicsPipelineBuilder.hpp"
#include "utils/Logger.hpp"
#include "renderer/object/Mesh.hpp"
#include "backend/raytracing/RaytracingBuilderKHR.hpp"
#include "core/Bitmap.hpp"

#pragma warning (disable:4702)

RaytracingPipeline::RaytracingPipeline(ObjectPipeline* objectPipeline) :
	p_ObjectPipeline(objectPipeline)
{
	VkDevice device = VulkanContext::GetDevice();

	// Get properties and features
	rayTracingPipelineProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR;
	VkPhysicalDeviceProperties2 deviceProperties2{};
	deviceProperties2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
	deviceProperties2.pNext = &rayTracingPipelineProperties;
	vkGetPhysicalDeviceProperties2(*VulkanContext::Get()->getPhysicalDevice(), &deviceProperties2);
	
	accelerationStructureFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
	
	VkPhysicalDeviceFeatures2 deviceFeatures2{};
	deviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
	deviceFeatures2.pNext = &accelerationStructureFeatures;
	vkGetPhysicalDeviceFeatures2(*VulkanContext::Get()->getPhysicalDevice(), &deviceFeatures2);

	// Get the function pointers required for ray tracing
	vkGetBufferDeviceAddressKHR = reinterpret_cast<PFN_vkGetBufferDeviceAddressKHR>(vkGetDeviceProcAddr(device, "vkGetBufferDeviceAddressKHR"));
	vkCmdBuildAccelerationStructuresKHR = reinterpret_cast<PFN_vkCmdBuildAccelerationStructuresKHR>(vkGetDeviceProcAddr(device, "vkCmdBuildAccelerationStructuresKHR"));
	vkBuildAccelerationStructuresKHR = reinterpret_cast<PFN_vkBuildAccelerationStructuresKHR>(vkGetDeviceProcAddr(device, "vkBuildAccelerationStructuresKHR"));
	vkCreateAccelerationStructureKHR = reinterpret_cast<PFN_vkCreateAccelerationStructureKHR>(vkGetDeviceProcAddr(device, "vkCreateAccelerationStructureKHR"));
	vkDestroyAccelerationStructureKHR = reinterpret_cast<PFN_vkDestroyAccelerationStructureKHR>(vkGetDeviceProcAddr(device, "vkDestroyAccelerationStructureKHR"));
	vkGetAccelerationStructureBuildSizesKHR = reinterpret_cast<PFN_vkGetAccelerationStructureBuildSizesKHR>(vkGetDeviceProcAddr(device, "vkGetAccelerationStructureBuildSizesKHR"));
	vkGetAccelerationStructureDeviceAddressKHR = reinterpret_cast<PFN_vkGetAccelerationStructureDeviceAddressKHR>(vkGetDeviceProcAddr(device, "vkGetAccelerationStructureDeviceAddressKHR"));
	vkCmdTraceRaysKHR = reinterpret_cast<PFN_vkCmdTraceRaysKHR>(vkGetDeviceProcAddr(device, "vkCmdTraceRaysKHR"));
	vkGetRayTracingShaderGroupHandlesKHR = reinterpret_cast<PFN_vkGetRayTracingShaderGroupHandlesKHR>(vkGetDeviceProcAddr(device, "vkGetRayTracingShaderGroupHandlesKHR"));
	vkCreateRayTracingPipelinesKHR = reinterpret_cast<PFN_vkCreateRayTracingPipelinesKHR>(vkGetDeviceProcAddr(device, "vkCreateRayTracingPipelinesKHR"));
	vkCmdWriteAccelerationStructuresPropertiesKHR = reinterpret_cast<PFN_vkCmdWriteAccelerationStructuresPropertiesKHR>(vkGetDeviceProcAddr(device, "vkCmdWriteAccelerationStructuresPropertiesKHR"));
	vkCmdCopyAccelerationStructureKHR = reinterpret_cast<PFN_vkCmdCopyAccelerationStructureKHR>(vkGetDeviceProcAddr(device, "vkCmdCopyAccelerationStructureKHR"));

	assert(vkGetBufferDeviceAddressKHR != nullptr);
	assert(vkCreateAccelerationStructureKHR != nullptr);
	assert(vkDestroyAccelerationStructureKHR != nullptr);
	assert(vkGetAccelerationStructureBuildSizesKHR != nullptr);
	assert(vkGetAccelerationStructureDeviceAddressKHR != nullptr);
	assert(vkBuildAccelerationStructuresKHR != nullptr);
	assert(vkCmdBuildAccelerationStructuresKHR != nullptr);
	assert(vkCmdTraceRaysKHR != nullptr);
	assert(vkGetRayTracingShaderGroupHandlesKHR != nullptr);
	assert(vkCreateRayTracingPipelinesKHR != nullptr);
	assert(vkCmdWriteAccelerationStructuresPropertiesKHR != nullptr);
	assert(vkCmdCopyAccelerationStructureKHR != nullptr);
}

RaytracingPipeline::~RaytracingPipeline()
{
	m_DescriptorAllocator.Cleanup(); // destroy pool and sets

	m_RTBuilder.Destroy();
	m_rtSBTBuffer.Destroy();

	if (m_PipelineLayout != VK_NULL_HANDLE) {
		vkDestroyPipelineLayout(VulkanContext::GetDevice(), m_PipelineLayout, nullptr);
		m_PipelineLayout = VK_NULL_HANDLE;
	}

	if (m_Pipeline != VK_NULL_HANDLE) {
		vkDestroyPipeline(VulkanContext::GetDevice(), m_Pipeline, nullptr);
		m_Pipeline = VK_NULL_HANDLE;
	}

}

void RaytracingPipeline::CreatePipeline()
{
	// Create the acceleration structures used to render the ray traced scene
	CreateBottomLevelAccelerationStructure();
	CreateTopLevelAccelerationStructure();
	CreateStorageImage();
	CreateDescriptorSets();
	CreateRayTracingPipeline();
	CreateShaderBindingTables();
	return;
	CreateUniformBuffer();
}

void RaytracingPipeline::Render(const Scene* scene, const CommandBuffer& commandBuffer)
{
}

void RaytracingPipeline::Prepare(const Scene* scene, const CommandBuffer& commandBuffer)
{
}

ScratchBuffer RaytracingPipeline::CreateScratchBuffer(VkDeviceSize size)
{
	ScratchBuffer scratchBuffer;

	VkMemoryAllocateFlagsInfoKHR flags_info{ VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO_KHR };
	flags_info.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT_KHR;

	scratchBuffer.buffer = Buffer(
		size, 
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 
		Buffer::Unmapped, 
		&flags_info
	);

	VkBufferDeviceAddressInfoKHR bufferDeviceAddresInfo{};
	bufferDeviceAddresInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
	bufferDeviceAddresInfo.buffer = scratchBuffer.buffer.getBuffer();
	scratchBuffer.deviceAddress = vkGetBufferDeviceAddressKHR(VulkanContext::GetDevice(), &bufferDeviceAddresInfo);

	return scratchBuffer;
}

void RaytracingPipeline::CreateAccelerationStructure(AccelerationStructure& accelerationStructure, VkAccelerationStructureTypeKHR type, VkAccelerationStructureBuildSizesInfoKHR buildSizeInfo)
{
	VkMemoryAllocateFlagsInfoKHR flags_info{ VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO_KHR };
	flags_info.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT_KHR;

	// Allocating the buffer to hold the acceleration structure
	accelerationStructure.buffer = Buffer(
		buildSizeInfo.accelerationStructureSize,
		VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		Buffer::Unmapped,
		&flags_info
	);

	// Acceleration structure
	VkAccelerationStructureCreateInfoKHR accelerationStructureCreate_info{};
	accelerationStructureCreate_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
	accelerationStructureCreate_info.buffer = accelerationStructure.buffer.getBuffer();
	accelerationStructureCreate_info.size = buildSizeInfo.accelerationStructureSize;
	accelerationStructureCreate_info.type = type;
	vkCreateAccelerationStructureKHR(VulkanContext::GetDevice(), &accelerationStructureCreate_info, nullptr, &accelerationStructure.handle);

	// AS device address
	VkAccelerationStructureDeviceAddressInfoKHR accelerationDeviceAddressInfo{};
	accelerationDeviceAddressInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
	accelerationDeviceAddressInfo.accelerationStructure = accelerationStructure.handle;
	accelerationStructure.deviceAddress = vkGetAccelerationStructureDeviceAddressKHR(VulkanContext::GetDevice(), &accelerationDeviceAddressInfo);
}

AccelerationStructure RaytracingPipeline::CreateAccelerationStructure(const VkAccelerationStructureCreateInfoKHR& createInfo)
{
	AccelerationStructure accelerationStructure;

	VkMemoryAllocateFlagsInfoKHR flags_info{ VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO_KHR };
	flags_info.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT_KHR;

	// Allocating the buffer to hold the acceleration structure
	accelerationStructure.buffer = Buffer(
		createInfo.size,
		VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		Buffer::Unmapped,
		&flags_info
	);

	// Setting the buffer
	VkAccelerationStructureCreateInfoKHR accel = createInfo;
	accel.buffer = accelerationStructure.buffer.getBuffer();

	// Create the acceleration structure
	vkCreateAccelerationStructureKHR(VulkanContext::GetDevice(), &accel, nullptr, &accelerationStructure.handle);

	VkAccelerationStructureDeviceAddressInfoKHR info{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR };
	info.accelerationStructure = accelerationStructure.handle;
	accelerationStructure.deviceAddress = vkGetAccelerationStructureDeviceAddressKHR(VulkanContext::GetDevice(), &info);

	return accelerationStructure;
}

void RaytracingPipeline::DeleteAccelerationStructure(AccelerationStructure& accelerationStructure)
{
	accelerationStructure.buffer.Destroy();
	vkDestroyAccelerationStructureKHR(VulkanContext::GetDevice(), accelerationStructure.handle, nullptr);
}

uint64_t RaytracingPipeline::GetBufferDeviceAddress(VkBuffer buffer)
{
	VkBufferDeviceAddressInfoKHR bufferDeviceAI{};
	bufferDeviceAI.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
	bufferDeviceAI.buffer = buffer;
	return vkGetBufferDeviceAddressKHR(VulkanContext::GetDevice(), &bufferDeviceAI);
}

VkStridedDeviceAddressRegionKHR RaytracingPipeline::GetSbtEntryStridedDeviceAddressRegion(VkBuffer buffer, uint32_t handleCount)
{
	const uint32_t handleSizeAligned = alignedSize(rayTracingPipelineProperties.shaderGroupHandleSize, rayTracingPipelineProperties.shaderGroupHandleAlignment);
	VkStridedDeviceAddressRegionKHR stridedDeviceAddressRegionKHR{};
	stridedDeviceAddressRegionKHR.deviceAddress = GetBufferDeviceAddress(buffer);
	stridedDeviceAddressRegionKHR.stride = handleSizeAligned;
	stridedDeviceAddressRegionKHR.size = handleCount * handleSizeAligned;

	return stridedDeviceAddressRegionKHR;
}

RaytracingBuilderKHR::BlasInput MeshToGeometry(const Mesh* mesh)
{
	VkDeviceOrHostAddressConstKHR vertexBufferDeviceAddress{};
	VkDeviceOrHostAddressConstKHR indexBufferDeviceAddress{};

	vertexBufferDeviceAddress.deviceAddress = RaytracingPipeline::GetBufferDeviceAddress(mesh->getVertexBuffer().getBuffer());
	indexBufferDeviceAddress.deviceAddress = RaytracingPipeline::GetBufferDeviceAddress(mesh->getIndexBuffer().getBuffer());

	uint32_t numTriangles = mesh->getIndexCount() / 3;

	// Build
	VkAccelerationStructureGeometryKHR accelerationStructureGeometry{};
	accelerationStructureGeometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
	accelerationStructureGeometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
	accelerationStructureGeometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
	accelerationStructureGeometry.geometry.triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
	accelerationStructureGeometry.geometry.triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
	accelerationStructureGeometry.geometry.triangles.vertexData = vertexBufferDeviceAddress;
	accelerationStructureGeometry.geometry.triangles.maxVertex = mesh->getVertexCount() - 1;
	accelerationStructureGeometry.geometry.triangles.vertexStride = sizeof(Mesh::Vertex);
	accelerationStructureGeometry.geometry.triangles.indexType = VK_INDEX_TYPE_UINT32;
	accelerationStructureGeometry.geometry.triangles.indexData = indexBufferDeviceAddress;
	accelerationStructureGeometry.geometry.triangles.transformData.deviceAddress = 0;
	accelerationStructureGeometry.geometry.triangles.transformData.hostAddress = nullptr;

	// The entire array will be used to build the BLAS.
	VkAccelerationStructureBuildRangeInfoKHR offset;
	offset.firstVertex = 0;
	offset.primitiveCount = numTriangles;
	offset.primitiveOffset = 0;
	offset.transformOffset = 0;

	// Our blas is made from only one geometry, but could be made of many geometries
	RaytracingBuilderKHR::BlasInput input;
	input.asGeometry.emplace_back(accelerationStructureGeometry);
	input.asBuildOffsetInfo.emplace_back(offset);

	return input;
}

void RaytracingPipeline::CreateBottomLevelAccelerationStructure()
{
	const auto& allMeshes = Resources::Get()->FindAllOfType<Mesh>();
	std::vector<RaytracingBuilderKHR::BlasInput> allBlas;

	uint32_t id = 0;

	for (const auto& [node, resource] : allMeshes)
	{
		std::shared_ptr<Mesh> mesh = std::dynamic_pointer_cast<Mesh>(resource);

		mesh->SetBlasID(id++); // incremental blas id

		allBlas.emplace_back(MeshToGeometry(mesh.get()));
	}
	m_RTBuilder.BuildBlas(allBlas, VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR);

	NE_DEBUG(std::format("Built {} bottom level triangle geometries", allBlas.size()), Logger::CYAN, Logger::BOLD);
}

void RaytracingPipeline::CreateTopLevelAccelerationStructure()
{
	std::vector<VkAccelerationStructureInstanceKHR> tlas;
	const auto& allInstances = SceneManager::Get()->getScene()->getObjectInstances();

	uint32_t totalSize = 0;
	for (int workflowIndex = 0; workflowIndex < allInstances.size(); ++workflowIndex)
		totalSize += (uint32_t)allInstances[workflowIndex].size();
	tlas.reserve(totalSize);

	for (int workflowIndex = 0; workflowIndex < allInstances.size(); ++workflowIndex)
	{
		const auto& workflowInstances = allInstances[workflowIndex];
		uint32_t instanceCount = (uint32_t)workflowInstances.size();
		for (uint32_t i = 0; i < instanceCount; i++)
		{
			VkAccelerationStructureInstanceKHR rayInst{};
			rayInst.transform = toTransformMatrixKHR(workflowInstances[i].m_TransformUniform.modelMatrix); // Position of the instance
			rayInst.instanceCustomIndex = workflowInstances[i].mesh->getBlasID(); // gl_InstanceCustomIndexEXT
			rayInst.accelerationStructureReference = m_RTBuilder.getBlasDeviceAddress(workflowInstances[i].mesh->getBlasID());
			rayInst.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
			rayInst.mask = 0xFF; //  Only be hit if rayMask & instance.mask != 0
			rayInst.instanceShaderBindingTableRecordOffset = 0; // We will use the same hit group for all objects
			tlas.emplace_back(rayInst);
		}
	}

	m_RTBuilder.BuildTlas(tlas, VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR);

	NE_DEBUG(std::format("Built {} top level instances", totalSize), Logger::CYAN, Logger::BOLD);
}

void RaytracingPipeline::CreateStorageImage()
{
	const SwapChain* swapchain = VulkanContext::Get()->getSwapChain();

	glm::uvec2 extent = swapchain->getExtentVec2();
	//VkFormat format = VulkanContext::Get()->getSurface()->getFormat().format;
	VkFormat format = VK_FORMAT_R32G32B32A32_SFLOAT;

	m_RtxImage = std::make_unique<Image2D>(
		extent.x, extent.y, format, 
		VK_IMAGE_LAYOUT_GENERAL,
		VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT,
		false
	);

	m_RtxImage->Load();
}

void RaytracingPipeline::CreateUniformBuffer()
{
}

void RaytracingPipeline::CreateRayTracingPipeline()
{
	enum StageIndices
	{
		eRaygen,
		eMiss,
		eClosestHit,
		eShaderGroupCount
	};

	// load all shaders
	VulkanShader raygenModule("../spv/shaders/raytracing/raytrace.rgen.spv", VulkanShader::ShaderStage::RTX_Raygen);
	VulkanShader missModule("../spv/shaders/raytracing/raytrace.rmiss.spv", VulkanShader::ShaderStage::RTX_Miss);
	VulkanShader closesthitModule("../spv/shaders/raytracing/raytrace.rchit.spv", VulkanShader::ShaderStage::RTX_CHit);

	std::array<VkPipelineShaderStageCreateInfo, eShaderGroupCount> stages{};
	VkPipelineShaderStageCreateInfo stage{ VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };

	stages[eRaygen] = raygenModule.shaderStage();
	stages[eMiss] = missModule.shaderStage();
	stages[eClosestHit] = closesthitModule.shaderStage();

	// Shader groups
	VkRayTracingShaderGroupCreateInfoKHR group{ VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR };
	group.anyHitShader = VK_SHADER_UNUSED_KHR;
	group.closestHitShader = VK_SHADER_UNUSED_KHR;
	group.generalShader = VK_SHADER_UNUSED_KHR;
	group.intersectionShader = VK_SHADER_UNUSED_KHR;

	// Raygen
	group.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
	group.generalShader = eRaygen;
	m_RTShaderGroups.push_back(group);

	// Miss
	group.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
	group.generalShader = eMiss;
	m_RTShaderGroups.push_back(group);

	// closest hit shader
	group.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
	group.generalShader = VK_SHADER_UNUSED_KHR;
	group.closestHitShader = eClosestHit;
	m_RTShaderGroups.push_back(group);

	// Push constant: we want to be able to update constants used by the shaders
	VkPushConstantRange pushConstant{ VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_MISS_BIT_KHR,
									 0, sizeof(PushConstantRay) };

	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{ VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
	pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
	pipelineLayoutCreateInfo.pPushConstantRanges = &pushConstant;

	// Descriptor sets: one specific to ray tracing, and one shared with the rasterization pipeline
	std::vector<VkDescriptorSetLayout> rtDescSetLayouts = { 
		set0_layout, // rtx tlas and out image
		p_ObjectPipeline->set0_WorldLayout,
		p_ObjectPipeline->set1_StorageBuffersLayout,
		p_ObjectPipeline->set2_TexturesLayout,
		p_ObjectPipeline->set3_CubemapLayout,
		p_ObjectPipeline->set4_ShadowMapLayout,
	};

	pipelineLayoutCreateInfo.setLayoutCount = static_cast<uint32_t>(rtDescSetLayouts.size());
	pipelineLayoutCreateInfo.pSetLayouts = rtDescSetLayouts.data();

	vkCreatePipelineLayout(VulkanContext::GetDevice(), &pipelineLayoutCreateInfo, nullptr, &m_PipelineLayout);


	// Assemble the shader stages and recursion depth info into the ray tracing pipeline
	VkRayTracingPipelineCreateInfoKHR rayPipelineInfo{ VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR };
	rayPipelineInfo.stageCount = static_cast<uint32_t>(stages.size());  // Stages are shaders
	rayPipelineInfo.pStages = stages.data();

	// In this case, m_RTShaderGroups.size() == 3: we have one raygen group,
	// one miss shader group, and one hit group.
	rayPipelineInfo.groupCount = static_cast<uint32_t>(m_RTShaderGroups.size());
	rayPipelineInfo.pGroups = m_RTShaderGroups.data();

	rayPipelineInfo.maxPipelineRayRecursionDepth = 1;  // Ray depth
	rayPipelineInfo.layout = m_PipelineLayout;

	vkCreateRayTracingPipelinesKHR(VulkanContext::GetDevice(), {}, {}, 1, & rayPipelineInfo, nullptr, & m_Pipeline);

	NE_DEBUG("Built rtx pipeline", Logger::CYAN, Logger::BOLD);
}

void RaytracingPipeline::CreateShaderBindingTables()
{
	uint32_t missCount = 1;
	uint32_t hitCount = 1;
	uint32_t handleCount = /*raygen: always 1*/1 + missCount + hitCount;
	uint32_t handleSize = rayTracingPipelineProperties.shaderGroupHandleSize;

	const uint32_t handleSizeAligned = alignedSize(rayTracingPipelineProperties.shaderGroupHandleSize, rayTracingPipelineProperties.shaderGroupHandleAlignment);

	m_rgenRegion.stride = alignedSize(handleSizeAligned, rayTracingPipelineProperties.shaderGroupBaseAlignment);
	m_rgenRegion.size = m_rgenRegion.stride;  // The size member of pRayGenShaderBindingTable must be equal to its stride member

	m_missRegion.stride = handleSizeAligned;
	m_missRegion.size = alignedSize(missCount * handleSizeAligned, rayTracingPipelineProperties.shaderGroupBaseAlignment);

	m_hitRegion.stride = handleSizeAligned;
	m_hitRegion.size = alignedSize(hitCount * handleSizeAligned, rayTracingPipelineProperties.shaderGroupBaseAlignment);

	// Get the shader group handles
	uint32_t dataSize = handleCount * handleSize;
	std::vector<uint8_t> handles(dataSize);
	VulkanContext::VK_CHECK(vkGetRayTracingShaderGroupHandlesKHR(VulkanContext::GetDevice(), m_Pipeline, 0, handleCount, dataSize, handles.data()));

	// Allocate a buffer for storing the SBT.
	VkMemoryAllocateFlagsInfoKHR flags_info{ VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO_KHR };
	flags_info.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT_KHR;

	VkDeviceSize sbtSize = m_rgenRegion.size + m_missRegion.size + m_hitRegion.size + m_callRegion.size;
	m_rtSBTBuffer = Buffer(
		sbtSize,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT
		| VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		Buffer::Mapped,
		&flags_info
	);

	// Find the SBT addresses of each group
	VkBufferDeviceAddressInfo info{ VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO, nullptr, m_rtSBTBuffer.getBuffer() };
	VkDeviceAddress sbtAddress = vkGetBufferDeviceAddress(VulkanContext::GetDevice(), &info);
	m_rgenRegion.deviceAddress = sbtAddress;
	m_missRegion.deviceAddress = sbtAddress + m_rgenRegion.size;
	m_hitRegion.deviceAddress = sbtAddress + m_rgenRegion.size + m_missRegion.size;

	// Helper to retrieve the handle data
	auto getHandle = [&](int i) { return handles.data() + i * handleSize; };

	// Map the SBT buffer and write in the handles.
	auto* pSBTBuffer = reinterpret_cast<uint8_t*>(m_rtSBTBuffer.data());
	uint8_t* pData{ nullptr };
	uint32_t handleIdx{ 0 };

	// Raygen
	pData = pSBTBuffer;
	memcpy(pData, getHandle(handleIdx++), handleSize);

	// Miss
	pData = pSBTBuffer + m_rgenRegion.size;
	for (uint32_t c = 0; c < missCount; c++)
	{
		memcpy(pData, getHandle(handleIdx++), handleSize);
		pData += m_missRegion.stride;
	}

	// Hit
	pData = pSBTBuffer + m_rgenRegion.size + m_missRegion.size;
	for (uint32_t c = 0; c < hitCount; c++)
	{
		memcpy(pData, getHandle(handleIdx++), handleSize);
		pData += m_hitRegion.stride;
	}

	m_rtSBTBuffer.UnmapMemory();

	NE_DEBUG("Built RTX SBT", Logger::CYAN, Logger::BOLD);
}

void RaytracingPipeline::CreateDescriptorSets()
{
	std::vector<VkDescriptorPoolSize> poolSizes = 
	{
		{ VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, 1 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1 },
		//{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1 },
		//{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 2 }
	};

	m_DescriptorAllocator.SetCustomPoolParams(poolSizes, 100);

	VkDescriptorImageInfo rtxImageInfo = m_RtxImage->GetDescriptorInfo();

	auto as = m_RTBuilder.getAccelerationStructure();

	VkWriteDescriptorSetAccelerationStructureKHR descASInfo{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR };
	descASInfo.accelerationStructureCount = 1;
	descASInfo.pAccelerationStructures = &as;

	DescriptorBuilder::Start(VulkanContext::Get()->getDescriptorLayoutCache(), &m_DescriptorAllocator)
		.BindAccelerationStructure(RTXBindings::TLAS, descASInfo, VK_SHADER_STAGE_RAYGEN_BIT_KHR)
		.BindImage(RTXBindings::OutImage, &rtxImageInfo, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_RAYGEN_BIT_KHR)
		.Build(set0, set0_layout);
}
