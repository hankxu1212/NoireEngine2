#include "BLASBuilder.h"

#include "backend/VulkanContext.hpp"
#include <format>

BlasBuilder::~BlasBuilder()
{
    Destroy();
}

void BlasBuilder::CreateQueryPool(uint32_t maxBlasCount)
{
    VkQueryPoolCreateInfo qpci = { VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO };
    qpci.queryType = VK_QUERY_TYPE_ACCELERATION_STRUCTURE_COMPACTED_SIZE_KHR;
    qpci.queryCount = maxBlasCount;
    vkCreateQueryPool(VulkanContext::GetDevice(), &qpci, nullptr, &m_queryPool);
}

// This will build multiple BLAS serially, one after the other, ensuring that the process
// stays within the specified memory budget.
bool BlasBuilder::CmdCreateBlas(VkCommandBuffer cmd,
    std::vector<AccelerationStructureBuildData>& blasBuildData,
    std::vector<AccelerationStructure>& blasAccel,
    VkDeviceAddress                              scratchAddress,
    VkDeviceSize                                 hintMaxBudget)
{
    // It won't run in parallel, but will process all BLAS within the budget before returning
    return CmdCreateParallelBlas(cmd, blasBuildData, blasAccel, { scratchAddress }, hintMaxBudget);
}

// This function is responsible for building multiple Bottom-Level Acceleration Structures (BLAS) in parallel,
// ensuring that the process stays within the specified memory budget.
//
// Returns:
//   A boolean indicating whether all BLAS in the `blasBuildData` have been built by this function call.
//   Returns `true` if all BLAS were built, `false` otherwise.
bool BlasBuilder::CmdCreateParallelBlas(VkCommandBuffer cmd,
    std::vector<AccelerationStructureBuildData>& blasBuildData,
    std::vector<AccelerationStructure>& blasAccel,
    const std::vector<VkDeviceAddress>& scratchAddress,
    VkDeviceSize                                 hintMaxBudget)
{
    // Initialize the query pool if necessary to handle queries for properties of built acceleration structures.
    InitializeQueryPoolIfNeeded(blasBuildData);

    VkDeviceSize processBudget = 0;                  // Tracks the total memory used in the construction process.
    uint32_t     currentQueryIdx = m_currentQueryIdx;  // Local copy of the current query index.

    // Process each BLAS in the data vector while staying under the memory budget.
    while (m_currentBlasIdx < blasBuildData.size() && processBudget < hintMaxBudget)
    {
        // Build acceleration structures and accumulate the total memory used.
        processBudget += BuildAccelerationStructures(cmd, blasBuildData, blasAccel, scratchAddress, hintMaxBudget,
            processBudget, currentQueryIdx);
    }

    // Check if all BLAS have been built.
    return m_currentBlasIdx >= blasBuildData.size();
}


// Initializes a query pool for recording acceleration structure properties if necessary.
// This function ensures a query pool is available if any BLAS in the build data is flagged for compaction.
void BlasBuilder::InitializeQueryPoolIfNeeded(const std::vector<AccelerationStructureBuildData>& blasBuildData)
{
    if (!m_queryPool)
    {
        // Iterate through each BLAS build data element to check if the compaction flag is set.
        for (const auto& blas : blasBuildData)
        {
            if (blas.hasCompactFlag())
            {
                CreateQueryPool(static_cast<uint32_t>(blasBuildData.size()));
                break;
            }
        }
    }

    // If a query pool is now available (either newly created or previously existing),
    // reset the query pool to clear any old data or states.
    if (m_queryPool)
    {
        vkResetQueryPool(VulkanContext::GetDevice(), m_queryPool, 0, static_cast<uint32_t>(blasBuildData.size()));
    }
}

