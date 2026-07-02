#version 430 core

layout(location = 0) in vec3 inLocalPosition;
layout(location = 1) in vec2 inLocalUV;

layout(std140, binding = 1) uniform CameraData
{
	mat4 uViewProjection;
};

layout(std140, binding = 2) uniform ModelData
{
	mat4 uModel;
};

out vec2 vertexUV;

void main()
{
	gl_Position = uViewProjection * uModel * vec4(inLocalPosition, 1.0);
	vertexUV = inLocalUV;
}
