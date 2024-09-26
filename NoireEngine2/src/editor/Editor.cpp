#include "Editor.hpp"

#include "Application.hpp"
#include "imgui/imgui.h"
#include "renderer/scene/SceneManager.hpp"
#include "backend/pipeline/ObjectPipeline.hpp"

Editor* Editor::g_Editor = nullptr;

void Editor::OnAttach()
{
    open = true;
    //ImGuizmo::SetImGuiContext(ImGui::GetCurrentContext());
}

void Editor::OnUpdate()
{
}

void Editor::OnDetach()
{
    open = false;
}

void Editor::OnEvent(Event& e)
{
}

void Editor::OnImGuiRender()
{
    Display();
}

void Editor::OnViewportRender()
{
    ShowGizmos();
}

void Editor::Display()
{
    if (!open)
        return;

    if (statsOnly) 
    {
        ShowStats();
        return;
    }
    SetupDockspace();
    ShowMainMenuBar();
    //if (m_EditorInfo.show_imgui_demo)       ImGui::ShowDemoWindow();
    if (m_EditorInfo.show_hierarchy)        ShowHierarchy();
    //if (m_EditorInfo.show_scene_view)       ShowSceneView();
    //if (m_EditorInfo.show_game_view)        ShowGameView();
    //if (m_EditorInfo.show_asset_browser)    ShowAssetBrowser();
    if (m_EditorInfo.show_settings)         ShowStats();
}

