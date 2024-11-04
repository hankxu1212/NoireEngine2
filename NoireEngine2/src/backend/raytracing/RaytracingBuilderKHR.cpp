#include <numeric>

#include "BLASBuilder.h"
#include "RaytracingBuilderKHR.hpp"
#include "utils/Logger.hpp"
#include "backend/VulkanContext.hpp"

//--------------------------------------------------------------------------------------------------
// Destroying all allocations
//
void RaytracingBuilderKHR::Destroy()
{
    for(auto& b : m_blas)
    {
        RaytracingPipeline::DeleteAccelerationStructure(b);
    }
    RaytracingPipeline::DeleteAccelerationStructure(m_tlas);

    m_blas.clear();
}

//--------------------------------------------------------------------------------------------------
// Return the device address of a Blas previously created.
//
VkDeviceAddress RaytracingBuilderKHR::getBlasDeviceAddress(uint32_t blasId)
{
    assert(size_t(blasId) < m_blas.size());
    VkAccelerationStructureDeviceAddressInfoKHR addressInfo{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR};
    addressInfo.accelerationStructure = m_blas[blasId].handle;
    return RaytracingPipeline::vkGetAccelerationStructureDeviceAddressKHR(VulkanContext::GetDevice(), &addressInfo);
}

//--------------------------------------------------------------------------------------------------
// Create all the BLAS from the vector of BlasInput
// - There will be one BLAS per input-vector entry
// - There will be as many BLAS as input.size()
// - The resulting BLAS (along with the inputs used to build) are stored in m_blas,
//   and can be referenced by index.
// - if flag has the 'Compact' flag, the BLAS will be compacted
//
void RaytracingBuilderKHR::BuildBlas(const std::vector<BlasInput>& input, VkBuildAccelerationStructureFlagsKHR flags)
{
    auto         numBlas = static_cast<uint32_t>(input.size());
    VkDeviceSize maxScratchSize{0};  // Largest scratch size

    std::vector<AccelerationStructureBuildData> blasBuildData(numBlas);
    m_blas.resize(numBlas);  // Resize to hold all the BLAS
    for(uint32_t idx = 0; idx < numBlas; idx++)
    {
        blasBuildData[idx].asType           = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
        blasBuildData[idx].asGeometry       = input[idx].asGeometry;
        blasBuildData[idx].asBuildRangeInfo = input[idx].asBuildOffsetInfo;

        auto sizeInfo  = blasBuildData[idx].finalizeGeometry(VulkanContext::GetDevice(), input[idx].flags | flags);
        maxScratchSize = std::max(maxScratchSize, sizeInfo.buildScratchSize);
    }

    VkDeviceSize hintMaxBudget{256'000'000};  // 256 MB

    // Allocate the scratch buffers holding the temporary data of the acceleration structure builder
    RaytracingPipeline::ScratchBuffer blasScratchBuffer;

    bool hasCompaction = hasFlag(flags, VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_COMPACTION_BIT_KHR);

    BlasBuilder blasBuilder;

    uint32_t minAlignment = 128; /*m_rtASProperties.minAccelerationStructureScratchOffsetAlignment*/
    
    // 1) finding the largest scratch size
    VkDeviceSize scratchSize = blasBuilder.getScratchSize(hintMaxBudget, blasBuildData, minAlignment);

    // 2) allocating the scratch buffer
    blasScratchBuffer = RaytracingPipeline::CreateScratchBuffer(scratchSize);

    // 3) getting the device address for the scratch buffer
    std::vector<VkDeviceAddress> scratchAddresses;
    blasBuilder.getScratchAddresses(hintMaxBudget, blasBuildData, blasScratchBuffer.deviceAddress, scratchAddresses, minAlignment);

    bool finished = false;
    do
    {
        NE_INFO(blasBuilder.getStatistics().toString());
        {
            CommandBuffer cmd;
            finished = blasBuilder.CmdCreateParallelBlas(cmd.getCommandBuffer(), blasBuildData, m_blas, scratchAddresses, hintMaxBudget);
            cmd.SubmitWait();
        }
        if(hasCompaction)
        {
            CommandBuffer cmd;
            blasBuilder.CmdCompactBlas(cmd, blasBuildData, m_blas);
            cmd.SubmitWait();
            blasBuilder.destroyNonCompactedBlas();
        }
    } while(!finished);

    if(hasCompaction)
    {
        NE_INFO(blasBuilder.getStatistics().toString());
    }

    blasScratchBuffer.Destroy();
}

//--------------------------------------------------------------------------------------------------
// Refit BLAS number blasIdx from updated buffer contents.
//
void RaytracingBuilderKHR::UpdateBlas(uint32_t blasIdx, BlasInput& blas, VkBuildAccelerationStructureFlagsKHR flags)
{
    assert(size_t(blasIdx) < m_blas.size());

    AccelerationStructureBuildData buildData{VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR};
    buildData.asGeometry       = blas.asGeometry;
    buildData.asBuildRangeInfo = blas.asBuildOffsetInfo;
    auto sizeInfo              = buildData.finalizeGeometry(VulkanContext::GetDevice(), flags);

    RaytracingPipeline::ScratchBuffer scratchBuffer = RaytracingPipeline::CreateScratchBuffer(sizeInfo.updateScratchSize);

    CommandBuffer cmdBuf;

    buildData.cmdUpdateAccelerationStructure(cmdBuf, m_blas[blasIdx].handle, scratchBuffer.deviceAddress);
  
    cmdBuf.SubmitIdle();
    scratchBuffer.Destroy();
}
