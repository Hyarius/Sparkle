#version 430 core

uniform sampler2D uTexture;
uniform bool uTranslucent;

layout(std140, binding = 2) uniform ModelData
{
	mat4 uModel;
	vec4 uTint;
};

layout(std140, binding = 3) uniform DirectionalLightData
{
	vec4 uLightDirection;
	vec4 uLightColor;
	vec4 uAmbient;
};

in vec3 vertexNormal;
in vec2 vertexUV;
out vec4 outColor;

void main()
{
	vec4 sampled = texture(uTexture, vertexUV);
	float diffuse = max(dot(normalize(vertexNormal), -uLightDirection.xyz), 0.0);
	vec3 lighting = vec3(uAmbient.x) + diffuse * uLightColor.rgb;
	vec3 color = sampled.rgb * uTint.rgb * lighting;
	float alpha = uTranslucent ? sampled.a * uTint.a : 1.0;
	outColor = vec4(color, alpha);
}
