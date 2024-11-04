#pragma once

#include <sstream>
#include <vulkan/vulkan_core.h>
#include "backend/buffers/Buffer.hpp"
#include "glm/glm.hpp"

#include "RTCore.h"

class BlasBuilder
{
public:
    BlasBuilder() = default;
    ~BlasBuilder();

    struct Stats
    {
        VkDeviceSize totalOriginalSize = 0;
        VkDeviceSize totalCompactSize = 0;

        std::string toString() const;
    };

    // Create the BLAS from the vector of BlasBuildData
    // Each BLAS will be created in sequence and share the same scratch buffer
    // Return true if ALL the BLAS were created within the budget
    // if not, this function needs to be called again until it returns true
    bool CmdCreateBlas(VkCommandBuffer cmd,
        std::vector<AccelerationStructureBuildData>& blasBuildData,   // List of the BLAS to build */
        std::vector<AccelerationStructure>& blasAccel,       // List of the acceleration structure
        VkDeviceAddress                              scratchAddress,  //  Address of the scratch buffer
        VkDeviceSize                                 hintMaxBudget = 512'000'000);

    // Create the BLAS from the vector of BlasBuildData in parallel
    // The advantage of this function is that it will try to build as many BLAS as possible in parallel
    // but it requires a scratch buffer per BLAS, or less but then each of them must large enough to hold the largest BLAS
    // This function needs to be called until it returns true
    bool CmdCreateParallelBlas(VkCommandBuffer cmd,
        std::vector<AccelerationStructureBuildData>& blasBuildData,
        std::vector<AccelerationStructure>& blasAccel,
        const std::vector<VkDeviceAddress>& scratchAddress,
        VkDeviceSize                                 hintMaxBudget = 512'000'000);

    // Compact the BLAS that have been built
    // Synchronization must be done by the application between the build and the compact
    void CmdCompactBlas(VkCommandBuffer cmd, 
        std::vector<AccelerationStructureBuildData>& blasBuildData, 
        std::vector<AccelerationStructure>& blasAccel);

    // Destroy the original BLAS that was compacted
    void destroyNonCompactedBlas();

    // Destroy all information
    void Destroy();

    // Return the statistics about the compacted BLAS
    Stats getStatistics() const { return m_stats; };

    // Scratch size strategy:
    // Find the maximum size of the scratch buffer needed for the BLAS build
    //
    // Strategy:
    // - Loop over all BLAS to find the maximum size
    // - If the max size is within the budget, return it. This will return as many addresses as there are BLAS
    // - Otherwise, return n*maxBlasSize, where n is the number of BLAS and maxBlasSize is the maximum size found for a single BLAS.
    //   In this case, fewer addresses will be returned than the number of BLAS, but each can be used to build any BLAS
    //
    // Usage
    // - Call this function to get the maximum size needed for the scratch buffer
    // - User allocate the scratch buffer with the size returned by this function
    // - Call getScratchAddresses to get the address for each BLAS
    //
    // Note: 128 is the default alignment for the scratch buffer
    //       (VkPhysicalDeviceAccelerationStructurePropertiesKHR::minAccelerationStructureScratchOffsetAlignment)
    VkDeviceSize getScratchSize(VkDeviceSize                                             hintMaxBudget,
        const std::vector<AccelerationStructureBuildData>& buildData,
        uint32_t                                                 minAlignment = 128) const;

    void getScratchAddresses(VkDeviceSize                                             hintMaxBudget,
        const std::vector<AccelerationStructureBuildData>& buildData,
        VkDeviceAddress                                          scratchBufferAddress,
        std::vector<VkDeviceAddress>& scratchAddresses,
        uint32_t                                                 minAlignment = 128);

private:
    void         DestroyQueryPool();
    void         CreateQueryPool(uint32_t maxBlasCount);
    void         InitializeQueryPoolIfNeeded(const std::vector<AccelerationStructureBuildData>& blasBuildData);
    VkDeviceSize BuildAccelerationStructures(VkCommandBuffer                              cmd,
        std::vector<AccelerationStructureBuildData>& blasBuildData,
        std::vector<AccelerationStructure>& blasAccel,
        const std::vector<VkDeviceAddress>& scratchAddress,
        VkDeviceSize                                 hintMaxBudget,
        VkDeviceSize                                 currentBudget,
        uint32_t& currentQueryIdx);

    VkQueryPool              m_queryPool = VK_NULL_HANDLE;
    uint32_t                 m_currentBlasIdx{ 0 };
    uint32_t                 m_currentQueryIdx{ 0 };

    std::vector<AccelerationStructure> m_cleanupBlasAccel;

    // Stats
    Stats m_stats;
};