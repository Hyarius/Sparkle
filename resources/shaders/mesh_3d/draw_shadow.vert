#version 430 core
layout(location = 0) in vec3 inLocalPosition;
layout(std140, binding = 2) uniform ModelData { mat4 uModel; vec4 uTint; };
layout(std140, binding = 7) uniform ShadowData { mat4 uLightViewProjection; };
void main() { gl_Position = uLightViewProjection * uModel * vec4(inLocalPosition, 1.0); }
