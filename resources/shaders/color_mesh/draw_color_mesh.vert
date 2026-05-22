#version 330 core

layout(location = 0) in vec2 inPosition;

layout(std140) uniform ViewportData
{
	mat4 uProjection;
};

void main()
{
	gl_Position = uProjection * vec4(inPosition, 0.0, 1.0);
}
