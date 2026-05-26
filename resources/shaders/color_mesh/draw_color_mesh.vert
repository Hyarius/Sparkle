#version 410 core

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec4 inColor;

layout(std140) uniform ViewportData
{
	mat4 uProjection;
};

layout(location = 0) out vec4 outColor;

void main()
{
	gl_Position = uProjection * vec4(inPosition, 1.0);
	outColor = inColor;
}
