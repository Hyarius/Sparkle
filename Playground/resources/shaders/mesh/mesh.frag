#version 430 core

uniform sampler2D uTexture;

in vec2 vertexUV;
out vec4 outColor;

void main()
{
	// Step 1 is unlit: sample the texture and force full opacity so the cube is
	// always visible regardless of the source texture's alpha. Lighting (needs
	// per-vertex normals) arrives with the voxel mesh in Step 2.
	vec3 color = texture(uTexture, vertexUV).rgb;
	outColor = vec4(color, 1.0);
}
