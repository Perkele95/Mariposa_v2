#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 fragPosition;
layout(location = 1) in vec3 fragColour;
layout(location = 2) in vec3 fragNormal;
layout(location = 3) in vec3 lightPos;
layout(location = 4) in vec3 lightColour;

layout(location = 0) out vec4 outColor;

void main()
{
	vec3 normal = normalize(fragNormal);
	vec3 lightDirection = normalize(lightPos - fragPosition);
	
	float diff = max(dot(normal, lightDirection), 0.0);
	vec3 diffuse = diff * lightColour;
	vec3 ambient = vec3(0.2, 0.2, 0.2);
	
	vec3 result = (ambient + diffuse) * fragColour;
	outColor = vec4(result, 1.0);
}