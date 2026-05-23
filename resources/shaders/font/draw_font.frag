#version 330 core

uniform sampler2D uTexture;
uniform vec4 uColor;
uniform vec4 uOutlineColor;
uniform float uOutlineThickness;

in vec2 vertexUV;
out vec4 outColor;

void main()
{
	float sdf = texture(uTexture, vertexUV).r;
	float fillEdge = 0.5;
	float outlineEdge = fillEdge - uOutlineThickness;
	float fillAlpha = smoothstep(fillEdge - 0.05, fillEdge + 0.05, sdf);
	float outlineAlpha = uOutlineThickness > 0.0
		? smoothstep(outlineEdge - 0.05, outlineEdge + 0.05, sdf)
		: 0.0;

	vec4 outline = vec4(uOutlineColor.rgb * uOutlineColor.a * outlineAlpha, uOutlineColor.a * outlineAlpha);
	vec4 fill = vec4(uColor.rgb * uColor.a * fillAlpha, uColor.a * fillAlpha);

	vec4 premultiplied = fill + outline * (1.0 - fill.a);
	vec3 color = premultiplied.a > 0.0
		? premultiplied.rgb / premultiplied.a
		: vec3(0.0);

	outColor = vec4(color, premultiplied.a);
}