void Editor::ShowMainMenuBar()
{
    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            ShowFileMenu();
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Edit"))
        {
            ShowEditMenu();
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Tools"))
        {
            ShowToolsMenu();
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Build"))
        {
            ShowBuildMenu();
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Window"))
        {
            ShowWindowMenu();
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Help"))
        {
            ShowHelpMenu();
            ImGui::EndMenu();
        }

        ImGui::EndMainMenuBar();
    }
}

void Editor::SetupDockspace()
{
    ImGui::DockSpaceOverViewport(ImGui::GetMainViewport()->ID, ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);
}

void Editor::ShowFileMenu()
{
    /*
    if (ImGui::MenuItem("New Project")) {
    }

    if (ImGui::MenuItem("Open Project", "Ctrl+O")) {
    }

    if (ImGui::BeginMenu("Open Recent")) {
        ImGui::EndMenu();
    }

    if (ImGui::MenuItem("Save", "Ctrl+S")) {
    }

    if (ImGui::MenuItem("Save As..")) {
    }

    ImGui::Separator();

    if (ImGui::BeginMenu("Options")) {
        ImGui::EndMenu();
    }

    ImGui::Separator();

    if (ImGui::MenuItem("New Scene", "Ctrl+N")) {
        //SceneManager::Get()->NewScene();
    }

    if (ImGui::MenuItem("Save Scene", "Ctrl+S")) {
        //SceneManager::Get()->SaveScene();
    }

    if (ImGui::MenuItem("Save Scene As...", "Ctrl+Shift+S")) {
        //SceneManager::Get()->SaveSceneAs();
    }

    if (ImGui::MenuItem("Open Scene", "Ctrl+Shift+O")) {
        //SceneManager::Get()->OpenScenePrompt();
    }

    ImGui::Separator();
    */

    if (ImGui::MenuItem("Quit", "Alt+F4")) {
        Application::Get().Close();
    }
}

void Editor::ShowEditMenu()
{

}

void Editor::ShowToolsMenu()
{

}

void Editor::ShowBuildMenu()
{

}

void Editor::ShowWindowMenu()
{

}

void Editor::ShowHelpMenu()
{

}

void Editor::ShowHierarchy()
{
    if (!ImGui::Begin("Hierarchy", &m_EditorInfo.show_hierarchy, m_EditorInfo.window_flags))
    {
        ImGui::End();
        return;
    }

    s_HierarchyPanel->Render(m_EditorInfo.window_flags);

    ImGui::End();
}

void Editor::ShowSceneView()
{
    if (!ImGui::Begin("Scene View", &m_EditorInfo.show_scene_view, m_EditorInfo.window_flags))
    {
        ImGui::End();
        return;
    }
    ImGui::End();
}

void Editor::ShowGameView()
{
    if (!ImGui::Begin("Game View", &m_EditorInfo.show_game_view, m_EditorInfo.window_flags))
    {
        ImGui::End();
        return;
    }
    ImGui::End();
}

void Editor::ShowAssetBrowser() {
    if (!ImGui::Begin("Assets", &m_EditorInfo.show_hierarchy, m_EditorInfo.window_flags))
    {
        ImGui::End();
        return;
    }

    //s_ContentBrowerPanel->Render();

    ImGui::End();
}

void Editor::ShowStats()
{
    ImGuiWindowFlags window_flags = m_EditorInfo.window_flags
        | ImGuiWindowFlags_NoDecoration
        | ImGuiWindowFlags_NoDocking
        | ImGuiWindowFlags_AlwaysAutoResize
        | ImGuiWindowFlags_NoSavedSettings
        | ImGuiWindowFlags_NoFocusOnAppearing
        | ImGuiWindowFlags_NoNav;

    static int location = 3;
    if (location >= 0)
    {
        const float PAD = 10.0f;
        const ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImVec2 work_pos = viewport->WorkPos; // Use work area to avoid menu-bar/task-bar, if any!
        ImVec2 work_size = viewport->WorkSize;
        ImVec2 window_pos, window_pos_pivot;
        window_pos.x = (location & 1) ? (work_pos.x + work_size.x - PAD) : (work_pos.x + PAD);
        window_pos.y = (location & 2) ? (work_pos.y + work_size.y - PAD) : (work_pos.y + PAD);
        window_pos_pivot.x = (location & 1) ? 1.0f : 0.0f;
        window_pos_pivot.y = (location & 2) ? 1.0f : 0.0f;
        ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, window_pos_pivot);
        ImGui::SetNextWindowViewport(viewport->ID);
        window_flags |= ImGuiWindowFlags_NoMove;
    }
    else if (location == -2)
    {
        // Center window
        ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
        window_flags |= ImGuiWindowFlags_NoMove;
    }
    ImGui::SetNextWindowBgAlpha(0.35f); // Transparent background
    if (ImGui::Begin("Settings Overlay", &m_EditorInfo.show_settings, window_flags))
    {
        // fps
        ImGui::Text("FPS: %d", Application::GetFPS());
        ImGui::Separator(); // -----------------------------------------------------

        // specified physical device
        if (Application::GetSpecification().PhysicalDeviceName) {
            ImGui::Text("Specified Physical Device: %s", Application::GetSpecification().PhysicalDeviceName.value().c_str());
            ImGui::Separator(); // -----------------------------------------------------
        }

        // culling mode
        static auto cullMode = Application::GetSpecification().Culling;
        if (cullMode == ApplicationSpecification::Culling::None)
            ImGui::Text("Culling Mode: none");
        else if (cullMode == ApplicationSpecification::Culling::Frustum)
            ImGui::Text("Culling Mode: frustum");
        ImGui::Separator(); // -----------------------------------------------------

        // num objects drawn
        ImGui::Text("Number of Objects Drawn: %I64u", ObjectPipeline::ObjectsDrawn);
        ImGui::Separator(); // -----------------------------------------------------

        // camera mode
        static const char* items[]{ "Scene","User","Debug" };
        static int Selecteditem = 0;
        if (ImGui::Combo("Camera Mode", &Selecteditem, items, IM_ARRAYSIZE(items)))
        {
            SceneManager::Get()->SetCameraMode((Scene::CameraMode)Selecteditem);
        }
        ImGui::Separator(); // -----------------------------------------------------

        // enable UI
        ImGui::Columns(2);
        ImGui::Text("%s", "Stats Only UI");
        ImGui::NextColumn();
        ImGui::Checkbox("##STATSONLYUI", &statsOnly);
        ImGui::Columns(1);
        ImGui::Separator(); // -----------------------------------------------------

        // rendering stats
        // TODO: num threads
        ImGui::Text("Rendering Information");
        ImGui::Separator(); // -----------------------------------------------------
        ImGui::BulletText("Application Update Time: %.3fms", Application::ApplicationUpdateTime);
        ImGui::BulletText("Application Render Time: %.3fms", Application::ApplicationRenderTime);
        ImGui::Indent(20);
            ImGui::BulletText("Swapchain Wait Time: %.3fms", VulkanContext::WaitForSwapchainTime);
            ImGui::BulletText("Draw Call Time: %.3fms", VulkanContext::RenderTime);
            ImGui::Indent(20);
                ImGui::BulletText("Object Render Time: %.3fms", Renderer::ObjectRenderTime);
                ImGui::BulletText("UI Render Time: %.3fms", Renderer::UIRenderTime);
        ImGui::Unindent(40);

        if (ImGui::BeginPopupContextWindow())
        {
            if (ImGui::MenuItem("Custom", NULL, location == -1)) location = -1;
            if (ImGui::MenuItem("Center", NULL, location == -2)) location = -2;
            if (ImGui::MenuItem("Top-left", NULL, location == 0)) location = 0;
            if (ImGui::MenuItem("Top-right", NULL, location == 1)) location = 1;
            if (ImGui::MenuItem("Bottom-left", NULL, location == 2)) location = 2;
            if (ImGui::MenuItem("Bottom-right", NULL, location == 3)) location = 3;
            ImGui::Separator(); // -----------------------------------------------------
            if (&m_EditorInfo.show_settings && ImGui::MenuItem("Close")) m_EditorInfo.show_settings = false;
            ImGui::EndPopup();
        }
    }
    ImGui::End();
}

void Editor::ShowGizmos()
{
    if (!m_EditorInfo.show_gizmos)
        return;

    /*
    Entity selectedEntity = s_HierarchyPanel->getSelectedEntity();
    if (selectedEntity && m_GizmoType != -1 && selectedEntity.HasComponent<TransformComponent>())
    {
        ImGuizmo::SetDrawlist();

        auto& viewportBounds = Application::Get().GetImGuiLayer()->getViewportBounds();
        ImGuizmo::SetRect(viewportBounds.x, viewportBounds.y, viewportBounds.z - viewportBounds.x, viewportBounds.w - viewportBounds.y);

        // Editor camera
        auto sceneCamera = SceneManager::Get()->getScene()->getMainCamera();
        glm::mat4& cameraProjection = sceneCamera->GetProjectionMatrixUnsafe();
        const glm::mat4& cameraView = sceneCamera->getViewMatrix();

        // Entity transform
        auto& tc = selectedEntity.GetComponent<TransformComponent>();
        glm::mat4 transform = tc.transform.Local();

        // Snapping
        bool snap = Input::IsKeyPressed(Key::LeftControl);
        float snapValue = 0.5f; // Snap to 0.5m for translation/scale
        // Snap to 45 degrees for rotation
        if (m_GizmoType == ImGuizmo::OPERATION::ROTATE)
            snapValue = 45.0f;

        float snapValues[3] = { snapValue, snapValue, snapValue };

        cameraProjection[1][1] *= -1; // invert to opengl y-up
        ImGuizmo::Manipulate(glm::value_ptr(cameraView), glm::value_ptr(cameraProjection),
            (ImGuizmo::OPERATION)m_GizmoType, ImGuizmo::LOCAL, glm::value_ptr(transform),
            nullptr, snap ? snapValues : nullptr);
        cameraProjection[1][1] *= -1; // invert back

        if (ImGuizmo::IsUsing())
        {
            glm::vec3 translation, scale;
            glm::quat rotation;
            Transform::Decompose(transform, translation, rotation, scale);

            tc.transform.position = translation;
            tc.transform.rotation = rotation;
            tc.transform.scale = scale;
        }
    }
    */
}

