#pragma once

#include "core/layers/Layer.hpp"
#include "panels/SceneHierarchyPanel.h"
#include "imgui/imgui.h"

class Editor : public Layer
{
	static Editor* g_Editor;

	struct EditorInfo
	{
		ImGuiWindowFlags window_flags = 0;
		bool show_imgui_demo = true;
		bool use_dockspace = true;
		bool show_hierarchy = true;
		bool show_scene_view = false;
		bool show_game_view = false;
		bool show_asset_browser = true;
		bool show_settings = true;
		bool show_gizmos = true;
	};

public:
	Editor() : Layer("Editor")
	{
		g_Editor = this;

		s_HierarchyPanel = std::make_unique<SceneHierarchyPanel>();
		//s_ContentBrowerPanel = CreateScope<ContentBrowserPanel>();
	}

	static Editor* Get() { return g_Editor; }

public:
	void OnAttach() override;
	void OnUpdate() override;
	void OnDetach() override;
	void OnEvent(Event& e) override;
	void OnImGuiRender() override;
	void OnViewportRender() override;

private:
	void Display();

	void SetupDockspace();

	void ShowMainMenuBar();
	void ShowFileMenu();
	void ShowEditMenu();
	void ShowToolsMenu();
	void ShowBuildMenu();
	void ShowWindowMenu();
	void ShowHelpMenu();

	void ShowHierarchy();
	void ShowSceneView();
	void ShowGameView();
	void ShowAssetBrowser();

	void ShowGizmos();
	void ShowStats();

private:
	bool OnKeyPressed(KeyPressedEvent& e);

private:
	std::unique_ptr<SceneHierarchyPanel> s_HierarchyPanel;
	//std::unique_ptr<ContentBrowserPanel> s_ContentBrowerPanel;

	EditorInfo		m_EditorInfo;
	bool			open = true;
	bool			statsOnly = false;
	int				m_GizmoType = -1;
};

