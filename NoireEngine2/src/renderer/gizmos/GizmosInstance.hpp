#pragma once

#include "renderer/AABB.hpp"
#include "renderer/vertices/PosColVertex.hpp"

struct GizmosInstance 
{
    void DrawWireCube2(const glm::vec3& center, const glm::vec3& halfExtent, Color4_4 color = Color4_4::White) {
        m_LinesVertices.clear();

        // Define the 8 vertices of the cube
        glm::vec3 v0 = center + glm::vec3(-halfExtent.x, -halfExtent.y, -halfExtent.z);
        glm::vec3 v1 = center + glm::vec3(halfExtent.x, -halfExtent.y, -halfExtent.z);
        glm::vec3 v2 = center + glm::vec3(halfExtent.x, halfExtent.y, -halfExtent.z);
        glm::vec3 v3 = center + glm::vec3(-halfExtent.x, halfExtent.y, -halfExtent.z);
        glm::vec3 v4 = center + glm::vec3(-halfExtent.x, -halfExtent.y, halfExtent.z);
        glm::vec3 v5 = center + glm::vec3(halfExtent.x, -halfExtent.y, halfExtent.z);
        glm::vec3 v6 = center + glm::vec3(halfExtent.x, halfExtent.y, halfExtent.z);
        glm::vec3 v7 = center + glm::vec3(-halfExtent.x, halfExtent.y, halfExtent.z);

        // Create the 12 edges of the cube
        AddEdge(v0, v1, color); // Bottom face
        AddEdge(v1, v2, color);
        AddEdge(v2, v3, color);
        AddEdge(v3, v0, color);

        AddEdge(v4, v5, color); // Top face
        AddEdge(v5, v6, color);
        AddEdge(v6, v7, color);
        AddEdge(v7, v4, color);

        AddEdge(v0, v4, color); // Vertical edges
        AddEdge(v1, v5, color);
        AddEdge(v2, v6, color);
        AddEdge(v3, v7, color);
    }