// Builds multiple Bottom-Level Acceleration Structures (BLAS) for a Vulkan ray tracing pipeline.
// This function manages memory budgets and submits the necessary commands to the specified command buffer.
//
// Parameters:
//   cmd            - Command buffer where acceleration structure commands are recorded.
//   blasBuildData  - Vector of data structures containing the geometry and other build-related information for each BLAS.
//   blasAccel      - Vector where the function will store the created acceleration structures.
//   scratchAddress - Vector of device addresses pointing to scratch memory required for the build process.
//   hintMaxBudget  - A hint for the maximum budget allowed for building acceleration structures.
//   currentBudget  - The current usage of the budget prior to this call.
//   currentQueryIdx - Reference to the current index for queries, updated during execution.
//
// Returns:
//   The total device size used for building the acceleration structures during this function call.
VkDeviceSize BlasBuilder::BuildAccelerationStructures(VkCommandBuffer cmd,
    std::vector<AccelerationStructureBuildData>& blasBuildData,
    std::vector<AccelerationStructure>& blasAccel,
    const std::vector<VkDeviceAddress>& scratchAddress,
    VkDeviceSize                                 hintMaxBudget,
    VkDeviceSize                                 currentBudget,
    uint32_t& currentQueryIdx)
{
    // Temporary vectors for storing build-related data
    std::vector<VkAccelerationStructureBuildGeometryInfoKHR> collectedBuildInfo;
    std::vector<VkAccelerationStructureKHR>                  collectedAccel;
    std::vector<VkAccelerationStructureBuildRangeInfoKHR*>   collectedRangeInfo;

    // Pre-allocate memory based on the number of BLAS to be built
    collectedBuildInfo.reserve(blasBuildData.size());
    collectedAccel.reserve(blasBuildData.size());
    collectedRangeInfo.reserve(blasBuildData.size());

    // Initialize the total budget used in this function call
    VkDeviceSize budgetUsed = 0;

    // Loop through BLAS data while there is scratch address space and budget available
    while (collectedBuildInfo.size() < scratchAddress.size() && currentBudget + budgetUsed < hintMaxBudget
        && m_currentBlasIdx < blasBuildData.size())
    {
        auto& data = blasBuildData[m_currentBlasIdx];
        VkAccelerationStructureCreateInfoKHR createInfo = data.MakeCreateInfo();

        // Create and store acceleration structure
        blasAccel[m_currentBlasIdx] = RaytracingContext::CreateAccelerationStructure(createInfo);
        collectedAccel.push_back(blasAccel[m_currentBlasIdx].handle);

        // Setup build information for the current BLAS
        data.buildInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
        data.buildInfo.srcAccelerationStructure = VK_NULL_HANDLE;
        data.buildInfo.dstAccelerationStructure = blasAccel[m_currentBlasIdx].handle;
        data.buildInfo.scratchData.deviceAddress = scratchAddress[m_currentBlasIdx % scratchAddress.size()];
        data.buildInfo.pGeometries = data.asGeometry.data();
        collectedBuildInfo.push_back(data.buildInfo);
        collectedRangeInfo.push_back(data.asBuildRangeInfo.data());

        // Update the used budget with the size of the current structure
        budgetUsed += data.sizeInfo.accelerationStructureSize;
        m_currentBlasIdx++;
    }

    // Command to build the acceleration structures on the GPU
    RaytracingContext::vkCmdBuildAccelerationStructuresKHR(cmd, static_cast<uint32_t>(collectedBuildInfo.size()), collectedBuildInfo.data(),
        collectedRangeInfo.data());

    // Barrier to ensure proper synchronization after building
    AccelerationStructureBarrier(cmd, VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR, VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR);

    // If a query pool is available, record the properties of the built acceleration structures
    if (m_queryPool)
    {
        RaytracingContext::vkCmdWriteAccelerationStructuresPropertiesKHR(cmd, static_cast<uint32_t>(collectedAccel.size()), collectedAccel.data(),
            VK_QUERY_TYPE_ACCELERATION_STRUCTURE_COMPACTED_SIZE_KHR, m_queryPool,
            currentQueryIdx);
        currentQueryIdx += static_cast<uint32_t>(collectedAccel.size());
    }

    // Return the total budget used in this operation
    return budgetUsed;
}


