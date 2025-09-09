#include "common/typeInfo.hpp"
#include "engine/app.hpp"
#include "engine/components/transform.hpp"
#include "engine/ref.hpp"

// animates transform constantly
struct animateTransform : public engine::component
{
    createTypeInformation(animateTransform, engine::component);

    glm::vec3 position;
    glm::vec3 scale;
    glm::vec3 rotationAxis{0, 0, 1};
    float rotationValue;

    void update_() override
    {
        if (engine::transform *transformRef = getEntity()->getComponent<engine::transform>())
        {
            transformRef->position += position * engine::time::getDeltaTime();
            transformRef->scale += scale * engine::time::getDeltaTime();
            transformRef->rotation = glm::rotate(transformRef->rotation, rotationValue * engine::time::getDeltaTime(), rotationAxis);
            transformRef->markDirty();
        }
    }
};