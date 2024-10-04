#include "LambertianMaterial.hpp"

void LambertianMaterial::Push(const CommandBuffer& commandBuffer, VkPipelineLayout pipelineLayout)
{
	LambertianMaterial::MaterialPush push{
		.albedo = { m_Albedo.x, m_Albedo.y, m_Albedo.z, 0 },
		.materialIndex = m_AlbedoMapIndex
	};

	vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0,
		sizeof(LambertianMaterial::MaterialPush), &push);
}
