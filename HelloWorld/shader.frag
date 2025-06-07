#version 330 core

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

struct Light {
    vec3 position;
    vec3 color;
};

uniform Material material;
uniform Light light;
uniform vec3 viewPos;

void main()
{
    float intensity = 50;
    float dist = distance(light.position, pos);
    float affect = dist > intensity ? 0 : (1 - dist / intensity) * (cos((3.14f * dist) / intensity) + 0.5f);

    vec3 ambient = texture(material.diffuseTexture, texCoord).rgb * material.ambient;

    vec3 norm = normalize(normal);
    vec3 lightDir = normalize(light.position - pos);
    float diff = max(dot(norm, lightDir), 0.f);
    vec3 diffuse = (affect * light.color) * (diff * texture(material.diffuseTexture, texCoord).rgb * material.diffuse);

    float specularStrenth = 5.f;
    vec3 viewDir = normalize(viewPos - pos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.f), material.shininess);
    vec3 specular = (affect * light.color) * (spec * texture(material.specularTexture, texCoord).rgb * material.specular);

    FragColor = vec4(ambient + diffuse + specular, 1.f);
}
