#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec2 fragTexCoord;
layout(location = 1) in vec4 fragColour;

layout(binding = 0) uniform sampler textureSampler;
layout(binding = 1) uniform texture2D textureArray[8];

layout(push_constant) uniform PER_OBJECT
{
	int imageIndex;
}pushC;

layout(location = 0) out vec4 outColour;

void main()
{
	outColour = texture(sampler2D(textureArray[pushC.imageIndex], textureSampler), fragTexCoord);
}