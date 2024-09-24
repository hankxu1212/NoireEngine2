#pragma once

#include "editor/widgets/EntityEditorWidget.h"

class SceneHierarchyPanel
{
public:
	SceneHierarchyPanel();

	void Render(ImGuiWindowFlags editor_window_flags);

	inline Entity* getSelectedEntity() const { return m_SelectedEntity; }
	void SetSelectedEntity(Entity* entity);

private:
	template<typename T>
	void DisplayAddComponentEntry(const std::string& entryName);

	void DrawEntityNode(Entity* entity);

private:
	Entity* m_SelectedEntity = nullptr;
	EntityEditorWidget entityEditor;
};