// Compacts the Bottom-Level Acceleration Structures (BLAS) that have been built, reducing their memory footprint.
// This function uses the results from previously performed queries to determine the compacted sizes and then
// creates new, smaller acceleration structures. It also handles copying from the original to the compacted structures.
//
// Notes:
//   It assumes that a query has been performed earlier to determine the possible compacted sizes of the acceleration structures.
//
void BlasBuilder::CmdCompactBlas(VkCommandBuffer cmd,
    std::vector<AccelerationStructureBuildData>& blasBuildData,
    std::vector<AccelerationStructure>& blasAccel)
{
    // Compute the number of queries that have been conducted between the current BLAS index and the query index.
    uint32_t queryCtn = m_currentBlasIdx - m_currentQueryIdx;
    // Ensure there is a valid query pool and BLAS to compact;
    if (m_queryPool == VK_NULL_HANDLE || queryCtn == 0)
        return;

    // Retrieve the compacted sizes from the query pool.
    std::vector<VkDeviceSize> compactSizes(queryCtn);
    vkGetQueryPoolResults(VulkanContext::GetDevice(), m_queryPool, m_currentQueryIdx, (uint32_t)compactSizes.size(),
        compactSizes.size() * sizeof(VkDeviceSize), compactSizes.data(), sizeof(VkDeviceSize),
        VK_QUERY_RESULT_WAIT_BIT);

    // Iterate through each BLAS index to process compaction.
    for (size_t i = m_currentQueryIdx; i < m_currentBlasIdx; i++)
    {
        size_t       idx = i - m_currentQueryIdx;  // Calculate local index for compactSizes vector.
        VkDeviceSize compactSize = compactSizes[idx];
        if (compactSize > 0)
        {
            // Update statistical tracking of sizes before and after compaction.
            m_stats.totalCompactSize += compactSize;
            m_stats.totalOriginalSize += blasBuildData[i].sizeInfo.accelerationStructureSize;
            blasBuildData[i].sizeInfo.accelerationStructureSize = compactSize;
            m_cleanupBlasAccel.push_back(blasAccel[i]);  // Schedule old BLAS for cleanup.

            // Create a new acceleration structure for the compacted BLAS.
            VkAccelerationStructureCreateInfoKHR asCreateInfo{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR };
            asCreateInfo.size = compactSize;
            asCreateInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
            blasAccel[i] = RaytracingContext::CreateAccelerationStructure(asCreateInfo);

            // Command to copy the original BLAS to the newly created compacted version.
            VkCopyAccelerationStructureInfoKHR copyInfo{ VK_STRUCTURE_TYPE_COPY_ACCELERATION_STRUCTURE_INFO_KHR };
            copyInfo.src = blasBuildData[i].buildInfo.dstAccelerationStructure;
            copyInfo.dst = blasAccel[i].handle;
            copyInfo.mode = VK_COPY_ACCELERATION_STRUCTURE_MODE_COMPACT_KHR;
            RaytracingContext::vkCmdCopyAccelerationStructureKHR(cmd, &copyInfo);

            // Update the build data to reflect the new destination of the BLAS.
            blasBuildData[i].buildInfo.dstAccelerationStructure = blasAccel[i].handle;
        }
    }

    // Update the query index to the current BLAS index, marking the end of processing for these structures.
    m_currentQueryIdx = m_currentBlasIdx;
}


void BlasBuilder::destroyNonCompactedBlas()
{
    for (auto& blas : m_cleanupBlasAccel)
    {
        RaytracingContext::DeleteAccelerationStructure(blas);
    }

    m_cleanupBlasAccel.clear();
}

void BlasBuilder::DestroyQueryPool()
{
    if (m_queryPool)
    {
        vkDestroyQueryPool(VulkanContext::GetDevice(), m_queryPool, nullptr);
        m_queryPool = VK_NULL_HANDLE;
    }
}

void BlasBuilder::Destroy()
{
    DestroyQueryPool();
    destroyNonCompactedBlas();
}

struct ScratchSizeInfo
{
    VkDeviceSize maxScratch;
    VkDeviceSize totalScratch;
};

ScratchSizeInfo calculateScratchAlignedSizes(const std::vector<AccelerationStructureBuildData>& buildData, uint32_t minAlignment)
{
    VkDeviceSize maxScratch{ 0 };
    VkDeviceSize totalScratch{ 0 };

    for (auto& buildInfo : buildData)
    {
        VkDeviceSize alignedSize = AlignedVkSize(buildInfo.sizeInfo.buildScratchSize, minAlignment);
        // assert(alignedSize == buildInfo.sizeInfo.buildScratchSize);  // Make sure it was already aligned
        maxScratch = std::max(maxScratch, alignedSize);
        totalScratch += alignedSize;
    }

    return { maxScratch, totalScratch };
}

