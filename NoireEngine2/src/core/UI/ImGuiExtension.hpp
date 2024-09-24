/*****************************************************************//**
 * \file   ImGuiExtension.hpp
 * \brief  Custom extension of ImGui for Noire Engine
 * 
 * \author Hank Xu
 * \date   March 2024
 *********************************************************************/

#include "imgui/imgui.h"
#include "glm/glm.hpp"
#include "glm/gtx/quaternion.hpp"

using namespace ImGui;

namespace ImGuiExt {
	static const ImVec4 btn_red = ImVec4{ 0.8f, 0.1f, 0.15f, 1.0f };
	static const ImVec4 btn_red_hover = ImVec4{ 0.9f, 0.2f, 0.2f, 1.0f };
	static const ImVec4 btn_red_active = ImVec4{ 0.8f, 0.1f, 0.15f, 1.0f };

	static const ImVec4 btn_blue = ImVec4{ 0.1f, 0.25f, 0.8f, 1.0f };
	static const ImVec4 btn_blue_hover = ImVec4{ 0.2f, 0.35f, 0.9f, 1.0f };
	static const ImVec4 btn_blue_active = ImVec4{ 0.1f, 0.25f, 0.8f, 1.0f };

	static const ImVec4 btn_green = ImVec4{ 0.2f, 0.7f, 0.2f, 1.0f };
	static const ImVec4 btn_green_hover = ImVec4{ 0.3f, 0.8f, 0.3f, 1.0f };
	static const ImVec4 btn_green_active = ImVec4{ 0.2f, 0.7f, 0.2f, 1.0f };

	// dont forget PopStyleColor(3);
	static void PushRedButton() {
		PushStyleColor(ImGuiCol_Button, btn_red);
		PushStyleColor(ImGuiCol_ButtonHovered, btn_red_hover);
		PushStyleColor(ImGuiCol_ButtonActive, btn_red_active);
	}

	// dont forget PopStyleColor(3);
	static void PushBlueButton() {
		PushStyleColor(ImGuiCol_Button, btn_blue);
		PushStyleColor(ImGuiCol_ButtonHovered, btn_blue_hover);
		PushStyleColor(ImGuiCol_ButtonActive, btn_blue_active);
	}


	// dont forget PopStyleColor(3);
	static void PushGreenButton() {
		PushStyleColor(ImGuiCol_Button, btn_green);
		PushStyleColor(ImGuiCol_ButtonHovered, btn_green_hover);
		PushStyleColor(ImGuiCol_ButtonActive, btn_green_active);
	}

	static bool DrawVec3(float& x, float& y, float& z, const char* name, const char** labels, float columnWidth = 100, float sensitivity = 0.1f) {
		bool valueChange = false;
		PushID(name);
		{
			Columns(2);
			SetColumnWidth(0, columnWidth);
			Text("%s", name);
			NextColumn();


			PushMultiItemsWidths(3, CalcItemWidth());
			PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{ 10, 0 });

			// x
			{
                ImGui::Text(labels[0]);
				SameLine();
				valueChange |= DragFloat("##X", &x, sensitivity, 0.0f, 0.0f, "%.2f");
				PopItemWidth();
				SameLine();
			}

			// y
			{
                ImGui::Text(labels[1]);
				SameLine();
				valueChange |= DragFloat("##Y", &y, sensitivity, 0.0f, 0.0f, "%.2f");
				PopItemWidth();
				SameLine();
			}

			// z
			{
                ImGui::Text(labels[2]);
				SameLine();
				valueChange |= DragFloat("##Z", &z, sensitivity, 0.0f, 0.0f, "%.2f");
				PopItemWidth();
			}
			PopStyleVar();
			Columns(1);
		}
		PopID();

        return valueChange;
	}

	static bool DrawVec3(glm::vec3& v, const char* name, const char** labels, float columnWidth = 100, float sensitivity = 0.1f) {
		return DrawVec3(v.x, v.y, v.z, name, labels, columnWidth, sensitivity);
	}

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
				ImGui::Text(labels[0]);
				SameLine();
				valueChange |= DragFloat("##X", &x, sensitivity, 0.0f, 0.0f, "%.2f");
				PopItemWidth();
				SameLine();
			}

			// z
			{
				ImGui::Text(labels[1]);
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

	static void DrawQuaternion(glm::quat& q, const char* name, const char** labels, float columnWidth = 100, float sensitivity = 0.1f)
	{
		auto vec3Representation = glm::degrees(glm::eulerAngles(q));
		if (DrawVec3(vec3Representation, name, labels, columnWidth, sensitivity))
			q = glm::quat(glm::radians(vec3Representation));
	}

	template<typename ...Args>
	static void ColumnDragFloat(const char* text, Args... args) {
		ImGui::Columns(2);
		ImGui::Text(text);
		ImGui::NextColumn();
		ImGui::DragFloat(args...);
		ImGui::Columns(1);
	}
}
