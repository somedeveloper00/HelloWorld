#include "Transform.hpp"
struct Light
{
    Transform transform;
    glm::vec3 diffuseColor;
    glm::vec3 specularColor;

    /// <summary>
    /// angle of light cut-off (usefull for spotlight). value between 0 and 360
    /// </summary>
    float cutOffAngle;

    /// <summary>
    /// angle of light cut-off smooth cone (useful for spotlight). value between 0 and 360
    /// </summary>
    float cutOffSmoothCone;

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

    Light(glm::vec3 pos = glm::vec3(0.f), glm::vec3 diffuseColor = glm::vec3(1.f), glm::vec3 specularColor = glm::vec3(1.f), float angle = 5.f, float cutOffSmoothCone = 30.f, float attenuationConst = .1f, float attenuationLinear = .045f, float attenuationQuad = .0075f)
        : transform(pos), diffuseColor(diffuseColor), specularColor(specularColor), cutOffAngle(angle), cutOffSmoothCone(cutOffSmoothCone), attenuationConst(attenuationConst), attenuationLinear(attenuationLinear), attenuationQuad(attenuationQuad)
    {
    }
};