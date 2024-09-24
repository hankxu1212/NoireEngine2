#include "SceneHierarchyPanel.h"

#include "renderer/scene/Entity.hpp"
#include "renderer/scene/SceneManager.hpp"
#include "core/Core.hpp"

SceneHierarchyPanel::SceneHierarchyPanel()
{
	m_SelectedEntity = nullptr;
}

void SceneHierarchyPanel::DrawEntityNode(Entity* entity)
{
	//auto m_ActiveScene = SceneManager::Get()->getScene();

	auto& tag = entity->name();
	bool isLeaf = entity->children().empty();

	// set base flags
	ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_SpanAvailWidth;
			
	if (isLeaf)
		flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
	else
		flags |= ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick;
			
	if (m_SelectedEntity == entity)
		flags |= ImGuiTreeNodeFlags_Selected;

	bool opened = ImGui::TreeNodeEx((void*)entity, flags, tag.c_str());
	if (ImGui::IsItemClicked())
		m_SelectedEntity = entity;

	///////////////////////////////////////////////////////////////// Popup
	bool entityDeleted = false;
	if (ImGui::BeginPopupContextItem())
	{
		if (ImGui::MenuItem("Delete Entity")) {
			entityDeleted = true;
		}

		//if (ImGui::MenuItem("Clone")) {
		//	m_SelectedEntity = m_ActiveScene->CloneEntity(entity);
		//}

		ImGui::EndPopup();
	}

	///////////////////////////////////////////////////////////////// Dragging
	/*
	{
		// set drag source
		bool dragging = false;
		if (ImGui::BeginDragDropSource())
		{
			dragging = true;

			ImGui::SetDragDropPayload(NE_ENTITY_TYPE, (void*)&entity, sizeof(Entity));
			ImGui::Text(tag.c_str());
			ImGui::EndDragDropSource();
		}

		// set drag target
		if (!dragging && ImGui::BeginDragDropTarget())
		{
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(NE_ENTITY_TYPE)) {
				Entity* sourceEntity = (Entity*)payload->Data;
				if (*sourceEntity != entity)
				{
					sourceEntity->Move(entity);
				}
			}

			ImGui::EndDragDropTarget();
		}
	}
	*/

	// TODO: delete entity
	if (entityDeleted) {
		//m_ActiveScene->DestroyEntity(entity);
		if (m_SelectedEntity == entity)
			m_SelectedEntity = {};
	}

	// recursivly render children
	if (!isLeaf && opened)
	{
		for (const auto& childrenEntity : entity->children())
		{
			DrawEntityNode(childrenEntity.get());
		}
		ImGui::TreePop();
	}
}

void SceneHierarchyPanel::Render(ImGuiWindowFlags editor_window_flags)
{
	auto m_ActiveScene = SceneManager::Get()->getScene();
	if (!m_ActiveScene)
		return;

	auto region = ImGui::GetContentRegionAvail();

	ImGui::BeginChild("#Entities", ImVec2(region.x, region.y * 0.7f), ImGuiChildFlags_None, editor_window_flags);
	for (const auto& entity : Entity::root().children())
	{
		DrawEntityNode(entity.get());		
	}
	ImGui::EndChild();

	entityEditor.RenderEditor(m_SelectedEntity, editor_window_flags);
}

void SceneHierarchyPanel::SetSelectedEntity(Entity* entity)
{
	m_SelectedEntity = entity;
}

template<typename T>
inline void SceneHierarchyPanel::DisplayAddComponentEntry(const std::string& entryName)
{
}
