#version 430 core

uniform sampler2D uTexture;

in vec2 vertexUV;
out vec4 outColor;

void main()
{
	vec4 color = texture(uTexture, vertexUV);
	if (color.a < 0.5)
	{
		discard;
	}
	outColor = color;
}
