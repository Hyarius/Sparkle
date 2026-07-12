#version 430 core

uniform sampler2D uTexture;
uniform bool uTranslucent;

layout(std140, binding = 2) uniform ModelData
{
	mat4 uModel;
	vec4 uTint;
};

// Binding values mirror spk_scene_gpu_bindings.hpp. Arrays use std430 SSBOs.
layout(std140, binding = 3) uniform LightingHeader
{
	vec4 uAmbientColorAndIntensity;
	uint uDirectionalCount;
	uint uPointCount;
	uint uSpotCount;
	int uShadowDirectionalIndex;
};
struct DirectionalLight { vec4 directionAndIntensity; vec4 color; };
struct PointLight { vec4 positionAndRange; vec4 colorAndIntensity; };
struct SpotLight { vec4 positionAndRange; vec4 directionAndInnerCosine; vec4 colorAndIntensity; vec4 outerCosineAndPadding; };
layout(std430, binding = 4) readonly buffer DirectionalLights { DirectionalLight uDirectionalLights[]; };
layout(std430, binding = 5) readonly buffer PointLights { PointLight uPointLights[]; };
layout(std430, binding = 6) readonly buffer SpotLights { SpotLight uSpotLights[]; };
layout(std140, binding = 7) uniform DirectionalShadowData { mat4 uShadowMatrices[4]; vec4 uCascadeFarDistances; vec4 uShadowBiasDistanceTransition; uvec4 uShadowControl; };
layout(binding = 8) uniform sampler2D uDirectionalShadow0;
layout(binding = 9) uniform sampler2D uDirectionalShadow1;
layout(binding = 10) uniform sampler2D uDirectionalShadow2;
layout(binding = 11) uniform sampler2D uDirectionalShadow3;
layout(std140, binding = 1) uniform CameraData { mat4 uViewProjection; mat4 uView; };

in vec3 vertexNormal;
in vec2 vertexUV;
in float vertexAlpha;
in vec3 vertexWorldPosition;
out vec4 outColor;

float cascadeFarDistance(int index)
{
	if (index == 0) return uCascadeFarDistances.x;
	if (index == 1) return uCascadeFarDistances.y;
	if (index == 2) return uCascadeFarDistances.z;
	return uCascadeFarDistances.w;
}
float shadowDepth(int cascade, vec2 uv)
{
	if (cascade == 0) return texture(uDirectionalShadow0, uv).r;
	if (cascade == 1) return texture(uDirectionalShadow1, uv).r;
	if (cascade == 2) return texture(uDirectionalShadow2, uv).r;
	return texture(uDirectionalShadow3, uv).r;
}
ivec2 shadowSize(int cascade)
{
	if (cascade == 0) return textureSize(uDirectionalShadow0, 0);
	if (cascade == 1) return textureSize(uDirectionalShadow1, 0);
	if (cascade == 2) return textureSize(uDirectionalShadow2, 0);
	return textureSize(uDirectionalShadow3, 0);
}
float sampleShadowCascade(int cascade, vec3 normal, vec3 rayDirection)
{
	vec4 clip = uShadowMatrices[cascade] * vec4(vertexWorldPosition, 1.0);
	if (clip.w <= 0.0) return 1.0;
	vec3 projected = clip.xyz / clip.w * 0.5 + 0.5;
	if (projected.x <= 0.0 || projected.x >= 1.0 || projected.y <= 0.0 || projected.y >= 1.0 || projected.z >= 1.0) return 1.0;
	float bias = max(uShadowBiasDistanceTransition.x, uShadowBiasDistanceTransition.y * (1.0 - max(dot(normal, -normalize(rayDirection)), 0.0)));
	int radius = int(uShadowControl.y);
	vec2 texel = 1.0 / vec2(shadowSize(cascade));
	float visible = 0.0; float samples = 0.0;
	for (int y = -radius; y <= radius; ++y) for (int x = -radius; x <= radius; ++x) { visible += projected.z - bias > shadowDepth(cascade, projected.xy + vec2(x, y) * texel) ? 0.35 : 1.0; samples += 1.0; }
	return visible / samples;
}
float shadowVisibility(vec3 normal, vec3 rayDirection)
{
	int count = int(uShadowControl.x);
	if (uShadowControl.z == 0u || count == 0) return 1.0;
	float viewDepth = max(-(uView * vec4(vertexWorldPosition, 1.0)).z, 0.0);
	if (viewDepth > uShadowBiasDistanceTransition.z) return 1.0;
	int cascade = count - 1;
	for (int i = 0; i < count; ++i) if (viewDepth <= cascadeFarDistance(i)) { cascade = i; break; }
	float current = sampleShadowCascade(cascade, normal, rayDirection);
	if (cascade + 1 >= count || uShadowBiasDistanceTransition.w <= 0.0) return current;
	float previousFar = cascade == 0 ? 0.0 : cascadeFarDistance(cascade - 1);
	float width = (cascadeFarDistance(cascade) - previousFar) * uShadowBiasDistanceTransition.w;
	if (width <= 0.0 || viewDepth < cascadeFarDistance(cascade) - width) return current;
	return mix(current, sampleShadowCascade(cascade + 1, normal, rayDirection), clamp((viewDepth - (cascadeFarDistance(cascade) - width)) / width, 0.0, 1.0));
}

