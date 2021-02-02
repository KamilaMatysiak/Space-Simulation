#version 330 core
out vec4 FragColor;

in vec2 vTexCoords;

uniform sampler2D scene;
uniform sampler2D bloomBlur;

void main()
{             
    const float gamma = 1.0;
    vec3 hdrColor = texture(scene, vTexCoords).rgb;      
    vec3 bloomColor = texture(bloomBlur, vTexCoords).rgb;
    hdrColor += bloomColor;
    vec3 result = vec3(1.0) - exp(-hdrColor * 1.0f);    
    result = pow(result, vec3(1.0 / gamma));
    FragColor = vec4(result, 1.0);
}