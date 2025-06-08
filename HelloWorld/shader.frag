#version 460 core

#define MAX_LIGHTS 4
#define PI 3.14159265f

in vec2 texCoord;
in vec3 normal;
in vec3 pos;
out vec4 FragColor;

struct Material {
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float shininess;

    sampler2D diffuseTexture;
    sampler2D specularTexture;
};

// multi-purpose light data
struct Light {
    vec3 position;
    vec3 diffuseColor;
    vec3 specularColor;
    vec3 forward;
    float cutoff; // range [-1, 1]
    float smoothCone; // range [0, 2]
    float attenuationConst; // constant attenuation
    float attenuationLinear; // linear attenuation
    float attenuationQuad; // quadratic attenuation
};

uniform Light lights[MAX_LIGHTS];
uniform int lightsCount;
uniform Material material;
uniform vec3 viewPos;

vec3 calculateLight(Light light, vec3 norm, vec3 viewDir, vec3 diffuseCol, vec3 specularCol)
{
    vec3 lightDir = normalize(light.position - pos);

    // light's main influence
    float dist = distance(light.position, pos);
    float lightFragAngle = dot(lightDir, -light.forward);
    float angleFactor = lightFragAngle > light.cutoff 
        ? 1.f // full effect
        : lightFragAngle + light.smoothCone > light.cutoff
            ? smoothstep(1.f, 0.f, abs(lightFragAngle - light.cutoff) / light.smoothCone) // smoothed cosine
            : 0.f;
    float affect = angleFactor / (light.attenuationConst + light.attenuationLinear * dist + light.attenuationQuad * dist * dist);

    // cull lights with too low an affect
    if (affect < 0.0001f)
        return vec3(0.f);

    // diffuse
    float diff = max(dot(norm, lightDir), 0.f);
    vec3 diffuse = (affect * light.diffuseColor) * (diff * diffuseCol * material.diffuse);

    // specular
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.f), material.shininess);
    vec3 specular = (affect * light.specularColor) * (spec * specularCol * material.specular);

    return diffuse + specular;
}

void main()
{
    vec3 diffuseCol = texture(material.diffuseTexture, texCoord).rgb;
    vec3 specularCol = texture(material.specularTexture, texCoord).rgb;

    vec3 ambient = material.ambient;
    vec3 norm = normalize(normal);
    vec3 viewDir = normalize(viewPos - pos);

    vec3 result = vec3(0);
    // FragColor = vec4(lights.length()-3); return;
    for (int i = 0; i < MAX_LIGHTS; i++)
        result += i >= lightsCount ? vec3(0.f) : calculateLight(lights[i], norm, viewDir, diffuseCol, specularCol);

    FragColor = vec4(pow(result, vec3(1.0 / 2.2)), 1.0);
}
