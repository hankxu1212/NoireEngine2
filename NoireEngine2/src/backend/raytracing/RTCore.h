#pragma once

#include <sstream>
#include <vulkan/vulkan_core.h>
#include "backend/buffers/Buffer.hpp"
#include "glm/glm.hpp"

// Convert a Mat4x4 to the matrix required by acceleration structures
inline VkTransformMatrixKHR ToTransformMatrixKHR(glm::mat4 matrix)
{
	// VkTransformMatrixKHR uses a row-major memory layout, while glm::mat4
	// uses a column-major memory layout. We transpose the matrix so we can
	// memcpy the matrix's data directly.
	glm::mat4            temp = glm::transpose(matrix);
	VkTransformMatrixKHR out_matrix;
	memcpy(&out_matrix, &temp, sizeof(VkTransformMatrixKHR));
	return out_matrix;
}

// Helper function to insert a memory barrier for acceleration structures
inline void AccelerationStructureBarrier(VkCommandBuffer cmd, VkAccessFlags src, VkAccessFlags dst)
{
	VkMemoryBarrier barrier{ VK_STRUCTURE_TYPE_MEMORY_BARRIER };
	barrier.srcAccessMask = src;
	barrier.dstAccessMask = dst;
	vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
		VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR, 0, 1, &barrier, 0, nullptr, 0, nullptr);
}

inline uint32_t AlignedSize(uint32_t value, uint32_t alignment)
{
	return (value + alignment - 1) & ~(alignment - 1);
}

inline size_t AlignedSize(size_t value, size_t alignment)
{
	return (value + alignment - 1) & ~(alignment - 1);
}

inline VkDeviceSize AlignedVkSize(VkDeviceSize value, VkDeviceSize alignment)
{
	return (value + alignment - 1) & ~(alignment - 1);
}

// Holds information for a ray tracing acceleration structure
struct AccelerationStructure
{
	VkAccelerationStructureKHR handle = VK_NULL_HANDLE;
	uint64_t deviceAddress = 0;
	Buffer buffer;
};

// Single Geometry information, multiple can be used in a single BLAS
struct AccelerationStructureGeometryInfo
{
	VkAccelerationStructureGeometryKHR       geometry{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR };
	VkAccelerationStructureBuildRangeInfoKHR rangeInfo{};
};

struct AccelerationStructureBuildData
{
	VkAccelerationStructureTypeKHR asType = VK_ACCELERATION_STRUCTURE_TYPE_MAX_ENUM_KHR;  // Mandatory to set

	// Collection of geometries for the acceleration structure.
	std::vector<VkAccelerationStructureGeometryKHR> asGeometry;

	// Build range information corresponding to each geometry.
	std::vector<VkAccelerationStructureBuildRangeInfoKHR> asBuildRangeInfo;

	// Build information required for acceleration structure.
	VkAccelerationStructureBuildGeometryInfoKHR buildInfo{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR };

	// Size information for acceleration structure build resources.
	VkAccelerationStructureBuildSizesInfoKHR sizeInfo{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR };

	// Adds a geometry with its build range information to the acceleration structure.
	void AddGeometry(const VkAccelerationStructureGeometryKHR& asGeom, const VkAccelerationStructureBuildRangeInfoKHR& offset);
	void AddGeometry(const AccelerationStructureGeometryInfo& asGeom);

	AccelerationStructureGeometryInfo MakeInstanceGeometry(size_t numInstances, VkDeviceAddress instanceBufferAddr);

	// Configures the build information and calculates the necessary size information.
	VkAccelerationStructureBuildSizesInfoKHR FinalizeGeometry(VkDevice device, VkBuildAccelerationStructureFlagsKHR flags);

	// Creates an acceleration structure based on the current build and size info.
	VkAccelerationStructureCreateInfoKHR MakeCreateInfo() const;

	// Commands to build the acceleration structure in a Vulkan command buffer.
	void CmdBuildAccelerationStructure(VkCommandBuffer cmd, VkAccelerationStructureKHR accelerationStructure, VkDeviceAddress scratchAddress);

	// Commands to update the acceleration structure in a Vulkan command buffer.
	void CmdUpdateAccelerationStructure(VkCommandBuffer cmd, VkAccelerationStructureKHR accelerationStructure, VkDeviceAddress scratchAddress);

	// Checks if the compact flag is set for the build.
	inline bool hasCompactFlag() const { return buildInfo.flags & VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_COMPACTION_BIT_KHR; }
};

using ScratchBuffer = AddressedBuffer;
