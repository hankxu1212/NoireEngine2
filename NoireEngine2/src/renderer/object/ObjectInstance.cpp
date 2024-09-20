#include "ObjectInstance.hpp"

#include <vulkan/vulkan.h>
#include "backend/commands/CommandBuffer.hpp"
#include <iostream>
#include <array>

void ObjectInstance::BindMesh(const CommandBuffer& commandBuffer, uint32_t instanceID) const
{
    if (mesh == nullptr)
    {
        std::cerr << "Did not find active mesh on this object instance!\n";
        return;
    }

    std::array< VkBuffer, 1 > vertex_buffers{ mesh->vertexBuffer().getBuffer() };
    std::array< VkDeviceSize, 1 > offsets{ 0 };
    vkCmdBindVertexBuffers(commandBuffer, 0, uint32_t(vertex_buffers.size()), vertex_buffers.data(), offsets.data());
}

void ObjectInstance::Draw(const CommandBuffer& commandBuffer, uint32_t instanceID) const
{
    vkCmdDraw(commandBuffer, numVertices, 1, firstVertex, instanceID);
}