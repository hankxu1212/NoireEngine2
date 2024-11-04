#pragma once

#include <mutex>
#include "AccelerationStructureBuildData.h"

// Ray tracing BLAS and TLAS builder
class RaytracingBuilderKHR
{
public:
    // Data used to build acceleration structure geometry
    struct BlasInput
    {
        std::vector<VkAccelerationStructureGeometryKHR>       asGeometry;
        std::vector<VkAccelerationStructureBuildRangeInfoKHR> asBuildOffsetInfo;
        VkBuildAccelerationStructureFlagsKHR                  flags{0};
    };

    // Destroying all allocations
    void Destroy();

    // Returning the constructed top-level acceleration structure
    VkAccelerationStructureKHR getAccelerationStructure() const { return m_tlas.handle; }

    // Return the Acceleration Structure Device Address of a BLAS Id
    VkDeviceAddress getBlasDeviceAddress(uint32_t blasId);

    // Create all the BLAS from the vector of BlasInput
    void BuildBlas(const std::vector<BlasInput>& input,
                    VkBuildAccelerationStructureFlagsKHR flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR);

    // Refit BLAS number blasIdx from updated buffer contents.
    void UpdateBlas(uint32_t blasIdx, BlasInput& blas, VkBuildAccelerationStructureFlagsKHR flags);

private:
    std::vector<AccelerationStructure> m_blas;  // Bottom-level acceleration structure
    AccelerationStructure              m_tlas;  // Top-level acceleration structure

    // Setup
    bool hasFlag(VkFlags item, VkFlags flag) { return (item & flag) == flag; }
};