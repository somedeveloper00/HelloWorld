#pragma once
#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp"
#include "glm/gtc/matrix_transform.hpp"

struct Transform
{
    glm::vec3 position;
    glm::quat rotation;
    glm::vec3 scale;

    Transform(glm::vec3 position = glm::vec3(0), glm::quat rotation = glm::quat(), glm::vec3 scale = glm::vec3(1.f))
        : position(position), rotation(rotation), scale(scale)
    {
    }

    void rotateAround(glm::vec3 axis, float amount)
    {
        rotation = glm::normalize(rotation * glm::angleAxis(amount, axis));
    }

    glm::vec3 getForward() const
    {
        return rotation * glm::vec3(0.f, 0.f, -1.f);
    }

    glm::vec3 getUp() const
    {
        return rotation * glm::vec3(0.f, 1.f, 0.f);
    }

    glm::mat4 getViewMatrix() const
    {
        return glm::lookAt(position, position + getForward(), getUp());
    }

    glm::mat4 getMatrix4() const
    {
        auto t = glm::translate(glm::mat4(1), position);
        auto r = glm::mat4_cast(rotation);
        auto s = glm::scale(glm::mat4(1), scale);
        return t * r * s;
    }
};