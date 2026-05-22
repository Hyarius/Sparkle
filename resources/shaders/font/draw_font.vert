#version 330 core

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inUV;

layout(std140) uniform ViewportData
{
	mat4 uProjection;
};

out vec2 vertexUV;

void main()
{
	vertexUV = inUV;
	gl_Position = uProjection * vec4(inPosition, 1.0);
}
