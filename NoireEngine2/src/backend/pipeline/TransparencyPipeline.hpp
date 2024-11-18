#pragma once

#include "VulkanPipeline.hpp"
#include "backend/renderpass/Renderpass.hpp"
#include "backend/buffers/Buffer.hpp"
#include "backend/descriptor/DescriptorBuilder.hpp"
#include "backend/images/Image2D.hpp"
#include "core/Core.hpp"

class Renderer;

class TransparencyPipeline : public VulkanPipeline
{
public:
	~TransparencyPipeline();

	void CreatePipeline() override;

	void Render(const Scene* scene, const CommandBuffer& commandBuffer) override;

	void Prepare(const Scene* scene, const CommandBuffer& commandBuffer);

	void OnUIRender();

private:
	VkPipeline m_Pipeline = VK_NULL_HANDLE;
	VkPipelineLayout m_PipelineLayout = VK_NULL_HANDLE;

	std::vector<VkRayTracingShaderGroupCreateInfoKHR> m_RTShaderGroups;

	struct PushConstantRay
	{
		int rayDepth = 3;
		int frame = -1;
	}m_TransparencyPush;

	bool m_TransparencyIsDirty = false;

	// SBT
	Buffer m_rtSBTBuffer;
	VkStridedDeviceAddressRegionKHR m_rgenRegion{};
	VkStridedDeviceAddressRegionKHR m_missRegion{};
	VkStridedDeviceAddressRegionKHR m_hitRegion{};
	VkStridedDeviceAddressRegionKHR m_callRegion{};

private:
	void CreateRayTracingPipeline();
	void CreateShaderBindingTables();

	std::vector<VkAccelerationStructureInstanceKHR> m_TlasBuildStructs;
};