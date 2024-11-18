#include <numeric>

#include "BLASBuilder.h"
#include "RaytracingBuilderKHR.hpp"
#include "backend/VulkanContext.hpp"

void RaytracingBuilderKHR::Destroy()
{
    for(auto& b : m_blas)
    {
        RaytracingContext::DeleteAccelerationStructure(b);
    }
    RaytracingContext::DeleteAccelerationStructure(m_tlas);

    m_InstanceBuffer.Destroy();
    m_InstanceStagingBuffer.Destroy();
    m_TlasScratchBuffer.Destroy();
}

// Return the device address of a Blas previously created.
VkDeviceAddress RaytracingBuilderKHR::getBlasDeviceAddress(uint32_t blasId)
{
    assert(size_t(blasId) < m_blas.size());
    VkAccelerationStructureDeviceAddressInfoKHR addressInfo{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR};
    addressInfo.accelerationStructure = m_blas[blasId].handle;
    return RaytracingContext::vkGetAccelerationStructureDeviceAddressKHR(VulkanContext::GetDevice(), &addressInfo);
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

    bool hasCompaction = hasFlag(flags, VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_COMPACTION_BIT_KHR);

    BlasBuilder blasBuilder;

    uint32_t minAlignment = 128; /*m_rtASProperties.minAccelerationStructureScratchOffsetAlignment*/
    
    // 1) finding the largest scratch size
    VkDeviceSize scratchSize = blasBuilder.getScratchSize(hintMaxBudget, blasBuildData, minAlignment);

    // Allocate the scratch buffers holding the temporary data of the acceleration structure builder
    ScratchBuffer blasScratchBuffer = RaytracingContext::CreateScratchBuffer(scratchSize);

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

// Refit BLAS number blasIdx from updated buffer contents.
void RaytracingBuilderKHR::UpdateBlas(uint32_t blasIdx, BlasInput& blas, VkBuildAccelerationStructureFlagsKHR flags)
{
    assert(size_t(blasIdx) < m_blas.size());

    AccelerationStructureBuildData buildData{VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR};
    buildData.asGeometry       = blas.asGeometry;
    buildData.asBuildRangeInfo = blas.asBuildOffsetInfo;
    auto sizeInfo              = buildData.FinalizeGeometry(VulkanContext::GetDevice(), flags);

    ScratchBuffer scratchBuffer = RaytracingContext::CreateScratchBuffer(sizeInfo.updateScratchSize);

    CommandBuffer cmdBuf;

    buildData.CmdUpdateAccelerationStructure(cmdBuf, m_blas[blasIdx].handle, scratchBuffer.deviceAddress);
  
    cmdBuf.SubmitWait();
    scratchBuffer.Destroy();
}

void RaytracingBuilderKHR::BuildTlas(const std::vector<VkAccelerationStructureInstanceKHR>& instances, VkBuildAccelerationStructureFlagsKHR flags, bool update)
{
    assert(m_tlas.handle == VK_NULL_HANDLE || update);

    uint32_t countInstance = static_cast<uint32_t>(instances.size());
    size_t sizeInBytes = countInstance * sizeof(VkAccelerationStructureInstanceKHR);

    // if have not allocated before or too small, resize. We reuse the same instance buffer so we dont reallocate often
    if (!m_InstanceBuffer.buffer.getBuffer() || sizeInBytes > m_InstanceBuffer.buffer.getSize())
    {
        m_InstanceBuffer.buffer.Destroy();
        m_InstanceBuffer.buffer = Buffer(
            sizeInBytes,
            VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            Buffer::Unmapped
        );

        m_InstanceStagingBuffer.Destroy();
        m_InstanceStagingBuffer = Buffer(
            sizeInBytes,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            Buffer::Mapped
        );

        m_InstanceBuffer.deviceAddress = m_InstanceBuffer.buffer.GetBufferDeviceAddress();
    }

    CommandBuffer cmd;

    Buffer::CopyFromHost(cmd, m_InstanceStagingBuffer, m_InstanceBuffer.buffer, sizeInBytes, (void*)instances.data());

    // Make sure the copy of the instance buffer are copied before triggering the acceleration structure build
    m_InstanceStagingBuffer.InsertBufferMemoryBarrier(cmd,
        VK_ACCESS_TRANSFER_WRITE_BIT,
        VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR
    );

    // Creating the TLAS
    CmdCreateTlas(cmd, countInstance, m_InstanceBuffer.deviceAddress, m_TlasScratchBuffer, flags, update);

    // Finalizing and destroying temporary data
    cmd.SubmitIdle();
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

    if (!scratchBuffer.buffer.getBuffer() || scratchBuffer.buffer.getSize() < scratchSize)
        scratchBuffer = RaytracingContext::CreateScratchBuffer(scratchSize);

    if (update)
    {  // Update the acceleration structure
        tlasBuildData.asGeometry[0].geometry.instances.data.deviceAddress = instBufferAddr;
        tlasBuildData.CmdUpdateAccelerationStructure(cmdBuf, m_tlas.handle, scratchBuffer.deviceAddress);
    }
    else
    {  // Create and build the acceleration structure
        VkAccelerationStructureCreateInfoKHR createInfo = tlasBuildData.MakeCreateInfo();

        m_tlas = RaytracingContext::CreateAccelerationStructure(createInfo);
        tlasBuildData.CmdBuildAccelerationStructure(cmdBuf, m_tlas.handle, scratchBuffer.deviceAddress);
    }
}
