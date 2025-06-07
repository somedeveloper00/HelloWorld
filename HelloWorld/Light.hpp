#include "Transform.hpp"
struct PointLight
{
    Transform transform;
    glm::vec3 color;

    PointLight(glm::vec3 pos, glm::vec3 color)
        : transform(pos), color(color)
    {
    }
};