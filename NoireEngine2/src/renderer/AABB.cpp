#include "AABB.hpp"

void AABB::Update(const glm::mat4& model)
{
    auto& T = model[3];
    min = T;
    max = T;
    for (int i = 0; i < 3; ++i)
    {
        for (int j = 0; j < 3; ++j) {
            auto a = model[i][j] * originMin[j];
            auto b = model[i][j] * originMax[j];
            min[i] += a < b ? a : b;
            max[i] += a < b ? b : a;
        }
    }
}