#include "ImGuiExtension.hpp"

bool ImGuiExt::DrawVec3(float& x, float& y, float& z, const char* name, const char** labels, float columnWidth, float sensitivity)
{
	bool valueChange = false;
	ImGui::PushID(name);
	{
		ImGui::Columns(2);
		ImGui::SetColumnWidth(0, columnWidth);
		ImGui::Text("%s", name);
		ImGui::NextColumn();


		ImGui::PushMultiItemsWidths(3, ImGui::CalcItemWidth());
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{ 10, 0 });

		// x
		{
			ImGui::Text(labels[0]);
			ImGui::SameLine();
			valueChange |= ImGui::DragFloat("##X", &x, sensitivity, 0.0f, 0.0f, "%.2f");
			ImGui::PopItemWidth();
			ImGui::SameLine();
		}

		// y
		{
			ImGui::Text(labels[1]);
			ImGui::SameLine();
			valueChange |= ImGui::DragFloat("##Y", &y, sensitivity, 0.0f, 0.0f, "%.2f");
			ImGui::PopItemWidth();
			ImGui::SameLine();
		}

		// z
		{
			ImGui::Text(labels[2]);
			ImGui::SameLine();
			valueChange |= ImGui::DragFloat("##Z", &z, sensitivity, 0.0f, 0.0f, "%.2f");
			ImGui::PopItemWidth();
		}
		ImGui::PopStyleVar();
		ImGui::Columns(1);
	}
	ImGui::PopID();

	return valueChange;
}

bool ImGuiExt::DrawVec3(glm::vec3& v, const char* name, const char** labels, float columnWidth, float sensitivity)
{
	return DrawVec3(v.x, v.y, v.z, name, labels, columnWidth, sensitivity);
}

bool ImGuiExt::DrawQuaternion(glm::quat& q, const char* name, const char** labels, float columnWidth, float sensitivity)
{
	auto vec3Representation = glm::degrees(glm::eulerAngles(q));
	if (DrawVec3(vec3Representation, name, labels, columnWidth, sensitivity)) {
		q = glm::quat(glm::radians(vec3Representation));
		return true;
	}
	return false;
}