// Find if the total scratch size is within the budget, otherwise return n-time the max scratch size that fits in the budget
VkDeviceSize BlasBuilder::getScratchSize(VkDeviceSize                                             hintMaxBudget,
    const std::vector<AccelerationStructureBuildData>& buildData,
    uint32_t minAlignment /*= 128*/) const
{
    ScratchSizeInfo sizeInfo = calculateScratchAlignedSizes(buildData, minAlignment);
    VkDeviceSize    maxScratch = sizeInfo.maxScratch;
    VkDeviceSize    totalScratch = sizeInfo.totalScratch;

    if (totalScratch < hintMaxBudget)
    {
        return totalScratch;
    }
    else
    {
        uint64_t numScratch = std::max(uint64_t(1), hintMaxBudget / maxScratch);
        numScratch = std::min(numScratch, buildData.size());
        return numScratch * maxScratch;
    }
}

// Return the scratch addresses fitting the scrath strategy (see above)
void BlasBuilder::getScratchAddresses(VkDeviceSize                                             hintMaxBudget,
    const std::vector<AccelerationStructureBuildData>& buildData,
    VkDeviceAddress               scratchBufferAddress,
    std::vector<VkDeviceAddress>& scratchAddresses,
    uint32_t                      minAlignment /*=128*/)
{
    ScratchSizeInfo sizeInfo = calculateScratchAlignedSizes(buildData, minAlignment);
    VkDeviceSize    maxScratch = sizeInfo.maxScratch;
    VkDeviceSize    totalScratch = sizeInfo.totalScratch;

    // Strategy 1: scratch was large enough for all BLAS, return the addresses in order
    if (totalScratch < hintMaxBudget)
    {
        VkDeviceAddress address = {};
        for (auto& buildInfo : buildData)
        {
            scratchAddresses.push_back(scratchBufferAddress + address);
            VkDeviceSize alignedSize = AlignedVkSize(buildInfo.sizeInfo.buildScratchSize, minAlignment);
            address += alignedSize;
        }
    }
    // Strategy 2: there are n-times the max scratch fitting in the budget
    else
    {
        // Make sure there is at least one scratch buffer, and not more than the number of BLAS
        uint64_t numScratch = std::max(uint64_t(1), hintMaxBudget / maxScratch);
        numScratch = std::min(numScratch, buildData.size());

        VkDeviceAddress address = {};
        for (int i = 0; i < numScratch; i++)
        {
            scratchAddresses.push_back(scratchBufferAddress + address);
            address += maxScratch;
        }
    }
}


// Generates a formatted string summarizing the statistics of BLAS compaction results.
// The output includes the original and compacted sizes in megabytes (MB), the amount of memory saved,
// and the percentage reduction in size. This method is intended to provide a quick, human-readable
// summary of the compaction efficiency.
//
// Returns:
//   A string containing the formatted summary of the BLAS compaction statistics.
std::string BlasBuilder::Stats::toString() const
{
    const VkDeviceSize savedSize = totalOriginalSize - totalCompactSize;
    const float fractionSmaller = (totalOriginalSize == 0) ? 0.0f : savedSize / static_cast<float>(totalOriginalSize);

    const std::string output = std::format("BLAS Compaction: {} bytes -> {} bytes ({} bytes saved, {:.2f}% smaller)",
        totalOriginalSize, totalCompactSize, savedSize, fractionSmaller * 100.0f);

    return output;
}

// Returns the maximum scratch buffer size needed for building all provided acceleration structures.
// This function iterates through a vector of AccelerationStructureBuildData, comparing the scratch
// size required for each structure and returns the largest value found.
//
// Returns:
//   The maximum scratch size needed as a VkDeviceSize.
VkDeviceSize getMaxScratchSize(const std::vector<AccelerationStructureBuildData>& asBuildData)
{
    VkDeviceSize maxScratchSize = 0;
    for (const auto& blas : asBuildData)
    {
        maxScratchSize = std::max(maxScratchSize, blas.sizeInfo.buildScratchSize);
    }
    return maxScratchSize;
}
