#pragma once

#include "imgui/imgui.h"
#include "renderer/scene/Entity.hpp"

#define MAX_NAME_LEN 40

class EntityEditorWidget {
public:
	EntityEditorWidget() = default;

	void SetDebugMode(bool b) { debugMode = b; }

	void Close() { show = false; }

	void Show() { show = true; }

	void RenderEditor(Entity* e, ImGuiWindowFlags editor_window_flags);

private:
	Entity* activeEntity = nullptr;
	const uint8_t maxNameLength = MAX_NAME_LEN;
	char nameBuffer[MAX_NAME_LEN];
	bool show = true;
	bool debugMode = false;
};
