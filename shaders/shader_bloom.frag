#version 430 core
layout (location = 0) out vec4 fragColor;
layout (location = 1) out vec4 BrightColor;

in vec3 fragPos;
in vec3 interpNormal;
in vec2 vTexCoords;

struct PointLight {
    vec3 position;
    vec3 color;
};

#define NR_POINT_LIGHTS 2

uniform vec3 objectColor;
uniform vec3 lightPos;
uniform vec3 cameraPos;
uniform sampler2D colorTexture;
uniform PointLight pointLights[NR_POINT_LIGHTS];

void main()
{           
	vec3 result = vec3(0,0,0);
	vec4 textureColor = texture2D(colorTexture, -vTexCoords);
	vec4 ambient = vec4(0.1, 0.1, 0.1, 1.0) * textureColor;
	vec3 normal = normalize(interpNormal);
	for(int i = 0; i < NR_POINT_LIGHTS; i++)
	{
		vec3 lightDir = normalize(pointLights[i].position - fragPos);
	
		vec3 V = normalize(cameraPos-fragPos);
		vec3 R = reflect(-normalize(lightDir),normal);
	
		float specular = pow(max(0,dot(R,V)),10);
		float diffuse = max(0,dot(normal,normalize(lightDir)));
		vec3 texture = vec3(textureColor.x, textureColor.y, textureColor.z) * pointLights[i].color;
		result += mix(texture,texture*diffuse+vec3(1)*specular,0.9);
	}

	result = (fragColor + ambient).xyz;
    
	// check whether result is higher than some threshold, if so, output as bloom threshold color
    float brightness = dot(result, vec3(0.2126, 0.7152, 0.0722));
    if(brightness > 1.0)
        BrightColor = vec4(result, 1.0);
    else
        BrightColor = vec4(0.0, 0.0, 0.0, 1.0);
    fragColor = vec4(result, 1.0);
}