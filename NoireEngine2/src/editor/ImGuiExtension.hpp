/*****************************************************************//**
 * \file   ImGuiExtension.hpp
 * \brief  Custom extension of ImGui for Noire Engine
 * 
 * \author Hank Xu
 * \date   March 2024
 *********************************************************************/

#pragma once

#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"
#define GLM_ENABLE_EXPERIMENTAL
#include "glm/glm.hpp"
#include "glm/gtx/quaternion.hpp"

using namespace ImGui;

class ImGuiExt
{
public:
	static bool DrawVec3(float& x, float& y, float& z, const char* name, const char** labels, float columnWidth = 100, float sensitivity = 0.1f);

	static bool DrawVec3(glm::vec3& v, const char* name, const char** labels, float columnWidth = 100, float sensitivity = 0.1f);

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

	static bool DrawQuaternion(glm::quat& q, const char* name, const char** labels, float columnWidth = 100, float sensitivity = 0.1f);

	template<typename ...Args>
	static void ColumnDragFloat(const char* text, Args... args) {
		Columns(2);
		Text(text);
		NextColumn();
		DragFloat(args...);
		Columns(1);
	}
};
