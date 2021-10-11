#version 450

layout(location = 0) in vec3 fragPos;
layout(location = 1) in vec3 fragColor;
layout(location = 2) in vec3 fragNormal;

layout(location = 0) out vec4 outColor;

const vec3 lightPos = vec3(0.0, 0.0, 5.0);
const vec3 diffColor = vec3(1.0, 0.5, 0.0);
const vec3 specColor = vec3(1.0, 1.0, 1.0);

void main() {
	vec3 normal = normalize(fragNormal);
	vec3 viewDir = normalize(-fragPos);
	//if (dot(normal, viewDir) < 0.0)
		//normal *= -1.0;

	vec3 lightDir = normalize(lightPos - fragPos);
	float lamb = max(dot(lightDir, normal), 0.0);
	float spec = 0.0;

	if (lamb > 0.0) {
		vec3 refDir = reflect(-lightDir, normal);

		float specAngle = max(dot(refDir, viewDir), 0.0);
		spec = pow(specAngle, 4.0);
	}
	outColor = vec4(lamb * diffColor + spec * specColor, 1.0);
}