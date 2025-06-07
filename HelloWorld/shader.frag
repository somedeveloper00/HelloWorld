#version 330 core

in vec2 texCoord;
in vec3 normal;
in vec3 pos;
out vec4 FragColor;

// uniform sampler2D tex1;
// uniform sampler2D tex2;
uniform vec4 objectColor;
uniform vec3 lightColor;
uniform vec3 lightPos;
uniform vec3 viewPos;

void main()
{
    float ambientStrenth = .1f;
    vec3 ambient = lightColor * ambientStrenth;

    vec3 norm = normalize(normal);
    vec3 lightDir = normalize(lightPos - pos);
    float diff = max(dot(norm, lightDir), 0.f);
    vec3 diffuse = diff * lightColor;

    float specularStrenth = 5.f;
    vec3 viewDir = normalize(viewPos - pos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.f), 32);
    vec3 specular = spec * specularStrenth * lightColor;

    FragColor = vec4(ambient + diffuse + specular, 1.f) * objectColor;
}
