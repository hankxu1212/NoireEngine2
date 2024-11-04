#include "AccelerationStructureBuildData.h"

#include "backend/pipeline/RaytracingPipeline.hpp"

// Helper function to insert a memory barrier for acceleration structures
inline void accelerationStructureBarrier(VkCommandBuffer cmd, VkAccessFlags src, VkAccessFlags dst)
{
    VkMemoryBarrier barrier{ VK_STRUCTURE_TYPE_MEMORY_BARRIER };
    barrier.srcAccessMask = src;
    barrier.dstAccessMask = dst;
    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
        VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR, 0, 1, &barrier, 0, nullptr, 0, nullptr);
}

void AccelerationStructureBuildData::addGeometry(const VkAccelerationStructureGeometryKHR& asGeom,
    const VkAccelerationStructureBuildRangeInfoKHR& offset)
{
    asGeometry.push_back(asGeom);
    asBuildRangeInfo.push_back(offset);
}


void AccelerationStructureBuildData::addGeometry(const AccelerationStructureGeometryInfo& asGeom)
{
    asGeometry.push_back(asGeom.geometry);
    asBuildRangeInfo.push_back(asGeom.rangeInfo);
}


VkAccelerationStructureBuildSizesInfoKHR AccelerationStructureBuildData::finalizeGeometry(VkDevice device, VkBuildAccelerationStructureFlagsKHR flags)
{
    assert(asGeometry.size() > 0 && "No geometry added to Build Structure");
    assert(asType != VK_ACCELERATION_STRUCTURE_TYPE_MAX_ENUM_KHR && "Acceleration Structure Type not set");

    buildInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
    buildInfo.type = asType;
    buildInfo.flags = flags;
    buildInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
    buildInfo.srcAccelerationStructure = VK_NULL_HANDLE;
    buildInfo.dstAccelerationStructure = VK_NULL_HANDLE;
    buildInfo.geometryCount = static_cast<uint32_t>(asGeometry.size());
    buildInfo.pGeometries = asGeometry.data();
    buildInfo.ppGeometries = nullptr;
    buildInfo.scratchData.deviceAddress = 0;

    std::vector<uint32_t> maxPrimCount(asBuildRangeInfo.size());
    for (size_t i = 0; i < asBuildRangeInfo.size(); ++i)
    {
        maxPrimCount[i] = asBuildRangeInfo[i].primitiveCount;
    }

    RaytracingPipeline::vkGetAccelerationStructureBuildSizesKHR(device, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &buildInfo,
        maxPrimCount.data(), &sizeInfo);

    return sizeInfo;
}

VkAccelerationStructureCreateInfoKHR AccelerationStructureBuildData::makeCreateInfo() const
{
    assert(asType != VK_ACCELERATION_STRUCTURE_TYPE_MAX_ENUM_KHR && "Acceleration Structure Type not set");
    assert(sizeInfo.accelerationStructureSize > 0 && "Acceleration Structure Size not set");

    VkAccelerationStructureCreateInfoKHR createInfo{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR };
    createInfo.type = asType;
    createInfo.size = sizeInfo.accelerationStructureSize;

    return createInfo;
}

AccelerationStructureGeometryInfo AccelerationStructureBuildData::makeInstanceGeometry(size_t numInstances,
    VkDeviceAddress instanceBufferAddr)
{
    assert(asType == VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR && "Instance geometry can only be used with TLAS");

    // Describes instance data in the acceleration structure.
    VkAccelerationStructureGeometryInstancesDataKHR geometryInstances{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR };
    geometryInstances.data.deviceAddress = instanceBufferAddr;

    // Set up the geometry to use instance data.
    VkAccelerationStructureGeometryKHR geometry{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR };
    geometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
    geometry.geometry.instances = geometryInstances;

    // Specifies the number of primitives (instances in this case).
    VkAccelerationStructureBuildRangeInfoKHR rangeInfo{};
    rangeInfo.primitiveCount = static_cast<uint32_t>(numInstances);

    // Prepare and return geometry information.
    AccelerationStructureGeometryInfo result;
    result.geometry = geometry;
    result.rangeInfo = rangeInfo;

    return result;
}


void AccelerationStructureBuildData::cmdBuildAccelerationStructure(VkCommandBuffer cmd,
    VkAccelerationStructureKHR accelerationStructure,
    VkDeviceAddress scratchAddress)
{
    assert(asGeometry.size() == asBuildRangeInfo.size() && "asGeometry.size() != asBuildRangeInfo.size()");
    assert(accelerationStructure != VK_NULL_HANDLE && "Acceleration Structure not created, first call createAccelerationStructure");

    const VkAccelerationStructureBuildRangeInfoKHR* rangeInfo = asBuildRangeInfo.data();

    // Build the acceleration structure
    buildInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
    buildInfo.srcAccelerationStructure = VK_NULL_HANDLE;
    buildInfo.dstAccelerationStructure = accelerationStructure;
    buildInfo.scratchData.deviceAddress = scratchAddress;
    buildInfo.pGeometries = asGeometry.data();  // In case the structure was copied, we need to update the pointer

    RaytracingPipeline::vkCmdBuildAccelerationStructuresKHR(cmd, 1, &buildInfo, &rangeInfo);

    // Since the scratch buffer is reused across builds, we need a barrier to ensure one build
    // is finished before starting the next one.
    accelerationStructureBarrier(cmd, VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR, VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR);
}

void AccelerationStructureBuildData::cmdUpdateAccelerationStructure(VkCommandBuffer cmd,
    VkAccelerationStructureKHR accelerationStructure,
    VkDeviceAddress scratchAddress)
{
    assert(asGeometry.size() == asBuildRangeInfo.size() && "asGeometry.size() != asBuildRangeInfo.size()");
    assert(accelerationStructure != VK_NULL_HANDLE && "Acceleration Structure not created, first call createAccelerationStructure");

    const VkAccelerationStructureBuildRangeInfoKHR* rangeInfo = asBuildRangeInfo.data();

    // Build the acceleration structure
    buildInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_UPDATE_KHR;
    buildInfo.srcAccelerationStructure = accelerationStructure;
    buildInfo.dstAccelerationStructure = accelerationStructure;
    buildInfo.scratchData.deviceAddress = scratchAddress;
    buildInfo.pGeometries = asGeometry.data();

    RaytracingPipeline::vkCmdBuildAccelerationStructuresKHR(cmd, 1, &buildInfo, &rangeInfo);

    // Since the scratch buffer is reused across builds, we need a barrier to ensure one build
    // is finished before starting the next one.
    accelerationStructureBarrier(cmd, VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR, VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR);
}
