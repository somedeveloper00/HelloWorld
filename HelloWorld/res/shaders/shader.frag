#version 460 core

#define MAX_LIGHTS 8
#define PI 3.14159265f

in vec2 texCoord;
in vec3 normal;
in vec3 pos;
out vec4 FragColor;

struct Material {
    vec3 ambient;
    float shininess;

    sampler2D diffuse1;
    sampler2D specular1;
};

// multi-purpose light data
struct Light {
    vec4 position_and_cutoff; // cutoff: cos(inner cone angle)
    vec4 diffuseColor_and_outerCutoff; //outerCutoff: cos(outter cone angle)
    vec4 specularColor_and_attenuationConst; // attenuationConst: constant attenuation
    vec4 forward_and_attenuationLinear; // attenuationLinear: linear attenuation
    float attenuationQuad; // quadratic attenuation
};

layout(std140, binding = 0) uniform LightBlock {
    int lightsCount;
    Light lights[MAX_LIGHTS];
};

uniform Material material;
uniform vec3 viewPos;

vec3 calculateLight(const Light light, const vec3 N, const vec3 V, const vec3 baseDiffuse, const vec3 baseSpecular)
{
    // light vector and distance                                                                                                   
    vec3 L = light.position_and_cutoff.xyz - pos;                                                                                                 
    float dist = length(L);                                                                                                        
    L = dist > 0.f ? L / dist : vec3(0.f); // normalize L 

    // spotlight intensity (affects diffuse and specular)                                                                          
    float theta = dot(L, normalize(-light.forward_and_attenuationLinear.xyz));                                                                               
    float epsilon = light.position_and_cutoff.w - light.diffuseColor_and_outerCutoff.w;                                                                              
    float spotIntensity = clamp((theta - light.diffuseColor_and_outerCutoff.w) / epsilon, 0.0, 1.0);                                                  
    float attenuation = 1.0 / (light.specularColor_and_attenuationConst.w +                                                                            
                               light.forward_and_attenuationLinear.w * dist +                                                                    
                               light.attenuationQuad * dist * dist);                                                               
    float radiance = spotIntensity * attenuation;  
                                                                                                                               
    if (radiance < 1e-4)
        return vec3(0.0);                                                                                                      
                                                                                                                               
    // diffuse term                                                                                                                
    float NdotL = max(0, dot(N, L));                                                                                               
    vec3 diffuse = (radiance * light.diffuseColor_and_outerCutoff.rgb) * (NdotL * baseDiffuse);                                                        
                                                                                                                               
    // specular term (Blinn-Phong)                                                                                                 
    vec3 H = normalize(L + V);                                                                                                     
    float NdotH = max(dot(N, H), 0.0);                                                                                             
    vec3 specular = radiance * light.specularColor_and_attenuationConst.rgb * pow(NdotH, material.shininess) * baseSpecular;                                
                                                                                                                               
    return diffuse + specular;                                                                                                     
}

void main()
{
    // texture samples
    vec3 baseDiffuse = texture(material.diffuse1, texCoord).rgb;
    vec3 baseSpecular = texture(material.specular1, texCoord).rgb;

    // prepare vectors
    vec3 N = normalize(normal);
    vec3 V = normalize(viewPos - pos);

    vec3 ambient = material.ambient * baseDiffuse;

    // accumulate all lights (forward rendering)
    vec3 result = ambient;
    for (int i = 0; i < min(lightsCount, MAX_LIGHTS); i++)
        result += calculateLight(lights[i], N, V, baseDiffuse, baseSpecular);

    // gamma correction
    // result = pow(result, vec3(1.0 / 2.2));
    FragColor = vec4(result, 1.f);
}
