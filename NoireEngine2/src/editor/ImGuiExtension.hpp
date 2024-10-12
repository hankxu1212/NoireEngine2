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

	static bool DrawVec2(float& x, float& y, const char* name, const char** labels, float columnWidth = 100, float sensitivity = 0.1f);

	static bool DrawVec2(glm::vec2& v, const char* name, const char** labels, float columnWidth = 100, float sensitivity = 0.1f);

	static bool DrawQuaternion(glm::quat& q, const char* name, const char** labels, float columnWidth = 100, float sensitivity = 0.1f);

	template<typename ...Args>
	static void ColumnDragFloat(const char* text, Args... args) {
		Columns(2);
		Text(text);
		NextColumn();
		DragFloat(args...);
		Columns(1);
	}

	static void InspectTexture(const char** imguiIDs, const char* label, const char* path, int* id, float* multiplier = nullptr, float max=FLT_MAX);
};