    void DrawWireCube1(const glm::vec3& min, const glm::vec3& max, Color4_4 color = Color4_4::White)
    {
        m_LinesVertices.clear();

        // Calculate the 8 corners of the AABB
        PosColVertex corners[8] = {
            // Bottom face
            {{min.x, min.y, min.z}, color}, // (minX, minY, minZ)
            {{max.x, min.y, min.z}, color}, // (maxX, minY, minZ)
            {{min.x, max.y, min.z}, color}, // (minX, maxY, minZ)
            {{max.x, max.y, min.z}, color}, // (maxX, maxY, minZ)
            // Top face
            {{min.x, min.y, max.z}, color}, // (minX, minY, maxZ)
            {{max.x, min.y, max.z}, color}, // (maxX, minY, maxZ)
            {{min.x, max.y, max.z}, color}, // (minX, maxY, maxZ)
            {{max.x, max.y, max.z}, color}, // (maxX, maxY, maxZ)
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

    void DrawWireSphere(float radius, const glm::vec3& center, Color4_4 color=Color4_4::White)
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
                { center + glm::vec3{radius * cos(phi1), radius * sin(phi1), 0}, color },
                { center + glm::vec3{radius * cos(phi2), radius * sin(phi2), 0}, color }
            );

            float rc1 = radius * cos(theta1);
            float rc2 = radius * cos(theta2);
            // create 4 lines
            for (float l = 0; l < 2 * glm::pi<float>(); l += glm::half_pi<float>())
            {
                AddEdge(
                    { center + glm::vec3 {rc1 * cos(l), rc1 * sin(l), radius * sin(theta1) }, color },
                    { center + glm::vec3 {rc2 * cos(l), rc2 * sin(l), radius * sin(theta2) }, color }
                );
            }
        }
    }

    // Function to draw a wired spotlight with direction
    void DrawSpotLight(const glm::vec3& position, const glm::vec3& direction, float innerRadius, float outerRadius, float range, Color4_4 color = Color4_4::White) 
    {
        m_LinesVertices.clear();

        constexpr uint32_t segments = 3;

        glm::vec3 right = normalize(glm::cross(direction, Vec3::Up));
        glm::vec3 up = normalize(glm::cross(right, direction));

        // Base position at the end of the cone (range away from the light position)
        constexpr float coneRange = 5;
        glm::vec3 coneCenter = position + direction * coneRange;
        // Draw the inner and outer circles
        DrawCircle(coneCenter, innerRadius, right, up, 20, color);
        DrawCircle(coneCenter, outerRadius, right, up, 20, color);

        // Add an edge to `range/limit` like blender
        AddEdge({position, color}, { position + direction * range, color});

        for (int i = 0; i < segments; ++i) 
        {
            float angle = (2.0f * glm::pi<float>() * float(i)) / float(segments);
            glm::vec3 directionOnPlane = right * cos(angle) + up * sin(angle);
            glm::vec3 outerPoint = coneCenter + directionOnPlane * outerRadius;

            // Add lines from the position of the spotlight to the inner circle
            AddEdge({ position, color }, { outerPoint, color });
        }
    }

    // Function to draw a directional light gizmo
    void DrawDirectionalLight(const glm::vec3& lightDir, const glm::vec3& position, Color4_4 color = Color4_4::White, float arrowLength = 5.0f) 
    {
        m_LinesVertices.clear();

        glm::vec3 normalizedLightDir = normalize(lightDir);

        // Calculate the end point of the arrow based on the direction and length
        glm::vec3 arrowEnd = position + (normalizedLightDir * arrowLength);

        // Draw the main arrow shaft
        AddEdge({ position, color }, { arrowEnd, color });

        // Draw arrowhead (optional, for better visual)
        DrawArrowHead(arrowEnd, normalizedLightDir, arrowLength * 0.2f, color); // Make the arrowhead 20% of the arrow length
    }

    // Helper function to draw a circle in a specific plane defined by right and up vectors
    void DrawCircle(const glm::vec3& center, float radius, const glm::vec3& right, const glm::vec3& up, int segments, Color4_4 color = Color4_4::White)
    {
        for (int i = 0; i < segments; ++i) 
        {
            float angle1 = (2.0f * glm::pi<float>() * float(i)) / float(segments);
            float angle2 = (2.0f * glm::pi<float>() * float(i + 1)) / float(segments);

            // Create two points on the circle
            glm::vec3 point1 = center + (right * cos(angle1) + up * sin(angle1)) * radius;
            glm::vec3 point2 = center + (right * cos(angle2) + up * sin(angle2)) * radius;

            AddEdge({ point1, color }, { point2, color });
        }
    }

    // Helper function to draw an arrowhead
    void DrawArrowHead(const glm::vec3& tip, const glm::vec3& direction, float size, Color4_4 color = Color4_4::White)
    {
        // Create two perpendicular vectors for the arrowhead
        glm::vec3 right = normalize(glm::cross(direction, glm::vec3(0, 1, 0)));
        if (glm::length(right) < 0.01f) {
            right = normalize(glm::cross(direction, glm::vec3(1, 0, 0))); // Handle edge case where direction is almost vertical
        }
        glm::vec3 up = normalize(glm::cross(right, direction));

        // Calculate the points for the arrowhead
        glm::vec3 arrowLeft = tip - (direction * size) + (right * size * 0.5f);
        glm::vec3 arrowRight = tip - (direction * size) - (right * size * 0.5f);
        glm::vec3 arrowUp = tip - (direction * size) + (up * size * 0.5f);
        glm::vec3 arrowDown = tip - (direction * size) - (up * size * 0.5f);

        // Add lines for the arrowhead
        AddEdge({ tip, color }, {arrowLeft, color });
        AddEdge({ tip, color }, {arrowRight, color });
        AddEdge({ tip, color }, {arrowUp, color });
        AddEdge({ tip, color }, { arrowDown, color });
    }


    // Helper function to add edges
    void AddEdge(const PosColVertex& start, const PosColVertex& end)
    {
        m_LinesVertices.push_back(start);
        m_LinesVertices.push_back(end);
    };

    // Helper function to add edges
    void AddEdge(const glm::vec3& start, const glm::vec3& end, Color4_4 color)
    {
        m_LinesVertices.emplace_back(start, color);
        m_LinesVertices.emplace_back(end, color);
    };

    std::vector<PosColVertex> m_LinesVertices;
};
