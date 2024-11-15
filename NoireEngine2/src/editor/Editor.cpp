#include "Editor.hpp"

#include "Application.hpp"
#include "renderer/scene/SceneManager.hpp"
#include "renderer/Renderer.hpp"
#include "imguizmo/ImGuizmo.h"

Editor* Editor::g_Editor = nullptr;

void Editor::OnAttach()
{
    open = true;
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
    e.Dispatch<KeyPressedEvent>(NE_BIND_EVENT_FN(Editor::OnKeyPressed));
    e.Dispatch<MouseButtonReleasedEvent>(NE_BIND_EVENT_FN(Editor::OnMouseReleasedEvent));
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

    if (hideInspector) 
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

bool showShadowMenu = true;

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
        auto cullMode = Application::GetSpecification().Culling;
        if (cullMode == ApplicationSpecification::Culling::None)
            ImGui::Text("Culling Mode: none");
        else if (cullMode == ApplicationSpecification::Culling::Frustum)
            ImGui::Text("Culling Mode: frustum");
        ImGui::Separator(); // -----------------------------------------------------

        // camera mode
        static const char* items[]{ "Scene","User","Debug" };
        static int Selecteditem = (int)SceneManager::Get()->getCameraMode(); // gets the initial state
        if (ImGui::Combo("Camera Mode", &Selecteditem, items, IM_ARRAYSIZE(items)))
        {
            SceneManager::Get()->SetCameraMode((Scene::CameraMode)Selecteditem);
        }
        ImGui::Separator(); // -----------------------------------------------------

        // enable UI
        ImGui::Checkbox("Hide Inspector", &hideInspector);
        ImGui::Separator(); // -----------------------------------------------------

        Renderer::Instance->OnUIRender();

        if (ImGui::CollapsingHeader("Shadow Settings", &showShadowMenu))
        {
            ImGui::SeparatorText("Shadows"); // -----------------------------------------------------
            ImGui::Columns(2);
            ImGui::Text("PCF Samples");
            ImGui::NextColumn();
            ImGui::DragInt("##PCFSAMPLES", (int*)&ShadowPipeline::PCFSamples, 1, 1, 64);
            ImGui::Columns(1);
            ImGui::Separator(); // -----------------------------------------------------

            ImGui::Columns(2);
            ImGui::Text("PCSS Occluder Samples");
            ImGui::NextColumn();
            ImGui::DragInt("##PCSSOCCLUDERSAMPLES", (int*)&ShadowPipeline::PCSSOccluderSamples, 1, 1, 64);
            ImGui::Columns(1);
            ImGui::Separator(); // -----------------------------------------------------
        }

        if (ImGui::BeginPopupContextWindow())
        {
            if (ImGui::MenuItem("Custom", NULL, location == -1)) location = -1;
            if (ImGui::MenuItem("Center", NULL, location == -2)) location = -2;
            if (ImGui::MenuItem("Top-left", NULL, location == 0)) location = 0;
            if (ImGui::MenuItem("Top-right", NULL, location == 1)) location = 1;
            if (ImGui::MenuItem("Bottom-left", NULL, location == 2)) location = 2;
            if (ImGui::MenuItem("Bottom-right", NULL, location == 3)) location = 3;
            ImGui::Separator(); // -----------------------------------------------------
            
            if (ImGui::MenuItem("Close")) 
                m_EditorInfo.show_settings = false;
            ImGui::EndPopup();
        }
    }
    ImGui::End();
}

