#version 330 core

in vec2 texCoord;
out vec4 FragColor;

uniform sampler2D tex1;
uniform sampler2D tex2;

void main()
{
    FragColor = texture(tex1, texCoord) + texture(tex2, texCoord);
}
