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

	static uint64_t GetBufferDeviceAddress(VkBuffer buffer);

	VkStridedDeviceAddressRegionKHR GetSbtEntryStridedDeviceAddressRegion(VkBuffer buffer, uint32_t handleCount);

private:
	friend class ImGuiPipeline;

	ObjectPipeline* p_ObjectPipeline;

	START_BINDING(RTXBindings)
		TLAS = 0,  // Top-level acceleration structure
		OutImage = 1,   // Ray tracer output image
		ObjDesc = 2 // stores obj description and buffer addresses
	END_BINDING();

	VkPipeline			m_Pipeline = VK_NULL_HANDLE;
	VkPipelineLayout	m_PipelineLayout = VK_NULL_HANDLE;
	VkDescriptorSet set0;
	VkDescriptorSetLayout set0_layout;
	std::vector<VkRayTracingShaderGroupCreateInfoKHR> m_RTShaderGroups;

	struct PushConstantRay
	{
		glm::vec4  clearColor;
		glm::vec3  lightPosition;
		float lightIntensity;
		int   lightType;
	};

	PushConstantRay m_pcRay{};

	// Information of a obj model when referenced in a shader
	struct ObjectDescription
	{
		int      txtOffset;             // Texture index offset in the array of textures
		uint64_t vertexAddress;         // Address of the Vertex buffer
		uint64_t indexAddress;          // Address of the index buffer
		//uint64_t materialAddress;       // Address of the material buffer
		//uint64_t materialIndexAddress;  // Address of the triangle material index buffer
	};
	Buffer m_ObjDescBuffer;

	DescriptorAllocator						m_DescriptorAllocator;
	RaytracingBuilderKHR					m_RTBuilder;

	std::unique_ptr<Image2D>				m_RtxImage;

	// SBT
	Buffer                    m_rtSBTBuffer;
	VkStridedDeviceAddressRegionKHR m_rgenRegion{};
	VkStridedDeviceAddressRegionKHR m_missRegion{};
	VkStridedDeviceAddressRegionKHR m_hitRegion{};
	VkStridedDeviceAddressRegionKHR m_callRegion{};

private:
	//void MeshTo
	void CreateBottomLevelAccelerationStructure();
	void CreateTopLevelAccelerationStructure();
	void CreateStorageImage();
	void CreateDescriptorSets();
	void CreateObjectDescriptionBuffer();
	void CreateRayTracingPipeline();
	void CreateShaderBindingTables();
};

