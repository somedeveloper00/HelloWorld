#include "Transform.hpp"
struct Light
{
    Transform transform;
    glm::vec3 diffuseColor;
    glm::vec3 specularColor;

    /// <summary>
    /// angle of light cut-off (usefull for spotlight). value between 0 and 360
    /// </summary>
    float cutoffAngle;

    /// <summary>
    /// angle of light cut-off smooth cone (useful for spotlight). value between 0 and 360
    /// </summary>
    float outerCutoffAngle;

    /// <summary>
    /// constant attenuation
    /// </summary>
    float attenuationConst;

    /// <summary>
    /// linear attenuation
    /// </summary>
    float attenuationLinear;

    /// <summary>
    /// quadratic attenuation
    /// </summary>
    float attenuationQuad;

    Light(glm::vec3 pos = glm::vec3(0.f), glm::vec3 diffuseColor = glm::vec3(1.f), glm::vec3 specularColor = glm::vec3(1.f), float cutOffAngle = 10.f, float cutOffSmoothCone = 20.f, float attenuationConst = .05f, float attenuationLinear = .045f, float attenuationQuad = .0075f)
        : transform(pos), diffuseColor(diffuseColor), specularColor(specularColor), cutoffAngle(cutOffAngle), outerCutoffAngle(cutOffSmoothCone), attenuationConst(attenuationConst), attenuationLinear(attenuationLinear), attenuationQuad(attenuationQuad)
    {
    }
};