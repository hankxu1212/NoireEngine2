#pragma once

#include "EditorCore.h"

namespace Noire {
	class ContentBrowserPanel
	{
	public:
		ContentBrowserPanel();

		~ContentBrowserPanel();

		void Render();

	private:
		std::filesystem::path m_BaseDirectory;
		std::filesystem::path m_CurrentDirectory;
		
		Ref<Texture2D> m_DirectoryIcon;
		Ref<Texture2D> m_FileIcon;
		Ref<Texture2D> m_BackIcon;
		Ref<Texture2D> m_NoireEngineIcon;
		Ref<Texture2D> m_MaterialIcon;
		Ref<Texture2D> m_MeshIcon;
		Ref<Texture2D> m_ShaderIcon;
	};
}

