#version 430 core

layout(location = 0) in vec3 inLocalPosition;
layout(location = 1) in vec2 inLocalUV;

struct SpriteData
{
	vec2 anchor;
	vec2 size;
};

struct InstanceData
{
	mat4 modelTransform;
	SpriteData sprite;
};

layout(std430, binding = 0) readonly buffer InstanceBuffer
{
	InstanceData instances[];
};

layout(std140, binding = 1) uniform CameraData
{
	mat4 uViewProjection;
};

out vec2 vertexUV;

void main()
{
	InstanceData data = instances[gl_InstanceID];

	gl_Position = uViewProjection * data.modelTransform * vec4(inLocalPosition, 1.0);

	vertexUV = data.sprite.anchor + inLocalUV * data.sprite.size;
}
