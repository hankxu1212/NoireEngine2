#pragma once

#include "VulkanPipeline.hpp"
#include "backend/renderpass/Renderpass.hpp"
#include "backend/buffers/Buffer.hpp"
#include "backend/descriptor/DescriptorBuilder.hpp"
#include "backend/raytracing/RaytracingBuilderKHR.hpp"
#include "backend/images/Image2D.hpp"
#include "core/Core.hpp"

class Renderer;

class RaytracingPipeline : public VulkanPipeline
{
public:
	RaytracingPipeline();

	~RaytracingPipeline();

	void CreatePipeline() override;

	void CreateAccelerationStructures();

	void Render(const Scene* scene, const CommandBuffer& commandBuffer) override;

	void Prepare(const Scene* scene, const CommandBuffer& commandBuffer);

	void OnUIRender();

	inline VkAccelerationStructureKHR GetTLAS() const { return m_RTBuilder.getAccelerationStructure(); }

public: // ray tracing helpers
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

	static ScratchBuffer CreateScratchBuffer(VkDeviceSize size);
	
	// fills acceleration buffer and calls vkCreateAccelerationStructureKHR from build sizes
	static void CreateAccelerationStructure(AccelerationStructure& accelerationStructure, VkAccelerationStructureTypeKHR type, VkAccelerationStructureBuildSizesInfoKHR buildSizeInfo);
	
	// creates an acceleration structure from create info. Calls vkCreateAccelerationStructureKHR
	static AccelerationStructure CreateAccelerationStructure(const VkAccelerationStructureCreateInfoKHR& createInfo);

	static void DeleteAccelerationStructure(AccelerationStructure& accelerationStructure);

private:
	VkPipeline m_Pipeline = VK_NULL_HANDLE;
	VkPipelineLayout m_PipelineLayout = VK_NULL_HANDLE;

	std::vector<VkRayTracingShaderGroupCreateInfoKHR> m_RTShaderGroups;

	struct PushConstantRay
	{
		int rayDepth = 5;
	}m_ReflectionPush;

	RaytracingBuilderKHR m_RTBuilder;

	// SBT
	Buffer m_rtSBTBuffer;
	VkStridedDeviceAddressRegionKHR m_rgenRegion{};
	VkStridedDeviceAddressRegionKHR m_missRegion{};
	VkStridedDeviceAddressRegionKHR m_hitRegion{};
	VkStridedDeviceAddressRegionKHR m_callRegion{};

private:
	void CreateBottomLevelAccelerationStructure();
	void CreateTopLevelAccelerationStructure(bool update);
	void CreateRayTracingPipeline();
	void CreateShaderBindingTables();

	std::vector<VkAccelerationStructureInstanceKHR> m_TlasBuildStructs;
};

