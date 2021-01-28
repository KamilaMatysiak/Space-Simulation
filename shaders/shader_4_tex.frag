#version 430 core

struct PointLight {
	vec3 position;
	vec3 color;
	float intensity;
};

#define NR_POINT_LIGHTS 5

uniform vec3 objectColor;
uniform vec3 cameraPos;
uniform sampler2D colorTexture;
uniform PointLight pointLights[NR_POINT_LIGHTS];

in vec3 interpNormal;
in vec3 fragPos;
in vec2 vTexCoord;



void main()
{
	vec3 fragColor = vec3(0,0,0);
	vec4 textureColor = texture2D(colorTexture, vTexCoord);
	vec4 ambient = vec4(0.1, 0.1, 0.1, 1.0) * textureColor;
	vec3 normal = normalize(interpNormal);
	for(int i = 0; i < NR_POINT_LIGHTS; i++)
	{
		vec3 lightDir = normalize(pointLights[i].position - fragPos);
	
		vec3 V = normalize(cameraPos-fragPos);
		vec3 R = reflect(-lightDir,normal);

		float dist = distance(fragPos, pointLights[i].position) / 5;
		float distance = (1/dist) * (1/dist);
	
		float spec = pow(max(0,dot(R,V)),2);
		float diff = max(0,dot(normal,normalize(lightDir)));

		vec3 diffuse = pointLights[i].color * diff * distance * pointLights[i].intensity;
		vec3 specular = spec * pointLights[i].color * (pointLights[i].intensity/dist);

		vec3 texture = vec3(textureColor.x, textureColor.y, textureColor.z); // * pointLights[i].color;
		fragColor += mix(texture,texture*diffuse+vec3(1)*specular,0.9);
	}

	gl_FragColor = vec4(fragColor, 1.0) + ambient;
}
