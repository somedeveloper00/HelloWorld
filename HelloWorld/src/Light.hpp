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
    float position_and_cutoff[4];
    float diffuseColor_and_outerCutoff[4];
    float specularColor_and_attenuationConst[4];
    float forward_and_attenuationLinear[4];
    float attenuationQuad;

    inline void map(const Light& light) noexcept
    {
        const float cutoff = std::cos(glm::radians(light.cutoffAngle));
        const float outerCutoff = std::cos(glm::radians(light.outerCutoffAngle));
        const glm::vec3 forward = light.transform.getForward();

        // hint for SMID
        position_and_cutoff[0] = light.transform.position.x;
        position_and_cutoff[1] = light.transform.position.y;
        position_and_cutoff[2] = light.transform.position.z;
        position_and_cutoff[3] = cutoff;
        diffuseColor_and_outerCutoff[0] = light.diffuseColor.x;
        diffuseColor_and_outerCutoff[1] = light.diffuseColor.y;
        diffuseColor_and_outerCutoff[2] = light.diffuseColor.z;
        diffuseColor_and_outerCutoff[3] = outerCutoff;
        specularColor_and_attenuationConst[0] = light.specularColor.x;
        specularColor_and_attenuationConst[1] = light.specularColor.y;
        specularColor_and_attenuationConst[2] = light.specularColor.z;
        specularColor_and_attenuationConst[3] = light.attenuationConst;
        forward_and_attenuationLinear[0] = forward.x;
        forward_and_attenuationLinear[1] = forward.y;
        forward_and_attenuationLinear[2] = forward.z;
        forward_and_attenuationLinear[3] = light.attenuationLinear;
        attenuationQuad = light.attenuationQuad;
    }
};

struct alignas(16) LightBlock
{
    alignas(4) int lightsCount;
    alignas(16) LightBufferData lights[8];
};