void main()
{
	vec4 sampled = texture(uTexture, vertexUV);
	vec3 normal = normalize(vertexNormal);
	vec3 lighting = uAmbientColorAndIntensity.rgb * uAmbientColorAndIntensity.a;
	for (uint i = 0u; i < uDirectionalCount; ++i) {
		float visibility = int(i) == uShadowDirectionalIndex ? shadowVisibility(normal, uDirectionalLights[i].directionAndIntensity.xyz) : 1.0;
		lighting += max(dot(normal, -normalize(uDirectionalLights[i].directionAndIntensity.xyz)), 0.0) * uDirectionalLights[i].color.rgb * uDirectionalLights[i].directionAndIntensity.w * visibility;
	}
	for (uint i = 0u; i < uPointCount; ++i) {
		vec3 offset = uPointLights[i].positionAndRange.xyz - vertexWorldPosition; float d2 = max(dot(offset, offset), 0.0001); float d = sqrt(d2); float nd = d / uPointLights[i].positionAndRange.w;
		float fade = nd >= 1.0 ? 0.0 : pow(clamp(1.0 - pow(nd, 4.0), 0.0, 1.0), 2.0) / d2;
		lighting += max(dot(normal, offset / d), 0.0) * uPointLights[i].colorAndIntensity.rgb * uPointLights[i].colorAndIntensity.w * fade;
	}
	for (uint i = 0u; i < uSpotCount; ++i) {
		vec3 offset = uSpotLights[i].positionAndRange.xyz - vertexWorldPosition; float d2 = max(dot(offset, offset), 0.0001); float d = sqrt(d2); float nd = d / uSpotLights[i].positionAndRange.w;
		float fade = nd >= 1.0 ? 0.0 : pow(clamp(1.0 - pow(nd, 4.0), 0.0, 1.0), 2.0) / d2;
		float cone = smoothstep(uSpotLights[i].outerCosineAndPadding.x, uSpotLights[i].directionAndInnerCosine.w, dot(normalize(uSpotLights[i].directionAndInnerCosine.xyz), normalize(vertexWorldPosition - uSpotLights[i].positionAndRange.xyz)));
		lighting += max(dot(normal, offset / d), 0.0) * uSpotLights[i].colorAndIntensity.rgb * uSpotLights[i].colorAndIntensity.w * fade * cone;
	}
	vec3 color = sampled.rgb * uTint.rgb * lighting;
	float alpha = uTranslucent ? sampled.a * uTint.a * vertexAlpha : 1.0;
	outColor = vec4(color, alpha);
}
