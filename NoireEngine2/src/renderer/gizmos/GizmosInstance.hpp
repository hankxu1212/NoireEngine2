#pragma once

#include "renderer/AABB.hpp"
#include "renderer/vertices/PosColVertex.hpp"

struct GizmosInstance 
{
    void DrawWiredCubeAroundAABB(const AABB& aabb)
    {
        m_LinesVertices.clear();

        // Calculate the 8 corners of the AABB
        PosColVertex corners[8] = {
            // Bottom face
            {{aabb.min.x, aabb.min.y, aabb.min.z}, Color4_4::Green}, // (minX, minY, minZ)
            {{aabb.max.x, aabb.min.y, aabb.min.z}, Color4_4::Green}, // (maxX, minY, minZ)
            {{aabb.min.x, aabb.max.y, aabb.min.z}, Color4_4::Green}, // (minX, maxY, minZ)
            {{aabb.max.x, aabb.max.y, aabb.min.z}, Color4_4::Green}, // (maxX, maxY, minZ)
            // Top face
            {{aabb.min.x, aabb.min.y, aabb.max.z}, Color4_4::Green}, // (minX, minY, maxZ)
            {{aabb.max.x, aabb.min.y, aabb.max.z}, Color4_4::Green}, // (maxX, minY, maxZ)
            {{aabb.min.x, aabb.max.y, aabb.max.z}, Color4_4::Green}, // (minX, maxY, maxZ)
            {{aabb.max.x, aabb.max.y, aabb.max.z}, Color4_4::Green}, // (maxX, maxY, maxZ)
        };


        AddEdge(corners[0], corners[1]); // Bottom face: (minX, minY, minZ) to (maxX, minY, minZ)
        AddEdge(corners[0], corners[2]); // Bottom face: (minX, minY, minZ) to (minX, maxY, minZ)
        AddEdge(corners[1], corners[3]); // Bottom face: (maxX, minY, minZ) to (maxX, maxY, minZ)
        AddEdge(corners[2], corners[3]); // Bottom face: (minX, maxY, minZ) to (maxX, maxY, minZ)

        AddEdge(corners[4], corners[5]); // Top face: (minX, minY, maxZ) to (maxX, minY, maxZ)
        AddEdge(corners[4], corners[6]); // Top face: (minX, minY, maxZ) to (minX, maxY, maxZ)
        AddEdge(corners[5], corners[7]); // Top face: (maxX, minY, maxZ) to (maxX, maxY, maxZ)
        AddEdge(corners[6], corners[7]); // Top face: (minX, maxY, maxZ) to (maxX, maxY, maxZ)

        AddEdge(corners[0], corners[4]); // Side: (minX, minY, minZ) to (minX, minY, maxZ)
        AddEdge(corners[1], corners[5]); // Side: (maxX, minY, minZ) to (maxX, minY, maxZ)
        AddEdge(corners[2], corners[6]); // Side: (minX, maxY, minZ) to (minX, maxY, maxZ)
        AddEdge(corners[3], corners[7]); // Side: (maxX, maxY, minZ) to (maxX, maxY, maxZ)
    }

    void DrawWiredSphere(float radius, const glm::vec3& center, Color4_4 color=Color4_4::White)
    {
        m_LinesVertices.clear();
        constexpr uint32_t segments = 20;

        for (int i = 0; i < segments; ++i) 
        {
            float theta1 = glm::pi<float>() * (-0.5f + float(i) / float(segments));    // Latitude angle for segment i
            float theta2 = glm::pi<float>() * (-0.5f + float(i + 1) / float(segments)); // Next latitude angle

            float phi1 = 2.0f * glm::pi<float>() * float(i) / float(segments);         // Longitude angle for equator
            float phi2 = 2.0f * glm::pi<float>() * float(i + 1) / float(segments);     // Next longitude angle

            // Create smooth line along the equator (latitude = 0)
            AddEdge(
                { {radius * cos(phi1), radius * sin(phi1), 0}, color },
                { {radius * cos(phi2), radius * sin(phi2), 0}, color }
            );

            float rc1 = radius * cos(theta1);
            float rc2 = radius * cos(theta2);
            // create 4 lines
            for (float l = 0; l < 2 * glm::pi<float>(); l += glm::half_pi<float>())
            {
                AddEdge(
                    { {rc1 * cos(l), rc1 * sin(l), radius * sin(theta1) }, color },
                    { {rc2 * cos(l), rc2 * sin(l), radius * sin(theta2) }, color }
                );
            }
        }
    }

    // Helper function to add edges
    void AddEdge(const PosColVertex& start, const PosColVertex& end)
    {
        m_LinesVertices.push_back(start);
        m_LinesVertices.push_back(end);
    };

    std::vector<PosColVertex> m_LinesVertices;
};
