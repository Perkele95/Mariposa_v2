#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 fragPosition;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec4 fragColour;
// Light
layout(location = 3) in vec3 fragLightPosition;
layout(location = 4) in vec3 fragLightDiffuse;
layout(location = 5) in float fragLightConstant;
layout(location = 6) in float fragLightLinear;
layout(location = 7) in float fragLightQuadratic;
layout(location = 8) in float fragLightAmbient;

layout(location = 0) out vec4 outColor;

void main()
{
	vec3 normal = normalize(fragNormal);
	vec3 lightDirection = normalize(fragLightPosition - fragPosition);

	float diff = max(dot(normal, lightDirection), 0.0);
	float _distance = length(fragLightPosition - fragPosition);
	float attenuation = 1.0 / (fragLightConstant + _distance * fragLightLinear + _distance * _distance * fragLightQuadratic);

	vec4 diffuse = diff * vec4(fragLightDiffuse, 1.0) * attenuation;

	outColor = (fragLightAmbient + diffuse) * fragColour;
}