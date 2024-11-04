#include <numeric>

#include "BLASBuilder.h"
#include "RaytracingBuilderKHR.hpp"
#include "utils/Logger.hpp"
#include "backend/VulkanContext.hpp"

// Destroying all allocations
void RaytracingBuilderKHR::Destroy()
{
    for(auto& b : m_blas)
    {
        RaytracingPipeline::DeleteAccelerationStructure(b);
    }
    RaytracingPipeline::DeleteAccelerationStructure(m_tlas);
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

        auto sizeInfo  = blasBuildData[idx].FinalizeGeometry(VulkanContext::GetDevice(), input[idx].flags | flags);
        maxScratchSize = std::max(maxScratchSize, sizeInfo.buildScratchSize);
    }

    VkDeviceSize hintMaxBudget{256'000'000};  // 256 MB

    // Allocate the scratch buffers holding the temporary data of the acceleration structure builder
    ScratchBuffer blasScratchBuffer;

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
    auto sizeInfo              = buildData.FinalizeGeometry(VulkanContext::GetDevice(), flags);

    ScratchBuffer scratchBuffer = RaytracingPipeline::CreateScratchBuffer(sizeInfo.updateScratchSize);

    CommandBuffer cmdBuf;

    buildData.CmdUpdateAccelerationStructure(cmdBuf, m_blas[blasIdx].handle, scratchBuffer.deviceAddress);
  
    cmdBuf.SubmitWait();
    scratchBuffer.Destroy();
}

void RaytracingBuilderKHR::BuildTlas(const std::vector<VkAccelerationStructureInstanceKHR>& instances, VkBuildAccelerationStructureFlagsKHR flags, bool update)
{
    // Cannot call buildTlas twice except to update.
    assert(m_tlas.handle == VK_NULL_HANDLE || update);
    uint32_t countInstance = static_cast<uint32_t>(instances.size());
    size_t sizeInBytes = countInstance * sizeof(VkAccelerationStructureInstanceKHR);

    // Command buffer to create the TLAS
    CommandBuffer cmd;

    VkMemoryAllocateFlagsInfoKHR flags_info{ VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO_KHR };
    flags_info.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT_KHR;

    // Create a buffer holding the actual instance data (matrices++) for use by the AS builder
    Buffer instancesBuffer = Buffer(
        sizeInBytes,
        VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        Buffer::Unmapped,
        &flags_info
    );

    // source staging buffer
    Buffer transferSource = Buffer(
        sizeInBytes,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        Buffer::Mapped
    );

    std::memcpy(transferSource.data(), (void*)instances.data(), sizeInBytes);

    Buffer::CopyBuffer(cmd, transferSource.getBuffer(), instancesBuffer.getBuffer(), sizeInBytes);

    VkBufferDeviceAddressInfo bufferInfo{ VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO, nullptr, instancesBuffer.getBuffer()};
    VkDeviceAddress           instBufferAddr = vkGetBufferDeviceAddress(VulkanContext::GetDevice(), &bufferInfo);

    // Make sure the copy of the instance buffer are copied before triggering the acceleration structure build
    VkBufferMemoryBarrier barrier = transferSource.CreateBufferMemoryBarrier(VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR);
    vkCmdPipelineBarrier(
        cmd, 
        VK_PIPELINE_STAGE_TRANSFER_BIT, 
        VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
        0, 
        0, nullptr, 
        1, &barrier,
        0, nullptr
    );

    // Creating the TLAS
    ScratchBuffer scratchBuffer;
    CmdCreateTlas(cmd, countInstance, instBufferAddr, scratchBuffer, flags, update);

    // Finalizing and destroying temporary data
    cmd.SubmitWait();
    instancesBuffer.Destroy();
    transferSource.Destroy();
    scratchBuffer.Destroy();
}

void RaytracingBuilderKHR::CmdCreateTlas(VkCommandBuffer cmdBuf, uint32_t countInstance, VkDeviceAddress instBufferAddr, ScratchBuffer& scratchBuffer, VkBuildAccelerationStructureFlagsKHR flags, bool update)
{
    AccelerationStructureBuildData tlasBuildData;
    tlasBuildData.asType = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;

    AccelerationStructureGeometryInfo geo = tlasBuildData.MakeInstanceGeometry(countInstance, instBufferAddr);
    tlasBuildData.AddGeometry(geo);

    auto sizeInfo = tlasBuildData.FinalizeGeometry(VulkanContext::GetDevice(), flags);

    // Allocate the scratch memory
    VkDeviceSize scratchSize = update ? sizeInfo.updateScratchSize : sizeInfo.buildScratchSize;
    scratchBuffer = RaytracingPipeline::CreateScratchBuffer(scratchSize);

    VkDeviceAddress scratchAddress = RaytracingPipeline::GetBufferDeviceAddress(scratchBuffer.buffer.getBuffer());

    if (update)
    {  // Update the acceleration structure
        tlasBuildData.asGeometry[0].geometry.instances.data.deviceAddress = instBufferAddr;
        tlasBuildData.CmdUpdateAccelerationStructure(cmdBuf, m_tlas.handle, scratchAddress);
    }
    else
    {  // Create and build the acceleration structure
        VkAccelerationStructureCreateInfoKHR createInfo = tlasBuildData.MakeCreateInfo();

        m_tlas = RaytracingPipeline::CreateAccelerationStructure(createInfo);
        tlasBuildData.CmdBuildAccelerationStructure(cmdBuf, m_tlas.handle, scratchAddress);
    }
}
