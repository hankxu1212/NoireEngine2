#pragma once

#include "renderer/AABB.hpp"
#include "renderer/vertices/PosColVertex.hpp"

struct GizmosInstance 
{
    void DrawLineCubeAroundAABB(const AABB& aabb)
    {
        m_LinesVertices.clear();

        // Calculate the 8 corners of the AABB
        PosColVertex corners[8] = {
            // Bottom face
            {{aabb.min.x, aabb.min.y, aabb.min.z}, { 0,255,0,255 }}, // (minX, minY, minZ)
            {{aabb.max.x, aabb.min.y, aabb.min.z}, { 0,255,0,255 }}, // (maxX, minY, minZ)
            {{aabb.min.x, aabb.max.y, aabb.min.z}, { 0,255,0,255 }}, // (minX, maxY, minZ)
            {{aabb.max.x, aabb.max.y, aabb.min.z}, { 0,255,0,255 }}, // (maxX, maxY, minZ)
            // Top face
            {{aabb.min.x, aabb.min.y, aabb.max.z}, { 0,255,0,255 }}, // (minX, minY, maxZ)
            {{aabb.max.x, aabb.min.y, aabb.max.z}, { 0,255,0,255 }}, // (maxX, minY, maxZ)
            {{aabb.min.x, aabb.max.y, aabb.max.z}, { 0,255,0,255 }}, // (minX, maxY, maxZ)
            {{aabb.max.x, aabb.max.y, aabb.max.z}, { 0,255,0,255 }}, // (maxX, maxY, maxZ)
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

    // Helper function to add edges
    void AddEdge(const PosColVertex& start, const PosColVertex& end)
    {
        m_LinesVertices.push_back(start);
        m_LinesVertices.push_back(end);
    };

    std::vector<PosColVertex> m_LinesVertices;
};
