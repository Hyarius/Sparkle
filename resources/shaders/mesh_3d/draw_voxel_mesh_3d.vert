#version 430 core

layout(location = 0) in vec3 inLocalPosition;
layout(location = 1) in vec3 inLocalNormal;
layout(location = 2) in vec2 inLocalUV;
layout(location = 3) in float inAlpha;

layout(std140, binding = 1) uniform CameraData
{
	mat4 uViewProjection;
};

layout(std140, binding = 2) uniform ModelData
{
	mat4 uModel;
	vec4 uTint;
};

out vec3 vertexNormal;
out vec2 vertexUV;
out float vertexAlpha;

void main()
{
	gl_Position = uViewProjection * uModel * vec4(inLocalPosition, 1.0);
	vertexNormal = normalize(transpose(inverse(mat3(uModel))) * inLocalNormal);
	vertexUV = inLocalUV;
	vertexAlpha = inAlpha;
}
