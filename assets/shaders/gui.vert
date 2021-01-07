#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) in vec4 inColour;

layout(location = 0) out vec2 fragTexCoord;
layout(location = 1) out vec4 fragColour;

void main()
{
	gl_Position = vec4(inPosition, 0.0, 1.0);
    fragTexCoord = inTexCoord;
    fragColour = inColour;
}