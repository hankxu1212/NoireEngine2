#include "GlassMaterial.hpp"
#include "utils/Logger.hpp"

GlassMaterial::GlassMaterial(const CreateInfo& createInfo) :
    m_CreateInfo(createInfo) {
}

void GlassMaterial::Load()
{
}

Material* GlassMaterial::Deserialize(const Scene::TValueMap& obj)
{
	try {
		const auto& attributesMap = obj.at("glass").as_object().value();
		CreateInfo createInfo;
		createInfo.name = obj.at("name").as_string().value();
		createInfo.IOR = attributesMap.at("IOR").as_float();

		return Create<GlassMaterial>(createInfo).get();
	}
	catch (std::exception& e) {
		NE_WARN("Failed to deserialize value as Material: {}", e.what());
		return nullptr;
	}
}

void GlassMaterial::Inspect()
{
}

const Node& operator>>(const Node& node, GlassMaterial& material)
{
	node["createInfo"].Get(material.m_CreateInfo);
	node["workflow"].Get(material.getWorkflow());
	return node;
}

Node& operator<<(Node& node, const GlassMaterial& material)
{
	node["createInfo"].Set(material.m_CreateInfo);
	node["workflow"].Set(material.getWorkflow());
	return node;
}

const Node& operator>>(const Node& node, GlassMaterial::CreateInfo& info)
{
	node["name"].Get(info.name);
	node["ior"].Get(info.IOR);
	return node;
}

Node& operator<<(Node& node, const GlassMaterial::CreateInfo& info)
{
	node["name"].Set(info.name);
	node["ior"].Set(info.IOR);
	return node;
}