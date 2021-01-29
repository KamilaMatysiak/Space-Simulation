#version 330 core
out vec4 FragColor;

in vec2 vTexCoords;

uniform sampler2D scene;
uniform sampler2D bloomBlur;

void main()
{             
    const float gamma = 2.2;
    vec3 hdrColor = texture(scene, vTexCoords).rgb;      
    vec3 bloomColor = texture(bloomBlur, vTexCoords).rgb;
    hdrColor += bloomColor; // additive blending
    // tone mapping
    vec3 result = vec3(1.0) - exp(-hdrColor * 1.0f);
    // also gamma correct while we're at it       
    result = pow(result, vec3(1.0 / gamma));
    FragColor = vec4(result, 1.0);
}