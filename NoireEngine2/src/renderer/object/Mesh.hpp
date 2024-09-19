#pragma once

#include <filesystem>

#include "backend/buffers/Buffer.hpp"

template<typename T>
class Mesh
{
public:
	void LoadModel(std::filesystem::path& importPath);
private:
	Buffer m_VertexBuffer;
};

template<typename T>
inline void Mesh<T>::LoadModel(std::filesystem::path& importPath)
{
}
