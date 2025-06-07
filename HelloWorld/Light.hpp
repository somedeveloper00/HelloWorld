#include "Transform.hpp"
struct Light
{
    Transform transform;
    glm::vec3 color;

    Light(glm::vec3 pos, glm::vec3 color)
        : transform(pos), color(color)
    {
    }
};