#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec2 fragTexCoord;
layout(location = 1) in vec4 fragColour;

layout(binding = 0) uniform sampler2D textureSampler;

layout(location = 0) out vec4 outColour;

void main()
{
	outColour = texture(textureSampler, fragTexCoord);
}