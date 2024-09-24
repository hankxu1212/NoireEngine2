#include "EditorCore.h"
#include "ContentBrowserPanel.h"

namespace Noire {
	ImTextureID directoryIconId;
	ImTextureID fileIconId;
	ImTextureID backIconId;
	ImTextureID engineIconId;
	ImTextureID materialIconId;
	ImTextureID meshIconId;
	ImTextureID shaderIconId;

	ContentBrowserPanel::~ContentBrowserPanel()
	{
		GraphicsContext::Get()->WaitForCommands();
	}

	ContentBrowserPanel::ContentBrowserPanel()
		: m_BaseDirectory(Application::Get().GetSpecification().WorkingDirectory), m_CurrentDirectory(m_BaseDirectory)
	{
		m_DirectoryIcon = Texture2D::Create("textures/icon-folder-1.png");
		m_FileIcon = Texture2D::Create("textures/icon-file-1.png");
		m_BackIcon = Texture2D::Create("textures/icon-back.png");
		m_NoireEngineIcon = Texture2D::Create("textures/NE-icon.png");
		m_MaterialIcon = Texture2D::Create("textures/icon-mat.png");
		m_MeshIcon = Texture2D::Create("textures/icon-mesh.png");
		m_ShaderIcon = Texture2D::Create("textures/icon-shader.png");

		directoryIconId = m_DirectoryIcon->Register();
		fileIconId = m_FileIcon->Register();
		backIconId = m_BackIcon->Register();
		engineIconId = m_NoireEngineIcon->Register();
		materialIconId = m_MaterialIcon->Register();
		meshIconId = m_MeshIcon->Register();
		shaderIconId = m_ShaderIcon->Register();
	}

	void ContentBrowserPanel::Render()
	{
		static float padding = 16.0f;
		static float thumbnailSize = 128.0f;
		float cellSize = thumbnailSize + padding;

		float panelWidth = ImGui::GetContentRegionAvail().x;
		int columnCount = (int)(panelWidth / cellSize);
		if (columnCount < 1)
			columnCount = 1;

		if (m_CurrentDirectory != std::filesystem::path(m_BaseDirectory))
		{
			if (ImGui::ImageButton(backIconId, { 20, 20 }))
			{
				m_CurrentDirectory = m_CurrentDirectory.parent_path();
			}
		}

		ImGui::Columns(columnCount, 0, false);

		for (auto& directoryEntry : std::filesystem::directory_iterator(m_CurrentDirectory))
		{
			const auto& path = directoryEntry.path();
			std::string filenameString = path.filename().string();
			auto extension = directoryEntry.path().extension();

			ImGui::PushID(filenameString.c_str());

			ImTextureID icon;
			if (directoryEntry.is_directory())
				icon = directoryIconId;
			else if (extension == ".noire") {
				icon = engineIconId;
			}
			else if (extension == ".obj") {
				icon = meshIconId;
			}
			else if (extension == ".mat") {
				icon = materialIconId;
			}
			else if (extension == ".glsl" || extension == ".vert" || extension == ".frag" || extension == ".shader") {
				icon = shaderIconId;
			}
			else {
				icon = fileIconId;
			}

			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
			ImGui::ImageButton((ImTextureID)icon, { thumbnailSize, thumbnailSize });

			if (ImGui::BeginDragDropSource())
			{
				std::filesystem::path relativePath(path);
				const wchar_t* itemPath = relativePath.c_str();
				ImGui::SetDragDropPayload("CONTENT_BROWSER_ITEM", itemPath, (wcslen(itemPath) + 1) * sizeof(wchar_t));
				ImGui::EndDragDropSource();
			}

			ImGui::PopStyleColor();
			if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
			{
				if (directoryEntry.is_directory())
					m_CurrentDirectory /= path.filename();

				auto extension = directoryEntry.path().extension();

				NE_CORE_INFO("Opening {}", directoryEntry.path().string());
				if (extension == ".noire")
					SceneManager::Get()->OpenScene(directoryEntry.path());
			}
			ImGui::TextWrapped(filenameString.c_str());

			ImGui::NextColumn();

			ImGui::PopID();
		}

		ImGui::Columns(1);

		//ImGui::SliderFloat("Thumbnail Size", &thumbnailSize, 16, 512);
		//ImGui::SliderFloat("Padding", &padding, 0, 32);
	}

}
