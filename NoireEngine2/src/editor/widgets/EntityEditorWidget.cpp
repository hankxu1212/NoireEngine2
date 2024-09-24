#pragma once

#include "EntityEditorWidget.h"
#include "utils/Logger.hpp"

static bool g_Debug = false;

// renders a widget by calling Inspect() on the component
static void ComponentWidget(Entity* entity, Component* component)
{
	ImGui::PushID(component->getName());
	//if (ImGui::Button("-")) {
	//	entity.RemoveComponent(component); // let compiler deduce type
	//	ImGui::PopID();
	//	return;
	//}
	//else {
	//	ImGui::SameLine();
	//}

	if (ImGui::CollapsingHeader(component->getName(), ImGuiTreeNodeFlags_DefaultOpen)) {
		ImGui::Indent(30.f);
		ImGui::PushID("##ComponentWidget");
		{
			// custom widget for each component
			if (g_Debug)
				component->Debug();
			else
				component->Inspect();
		}
		ImGui::PopID();
		ImGui::Unindent(30.f);
	}
	ImGui::PopID();
}

void EntityEditorWidget::RenderEditor(Entity* e, ImGuiWindowFlags editor_window_flags)
{
	if (!ImGui::Begin("Entity Inspector", &show, editor_window_flags) || !e)
	{
		ImGui::End();
		return;
	}

	if (!e) {
		ImGui::Text("Invalid Entity");
		ImGui::End();
		return;
	}

	if (e != activeEntity) {
		auto& name = e->name();
		assert(name.length() < maxNameLength);
		strcpy_s(nameBuffer, name.c_str());
	}

	ImGui::Columns(2);
	ImGui::Text("%s", "Debug");
	ImGui::NextColumn();
	if (ImGui::Checkbox("##Debug", &debugMode))
	{
		g_Debug = debugMode;
	}
	ImGui::Columns(1);

	ImGuiInputTextFlags textFlags = ImGuiInputTextFlags_EnterReturnsTrue;
	if(ImGui::InputText("Entity Name", nameBuffer, maxNameLength, textFlags)) {
		e->name().assign(nameBuffer);
	}

	if (e) {
		ImGui::Separator(); // --------------------------------------------------

		// render components attached with this entity
		ImGui::PushID(static_cast<int>(e->id()));
		{
			if (debugMode)
			{
				ImGui::Text("Entity Handle: %d", (uint32_t)e->id());
				ImGui::Separator(); // --------------------------------------------------
			}

			for (const auto& component : e->components())
				ComponentWidget(e, component.get()); // <====================== render each component here

			ImGui::Separator(); // --------------------------------------------------

			if (ImGui::Button("+ Add Component")) {
				ImGui::OpenPopup("Add Component");
			}

			if (ImGui::BeginPopup("Add Component")) {
				ImGui::TextUnformatted("Available:");

				ImGui::Separator(); // --------------------------------------------------

				Logger::WARN("Adding components to an entity have not been implemented");

				ImGui::EndPopup();
			}
		}
		ImGui::PopID();
	}

	ImGui::End();
}
