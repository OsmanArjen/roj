#version 330 core
layout (location = 0) out vec3 gPosition;
layout (location = 1) out vec3 gNormal;
layout (location = 2) out vec4 gAlbedoSpec;
layout (location = 3) out vec4 gEmissive;

in vec3 TexCoords;

uniform samplerCube skybox;

void main()
{    
        // store the fragment position vector in the first gbuffer texture
    gPosition = vec3(0);
    // also store the per-fragment normals into the gbuffer
    gNormal = vec3(0);
    // and the diffuse per-fragment color
    gAlbedoSpec.rgb = vec3(0);
    // store specular intensity in gAlbedoSpec's alpha component
    gAlbedoSpec.a = 1.0;
    
    gEmissive = vec4(texture(skybox, TexCoords).rgb, 1);
}