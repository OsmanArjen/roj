#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D gAlbedoSpec;
uniform sampler2D gEmissive;
uniform sampler2D gLightmap;

void main()
{             
    vec3 Diffuse = texture(gAlbedoSpec, TexCoords).rgb;
    vec3 Emissive = texture(gEmissive, TexCoords).rgb;
    if(Diffuse == vec3(0))
    {
        FragColor = vec4(Emissive, 1.0);
        return;
    }

    vec3 Light = texture(gLightmap, TexCoords).rgb;        
    
    FragColor = vec4(Diffuse * max(Light, vec3(0.1)) + Emissive, 1.0);
}