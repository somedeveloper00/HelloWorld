#pragma once

#include "Transform.hpp"
#include "GlslTypes.cpp"
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

struct alignas(16) LightBufferData
{
    glslVec3 position;
    glslVec3 diffuseColor;
    glslVec3 specularColor;
    glslVec3 forward;
    float cutoff;
    float outerCutoff;
    float attenuationConst;
    float attenuationLinear;
    float attenuationQuad;

    LightBufferData() {}

    LightBufferData(const Light& light) :
        position(light.transform.position),
        diffuseColor(light.diffuseColor),
        specularColor(light.specularColor),
        forward(light.transform.getForward()),
        cutoff(std::cos(glm::radians(light.cutoffAngle))),
        outerCutoff(std::cos(glm::radians(light.outerCutoffAngle))),
        attenuationConst(light.attenuationConst),
        attenuationLinear(light.attenuationLinear),
        attenuationQuad(light.attenuationQuad)
    {
    }
};

struct alignas(16) LightBlock
{
    alignas(4) int lightsCount;
    alignas(16) LightBufferData lights[8];
};