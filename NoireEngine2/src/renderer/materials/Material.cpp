#include "Material.hpp"
#include "backend/pipeline/ObjectPipeline.hpp"

#include "glm/gtx/string_cast.hpp"

#include "utils/Logger.hpp"

Material::Material(const CreateInfo& createInfo) :
	m_CreateInfo(createInfo), m_Albedo(createInfo.albedo) {
	Logger::INFO("Created material with albedo {} ", glm::to_string(m_Albedo));
}

void Material::Push(const CommandBuffer& commandBuffer, VkPipelineLayout pipelineLayout)
{
	ObjectPipeline::MaterialPush push{
		.albedo = { m_Albedo.x, m_Albedo.y, m_Albedo.z, 0 }
	};

	vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0,
		sizeof(ObjectPipeline::MaterialPush), &push);
}

Material* Material::Deserialize(const Scene::TValueMap& obj)
{
	try {
		const auto& attributesMap = obj.at("lambertian").as_object().value();
		if (attributesMap.find("albedo") == attributesMap.end())
			throw std::runtime_error("Did not find albedo field in this material, aborting!");

		const auto& albedoArrOpt = attributesMap.at("albedo").as_array();
		assert(albedoArrOpt && "Material's albedo field cannot be read as array... aborting.");
		assert(albedoArrOpt.value().size() == 3 && "Material's albedo field must have size 3");
		const auto& albedo = albedoArrOpt.value();

		CreateInfo createInfo;
		createInfo.name = obj.at("name").as_string().value();
		createInfo.albedo.x = albedo[0].as_float();
		createInfo.albedo.y = albedo[1].as_float();
		createInfo.albedo.z = albedo[2].as_float();

		return Create(createInfo).get();
	}
	catch (std::exception& e) {
		std::cout << "Failed to deserialize value as Material: " << e.what() << std::endl;
		return nullptr;
	}
}

std::shared_ptr<Material> Material::CreateDefault()
{
	CreateInfo info;
	info.name = "Default Lambertian";
	info.albedo = glm::vec3(1);

	return Create(info);
}

std::shared_ptr<Material> Material::Create(const CreateInfo& createInfo)
{
	Material temp(createInfo);
	Node node;
	node << temp;
	return Create(node);
}

std::shared_ptr<Material> Material::Create(const Node& node)
{
	if (auto resource = Resources::Get()->Find<Material>(node)) {
		std::cout << "Reusing old material: " << resource->m_CreateInfo.name << " " << std::endl;
		return resource;
	}

	auto result = std::make_shared<Material>();
	Resources::Get()->Add(node, std::dynamic_pointer_cast<Resource>(result));
	node >> *result;
	return result;
}

const Node& operator>>(const Node& node, Material& material)
{
	node["createInfo"].Get(material.m_CreateInfo);
	node["albedo"].Get(material.m_Albedo);
	return node;
}

Node& operator<<(Node& node, const Material& material)
{
	node["createInfo"].Set(material.m_CreateInfo);
	node["albedo"].Set(material.m_Albedo);
	return node;
}

const Node& operator>>(const Node& node, Material::CreateInfo & info)
{
	node["albedo"].Get(info.albedo);
	node["name"].Get(info.name);
	return node;
}

Node& operator<<(Node& node, const Material::CreateInfo& info)
{
	node["albedo"].Set(info.albedo);
	node["name"].Set(info.name);
	return node;
}






const Node& operator>>(const Node& node, glm::vec3& v) {
	node["x"].Get(v.x);
	node["y"].Get(v.y);
	node["z"].Get(v.z);
	return node;
}

Node& operator<<(Node& node, const glm::vec3& v) {
	node["x"].Set(v.x);
	node["y"].Set(v.y);
	node["z"].Set(v.z);
	return node;
}