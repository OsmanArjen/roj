#version 330 core
layout (location = 0) out vec3 gPosition;
layout (location = 1) out vec3 gNormal;
layout (location = 2) out vec4 gAlbedoSpec;
layout (location = 3) out vec4 gEmissive;

in vec2 TexCoords;
in vec3 FragPos;
in vec3 Normal;

uniform sampler2D texture_diffuse1;
uniform sampler2D texture_specular1;
uniform vec3 viewPos;
void main()
{    
    // store the fragment position vector in the first gbuffer texture
    gPosition = FragPos - viewPos;
    // also store the per-fragment normals into the gbuffer
    gNormal = (normalize(Normal) + 1) / 2;
    // and the diffuse per-fragment color
    gAlbedoSpec.rgb = texture(texture_diffuse1, TexCoords).rgb;
    // store specular intensity in gAlbedoSpec's alpha component
    gAlbedoSpec.a = 1.0;

    gEmissive = vec4(vec3(0), 1);
}