#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 0) uniform UniformbufferObject
{
	mat4 Model;
	mat4 View;
	mat4 Proj;

	vec3 position;
	vec3 diffuse;
	float constant;
	float linear;
	float quadratic;
	float ambient;
} ubo;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec4 inColour;

layout(location = 0) out vec3 fragPosition;
layout(location = 1) out vec3 fragNormal;
layout(location = 2) out vec4 fragColour;
// Light
layout(location = 3) out vec3 fragLightPosition;
layout(location = 4) out vec3 fragLightDiffuse;
layout(location = 5) out float fragLightConstant;
layout(location = 6) out float fragLightLinear;
layout(location = 7) out float fragLightQuadratic;
layout(location = 8) out float fragLightAmbient;

void main()
{
	gl_Position = ubo.Proj * ubo.View * ubo.Model * vec4(inPosition, 1.0);
	fragPosition = inPosition;
	fragNormal = inNormal;
	fragColour = inColour;

	fragLightPosition = ubo.position;
	fragLightDiffuse = ubo.diffuse;
	fragLightConstant = ubo.constant;
	fragLightLinear = ubo.linear;
	fragLightQuadratic = ubo.quadratic;
	fragLightAmbient = ubo.ambient;
}