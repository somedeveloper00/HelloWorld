#version 330 core

out vec4 FragColor;
in vec3 vertCol;

uniform float t;

void main()
{
    FragColor = t + vec4(vertCol.xyz, 1.0f);
}