bool Editor::OnKeyPressed(KeyPressedEvent& e)
{
    if (e.m_IsRepeat)
        return false;

    //bool control = NativeInput::IsKeyPressed(Key::LeftControl) || NativeInput::IsKeyPressed(Key::RightControl);
    //bool shift = NativeInput::IsKeyPressed(Key::LeftShift) || NativeInput::IsKeyPressed(Key::RightShift);

    switch (e.m_KeyCode)
    {
    // Scene Commands
    //case Key::D:
    //{
    //    if (control)
    //        OnDuplicateEntity();
    //    break;
    //}

    // Gizmos
    case Key::Q:
    {
        if (!ImGuizmo::IsUsing())
            m_GizmoType = -1;
        break;
    }
    case Key::G:
    {
        if (!ImGuizmo::IsUsing())
            m_GizmoType = ImGuizmo::OPERATION::TRANSLATE;
        break;
    }
    case Key::R:
    {
        if (!ImGuizmo::IsUsing())
            m_GizmoType = ImGuizmo::OPERATION::ROTATE;
        break;
    }
    case Key::S:
    {
        /*if (control)
        {
            if (shift)
                SceneManager::Get()->SaveSceneAs();
            else
                SceneManager::Get()->SaveScene();
        }
        else */
        if (!ImGuizmo::IsUsing())
            m_GizmoType = ImGuizmo::OPERATION::SCALE;
        break;
    }
    /*
    case Key::Delete:
    {
        if (Application::Get().GetImGuiLayer()->getActiveWidgetID() == 0)
        {
            Entity selectedEntity = s_HierarchyPanel->getSelectedEntity();
            if (selectedEntity)
            {
                s_HierarchyPanel->SetSelectedEntity({});
                SceneManager::Get()->getScene()->DestroyEntity(selectedEntity);
            }
        }
        break;
    }*/
    }

    return false;
}

bool Editor::OnMouseReleasedEvent(MouseButtonReleasedEvent& e)
{
    if (e.m_Button == Mouse::ButtonLeft) 
    {
        UUID selectedID = Renderer::Instance->QueryMouseHoveredEntity();
        if (selectedID != 0)
        {
            Entity* ent = SceneManager::Get()->getScene()->FindEntity(selectedID);
            assert(ent);

            s_HierarchyPanel->SetSelectedEntity(ent);

            m_SelectedRendererComponent = ent->GetComponent<RendererComponent>();
            assert(m_SelectedRendererComponent);
        }
        else {
            s_HierarchyPanel->SetSelectedEntity(nullptr);
            m_SelectedRendererComponent = nullptr;
        }
    }
    return false;
}

void Editor::ShowGizmos()
{
    if (!m_EditorInfo.show_gizmos)
        return;

    Entity* selectedEntity = s_HierarchyPanel->getSelectedEntity();
    if (selectedEntity && m_GizmoType != -1)
    {
        ImGuizmo::SetDrawlist(ImGui::GetBackgroundDrawList());

        // Editor camera
        auto sceneCamera = VIEW_CAM;
        glm::mat4& cameraView = sceneCamera->GetViewMatrixUnsafe();
        glm::mat4& cameraProjection = sceneCamera->GetProjectionMatrixUnsafe();

        auto content = ImGui::GetContentRegionAvail();
        ImGuizmo::SetRect(ImGui::GetWindowPos().x, ImGui::GetWindowPos().y, content.x, content.y);

        // Entity transform
        glm::mat4 transformMat = selectedEntity->transform()->GetLocal();

        // Snapping
        bool snap = NativeInput::GetKeyPressed(Key::LeftControl);
        float snapValue = 0.5f; // Snap to 0.5m for translation/scale
        // Snap to 45 degrees for rotation
        if (m_GizmoType == ImGuizmo::OPERATION::ROTATE)
            snapValue = 45.0f;

        float snapValues[3] = { snapValue, snapValue, snapValue };

        cameraProjection[1][1] *= -1; // invert to opengl y-up
        ImGuizmo::Manipulate(glm::value_ptr(cameraView), glm::value_ptr(cameraProjection),
            (ImGuizmo::OPERATION)m_GizmoType, ImGuizmo::WORLD, glm::value_ptr(transformMat),
            nullptr, snap ? snapValues : nullptr);
        cameraProjection[1][1] *= -1; // invert back

        if (ImGuizmo::IsUsing())
        {
            selectedEntity->transform()->Decompose(transformMat);
        }
    }
}

