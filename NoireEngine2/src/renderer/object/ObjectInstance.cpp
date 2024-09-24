#include "ObjectInstance.hpp"

#include "backend/commands/CommandBuffer.hpp"
#include "Mesh.hpp"

#include <vulkan/vulkan.h>
#include <iostream>
#include <array>

bool ObjectInstance::BindMesh(const CommandBuffer& commandBuffer, uint32_t instanceID) const
{
    if (mesh == nullptr)
    {
        std::cerr << "Did not find active mesh on this object instance!\n";
        return false;
    }

    if (mesh->vertexBuffer().getBuffer() == VK_NULL_HANDLE)
    {
        std::cerr << "Mesh vertex buffer is null\n";
        return false;
    }

    std::array< VkBuffer, 1 > vertex_buffers{ mesh->vertexBuffer().getBuffer() };
    std::array< VkDeviceSize, 1 > offsets{ 0 };
    vkCmdBindVertexBuffers(commandBuffer, 0, uint32_t(vertex_buffers.size()), vertex_buffers.data(), offsets.data());

    return true;
}

void ObjectInstance::BindVertexInput(const CommandBuffer& commandBuffer) const
{
    mesh->getVertexInput()->Bind(commandBuffer);
}

void ObjectInstance::Draw(const CommandBuffer& commandBuffer, uint32_t instanceID) const
{
    vkCmdDraw(commandBuffer, numVertices, 1, firstVertex, instanceID);
}