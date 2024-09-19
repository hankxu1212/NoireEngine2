#include "ObjectInstance.hpp"


#include <vulkan/vulkan.h>
#include "backend/commands/CommandBuffer.hpp"

ObjectInstance::BindMesh(const CommandBuffer& commandBuffer, uint32_t instanceID)
{
    if (mesh == nullptr)
    {
        std::cerr << "Did not find active mesh on this object instance!\n";
        return;
    }

    std::array< VkBuffer, 1 > vertex_buffers{ mesh.vertexBuffer().getBuffer() };
    std::array< VkDeviceSize, 1 > offsets{ 0 };
    vkCmdBindVertexBuffers(commandBuffer, 0, uint32_t(vertex_buffers.size()), vertex_buffers.data(), offsets.data());
}

ObjectInstance::Draw(const CommandBuffer& commandBuffer, uint32_t instanceID)
{
    vkCmdDraw(commandBuffer, inst.numVertices, 1, inst.firstVertex, index);
}