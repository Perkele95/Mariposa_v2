#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 0) uniform UniformbufferObject
{
	mat4 Model;
	mat4 View;
	mat4 Proj;
} ubo;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec4 inColour;

layout(location = 0) out vec3 fragPosition;
layout(location = 1) out vec3 fragNormal;
layout(location = 2) out vec4 fragColour;
layout(location = 3) out vec3 lightPos;
layout(location = 4) out vec3 lightColour;

void main()
{
	gl_Position = ubo.Proj * ubo.View * ubo.Model * vec4(inPosition, 1.0);
	fragPosition = inPosition;
	fragNormal = inNormal;
	fragColour = inColour;
	lightPos = vec3(0.0, 0.0, 20.0);
	lightColour = vec3(1.0, 1.0, 1.0);
}