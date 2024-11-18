#include "RaytracingContext.hpp"

#include "backend/VulkanContext.hpp"
#include "backend/shader/VulkanShader.h"
#include "renderer/scene/SceneManager.hpp"
#include "backend/pipeline/VulkanGraphicsPipelineBuilder.hpp"
#include "utils/Logger.hpp"
#include "renderer/object/Mesh.hpp"
#include "renderer/materials/Material.hpp"
#include "backend/raytracing/RaytracingBuilderKHR.hpp"
#include "core/Bitmap.hpp"

#pragma warning (disable:4702)

RaytracingContext::RaytracingContext()
{
	assert(!g_Instance);
	g_Instance = this;

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

RaytracingContext::~RaytracingContext()
{
	m_RTBuilder.Destroy();
}

void RaytracingContext::CreateAccelerationStructures()
{
	CreateBottomLevelAccelerationStructure();
	CreateTopLevelAccelerationStructure(false);
}

ScratchBuffer RaytracingContext::CreateScratchBuffer(VkDeviceSize size)
{
	ScratchBuffer scratchBuffer;

	scratchBuffer.buffer = Buffer(
		size, 
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 
		Buffer::Unmapped
	);

	scratchBuffer.deviceAddress = scratchBuffer.buffer.GetBufferDeviceAddress();

	return scratchBuffer;
}

void RaytracingContext::CreateAccelerationStructure(AccelerationStructure& accelerationStructure, VkAccelerationStructureTypeKHR type, VkAccelerationStructureBuildSizesInfoKHR buildSizeInfo)
{
	// Allocating the buffer to hold the acceleration structure
	accelerationStructure.buffer = Buffer(
		buildSizeInfo.accelerationStructureSize,
		VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
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

AccelerationStructure RaytracingContext::CreateAccelerationStructure(const VkAccelerationStructureCreateInfoKHR& createInfo)
{
	AccelerationStructure accelerationStructure;

	// Allocating the buffer to hold the acceleration structure
	accelerationStructure.buffer = Buffer(
		createInfo.size,
		VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		Buffer::Unmapped
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

void RaytracingContext::DeleteAccelerationStructure(AccelerationStructure& accelerationStructure)
{
	accelerationStructure.buffer.Destroy();
	vkDestroyAccelerationStructureKHR(VulkanContext::GetDevice(), accelerationStructure.handle, nullptr);
}

static RaytracingBuilderKHR::BlasInput MeshToGeometry(Mesh* mesh)
{
	VkDeviceOrHostAddressConstKHR vertexBufferDeviceAddress{ mesh->getVertexBufferAddress() };
	VkDeviceOrHostAddressConstKHR indexBufferDeviceAddress{ mesh->getIndexBufferAddress() };

	uint32_t numTriangles = mesh->getIndexCount() / 3;

	// Build
	VkAccelerationStructureGeometryKHR accelerationStructureGeometry{};
	accelerationStructureGeometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
	accelerationStructureGeometry.flags = VK_GEOMETRY_NO_DUPLICATE_ANY_HIT_INVOCATION_BIT_KHR;
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

void RaytracingContext::CreateBottomLevelAccelerationStructure()
{
	const auto& allMeshes = Resources::Get()->FindAllOfType<Mesh>();
	std::vector<RaytracingBuilderKHR::BlasInput> allBlas;

	for (const auto& resource : allMeshes)
	{
		std::shared_ptr<Mesh> mesh = std::dynamic_pointer_cast<Mesh>(resource);
		allBlas.emplace_back(MeshToGeometry(mesh.get()));
	}
	m_RTBuilder.BuildBlas(allBlas, VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR);

	NE_DEBUG(std::format("Built {} bottom level triangle geometries", allBlas.size()), Logger::CYAN, Logger::BOLD);
}

void RaytracingContext::CreateTopLevelAccelerationStructure(bool update)
{
	m_TlasBuildStructs.clear();
	const auto& allInstances = SceneManager::Get()->getScene()->getObjectInstances();

	uint32_t instanceIndex = 0;

	for (int workflowIndex = 0; workflowIndex < allInstances.size(); ++workflowIndex)
	{
		const auto& workflowInstances = allInstances[workflowIndex];
		for (uint32_t i = 0; i < workflowInstances.size(); i++)
		{
			VkAccelerationStructureInstanceKHR rayInst{};
			rayInst.transform = ToTransformMatrixKHR(workflowInstances[i].m_TransformUniform.modelMatrix); // Position of the instance
			rayInst.instanceCustomIndex = instanceIndex; // gl_InstanceCustomIndexEXT
			rayInst.accelerationStructureReference = m_RTBuilder.getBlasDeviceAddress(workflowInstances[i].mesh->getID());
			rayInst.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
			rayInst.mask = 0xFF; //  Only be hit if rayMask & instance.mask != 0
			rayInst.instanceShaderBindingTableRecordOffset = (uint32_t)workflowInstances[i].material->getWorkflow();

			m_TlasBuildStructs.emplace_back(rayInst);
			instanceIndex++;
		}
	}
	m_RTBuilder.BuildTlas(m_TlasBuildStructs, VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR | VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR, update);
}