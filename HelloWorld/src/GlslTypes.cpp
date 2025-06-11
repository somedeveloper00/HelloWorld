#pragma once
#include <glm/glm.hpp>

struct alignas(16) glslVec3
{
    float x, y, z, w;

    glslVec3() {}

    glslVec3(glm::vec3 vec) : x(vec.x), y(vec.y), z(vec.z), w(1.f)
    {
    }
};