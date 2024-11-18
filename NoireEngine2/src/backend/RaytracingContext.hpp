#pragma once

#include "backend/renderpass/Renderpass.hpp"
#include "backend/buffers/Buffer.hpp"
#include "backend/raytracing/RaytracingBuilderKHR.hpp"
#include "backend/images/Image2D.hpp"
#include "core/Core.hpp"
#include "core/resources/Module.hpp"

class Renderer;

class RaytracingContext
{
	inline static RaytracingContext* g_Instance;

public:
	static RaytracingContext* Get() { return g_Instance; }

	RaytracingContext();

	~RaytracingContext();

	void CreateAccelerationStructures();

	inline VkAccelerationStructureKHR GetTLAS() const { return m_RTBuilder.getAccelerationStructure(); }

	void CreateBottomLevelAccelerationStructure();

	void CreateTopLevelAccelerationStructure(bool update);

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
	RaytracingBuilderKHR m_RTBuilder;
	
	std::vector<VkAccelerationStructureInstanceKHR> m_TlasBuildStructs;
};

