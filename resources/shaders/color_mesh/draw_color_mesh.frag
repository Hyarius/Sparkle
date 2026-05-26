#version 410 core

layout(location = 0) in vec4 fragColor;

out vec4 outColor;

void main()
{
	outColor = fragColor;
}
