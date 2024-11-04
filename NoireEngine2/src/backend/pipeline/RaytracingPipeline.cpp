#include "RaytracingPipeline.hpp"

#include "backend/VulkanContext.hpp"
#include "backend/shader/VulkanShader.h"
#include "renderer/scene/SceneManager.hpp"
#include "backend/pipeline/VulkanGraphicsPipelineBuilder.hpp"
#include "utils/Logger.hpp"
#include "renderer/object/Mesh.hpp"
#include "backend/raytracing/RaytracingBuilderKHR.hpp"

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

	//DeleteAccelerationStructure(bottomLevelAS);
	//DeleteAccelerationStructure(topLevelAS);

	//shaderBindingTables.raygen.Destroy();
	//shaderBindingTables.miss.Destroy();
	//shaderBindingTables.hit.Destroy();

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
	return;
	CreateTopLevelAccelerationStructure();
	//CreateStorageImage(swapChain.colorFormat, { width, height, 1 });
	CreateUniformBuffer();
	CreateRayTracingPipeline();
	CreateShaderBindingTables();
	CreateDescriptorSets();
	//buildCommandBuffers();
}

void RaytracingPipeline::Render(const Scene* scene, const CommandBuffer& commandBuffer)
{
}

void RaytracingPipeline::Prepare(const Scene* scene, const CommandBuffer& commandBuffer)
{
}

RaytracingPipeline::ScratchBuffer RaytracingPipeline::CreateScratchBuffer(VkDeviceSize size)
{
	ScratchBuffer scratchBuffer;

	VkMemoryAllocateFlagsInfoKHR flags_info{ VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO_KHR };
	flags_info.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT_KHR;

	scratchBuffer.buffer = Buffer(
		size, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
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

void RaytracingPipeline::CreateStorageImage(VkFormat format, VkExtent3D extent)
{
}

void RaytracingPipeline::DeleteStorageImage()
{
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

void RaytracingPipeline::CreateShaderBindingTable(ShaderBindingTable& shaderBindingTable, uint32_t handleCount)
{
	// Create buffer to hold all shader handles for the SBT
	shaderBindingTable.CreateBuffer(
		rayTracingPipelineProperties.shaderGroupHandleSize * handleCount,
		VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		Buffer::Mapped
	);

	// Get the strided address to be used when dispatching the rays
	shaderBindingTable.stridedDeviceAddressRegion = GetSbtEntryStridedDeviceAddressRegion(shaderBindingTable.getBuffer(), handleCount);
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

	for (const auto& [node, resource] : allMeshes)
	{
		std::shared_ptr<Mesh> mesh = std::dynamic_pointer_cast<Mesh>(resource);
		assert(mesh);

		allBlas.emplace_back(MeshToGeometry(mesh.get()));
	}
	m_RTBuilder.BuildBlas(allBlas, VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR);
}

void RaytracingPipeline::CreateTopLevelAccelerationStructure()
{
}

void RaytracingPipeline::CreateUniformBuffer()
{
}

void RaytracingPipeline::CreateRayTracingPipeline()
{
}

void RaytracingPipeline::CreateShaderBindingTables()
{
}

void RaytracingPipeline::CreateDescriptorSets()
{
}
