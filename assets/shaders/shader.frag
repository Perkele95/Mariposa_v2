#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 fragPosition;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec4 fragColour;
layout(location = 3) in vec3 fragGlobalLightPosition;
layout(location = 4) in vec3 fragGlobalLightColour;
layout(location = 5) in float fragAmbient;

layout(location = 0) out vec4 outColor;

void main()
{
	vec3 normal = normalize(fragNormal);
	vec3 lightDirection = normalize(fragGlobalLightPosition - fragPosition);

	float diff = max(dot(normal, lightDirection), 0.0);
	float _distance = length(fragGlobalLightPosition - fragPosition);
	float attenuation = 1.0 / (1.0 + _distance * 0.07 + _distance * _distance * 0.017);

	vec4 diffuse = diff * vec4(fragGlobalLightColour, 1.0) * attenuation;

	outColor = (fragAmbient + diffuse) * fragColour;
}