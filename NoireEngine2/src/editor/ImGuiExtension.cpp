#include "ImGuiExtension.hpp"
#include "core/Core.hpp"

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

/*
	static bool DrawVec2(float& x, float& y, const char* name, const char** labels, float columnWidth = 100, float sensitivity = 0.1f) {
		bool valueChange = false;
		PushID(name);
		{
			Columns(2);
			SetColumnWidth(0, columnWidth);
			Text("%s", name);
			NextColumn();


			PushMultiItemsWidths(2, CalcItemWidth());
			PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{ 20, 0 });

			// x
			{
				Text(labels[0]);
				SameLine();
				valueChange |= DragFloat("##X", &x, sensitivity, 0.0f, 0.0f, "%.2f");
				PopItemWidth();
				SameLine();
			}

			// z
			{
				Text(labels[1]);
				SameLine();
				valueChange |= DragFloat("##Y", &y, sensitivity, 0.0f, 0.0f, "%.2f");
				PopItemWidth();
			}
			PopStyleVar();
			Columns(1);
		}
		PopID();

		return valueChange;
	}


	static bool DrawVec2(glm::vec2& v, const char* name, const char** labels, float columnWidth = 100, float sensitivity = 0.1f) {
		return DrawVec2(v.x, v.y, name, labels, columnWidth, sensitivity);
	}
	*/

bool ImGuiExt::DrawQuaternion(glm::quat& q, const char* name, const char** labels, float columnWidth, float sensitivity)
{
	auto vec3Representation = glm::degrees(glm::eulerAngles(q));
	if (DrawVec3(vec3Representation, name, labels, columnWidth, sensitivity)) {
		q = glm::quat(glm::radians(vec3Representation));
		return true;
	}
	return false;
}

void ImGuiExt::InspectTexture(const char** imguiIDs, const char* label, const char* path, int* id, float* multiplier)
{
	ImGui::SeparatorText(label);

	ImGui::PushID(imguiIDs[0]);
	ImGui::Columns(2);
	ImGui::Text("Path");
	ImGui::NextColumn();
	ImGui::Text(path);
	ImGui::Columns(1);
	ImGui::PopID();

	if (strcmp(path, NE_NULL_STR) == 0)
		return;

	ImGui::PushID(imguiIDs[1]);
	ImGui::Columns(2);
	ImGui::Text("ID");
	ImGui::NextColumn();
	ImGui::DragInt("##id", id, 1, -1, 4096);
	ImGui::Columns(1);
	ImGui::PopID();

	if (multiplier) {
		ImGui::PushID(imguiIDs[2]);
		ImGui::Columns(2);
		ImGui::Text("Multiplier");
		ImGui::NextColumn();
		ImGui::DragFloat("##multiplier", multiplier, 0.01f, 0.0f, FLT_MAX, "%.2f", ImGuiSliderFlags_AlwaysClamp);
		ImGui::Columns(1);
		ImGui::PopID();
	}
}
