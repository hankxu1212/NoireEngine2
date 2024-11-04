#pragma once

#include "VulkanPipeline.hpp"
#include "backend/renderpass/Renderpass.hpp"
#include "backend/buffers/Buffer.hpp"
#include "backend/descriptor/DescriptorBuilder.hpp"
#include "backend/raytracing/RaytracingBuilderKHR.hpp"
#include "backend/images/Image2D.hpp"
#include "core/Core.hpp"

class ObjectPipeline;

class RaytracingPipeline : public VulkanPipeline
{
public:
	RaytracingPipeline(ObjectPipeline*);

	~RaytracingPipeline();

	void CreatePipeline() override;

	void Render(const Scene* scene, const CommandBuffer& commandBuffer) override;

	void Prepare(const Scene* scene, const CommandBuffer& commandBuffer);

public:
	// Function pointers for ray tracing related stuff
	inline static PFN_vkGetBufferDeviceAddressKHR vkGetBufferDeviceAddressKHR;
	inline static PFN_vkCreateAccelerationStructureKHR vkCreateAccelerationStructureKHR;
	inline static PFN_vkDestroyAccelerationStructureKHR vkDestroyAccelerationStructureKHR;
	inline static PFN_vkGetAccelerationStructureBuildSizesKHR vkGetAccelerationStructureBuildSizesKHR;
	inline static PFN_vkGetAccelerationStructureDeviceAddressKHR vkGetAccelerationStructureDeviceAddressKHR;
	inline static PFN_vkBuildAccelerationStructuresKHR vkBuildAccelerationStructuresKHR;
	inline static PFN_vkCmdBuildAccelerationStructuresKHR vkCmdBuildAccelerationStructuresKHR;
	inline static PFN_vkCmdTraceRaysKHR vkCmdTraceRaysKHR;
	inline static PFN_vkGetRayTracingShaderGroupHandlesKHR vkGetRayTracingShaderGroupHandlesKHR;
	inline static PFN_vkCreateRayTracingPipelinesKHR vkCreateRayTracingPipelinesKHR;
	inline static PFN_vkCmdWriteAccelerationStructuresPropertiesKHR vkCmdWriteAccelerationStructuresPropertiesKHR;
	inline static PFN_vkCmdCopyAccelerationStructureKHR vkCmdCopyAccelerationStructureKHR;

	// Available features and properties
	VkPhysicalDeviceRayTracingPipelinePropertiesKHR  rayTracingPipelineProperties{};
	VkPhysicalDeviceAccelerationStructureFeaturesKHR accelerationStructureFeatures{};

	// Extends the buffer class and holds information for a shader binding table
	class ShaderBindingTable : public Buffer {
	public:
		VkStridedDeviceAddressRegionKHR stridedDeviceAddressRegion{};
	};

	static ScratchBuffer CreateScratchBuffer(VkDeviceSize size);

	void CreateAccelerationStructure(AccelerationStructure& accelerationStructure, VkAccelerationStructureTypeKHR type, VkAccelerationStructureBuildSizesInfoKHR buildSizeInfo);
	
	static AccelerationStructure CreateAccelerationStructure(const VkAccelerationStructureCreateInfoKHR& createInfo);

	static void DeleteAccelerationStructure(AccelerationStructure& accelerationStructure);

	static uint64_t GetBufferDeviceAddress(VkBuffer buffer);

	VkStridedDeviceAddressRegionKHR GetSbtEntryStridedDeviceAddressRegion(VkBuffer buffer, uint32_t handleCount);

	void CreateShaderBindingTable(ShaderBindingTable& shaderBindingTable, uint32_t handleCount);

public:
	START_BINDING(RTXBindings)
		TLAS = 0,  // Top-level acceleration structure
		OutImage = 1   // Ray tracer output image
	END_BINDING();

	std::vector<VkRayTracingShaderGroupCreateInfoKHR> shaderGroups{};
	struct ShaderBindingTables {
		ShaderBindingTable raygen;
		ShaderBindingTable miss;
		ShaderBindingTable hit;
	} shaderBindingTables;

	VkPipeline			m_Pipeline = VK_NULL_HANDLE;
	VkPipelineLayout	m_PipelineLayout = VK_NULL_HANDLE;
	VkDescriptorSet set0;
	VkDescriptorSetLayout set0_layout;

	DescriptorAllocator						m_DescriptorAllocator;
	RaytracingBuilderKHR					m_RTBuilder;

	std::unique_ptr<Image2D>				m_RtxImage;

private:
	//void MeshTo
	void CreateBottomLevelAccelerationStructure();
	void CreateTopLevelAccelerationStructure();
	void CreateStorageImage();
	void CreateDescriptorSets();
	void CreateUniformBuffer();
	void CreateRayTracingPipeline();
	void CreateShaderBindingTables();

private:
	ObjectPipeline* p_ObjectPipeline;
};

