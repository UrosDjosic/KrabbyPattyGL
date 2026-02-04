//patty.frag
#version 330 core

out vec4 FragColor;
in vec2 TexCoord;

uniform sampler2D uTex;
uniform float cookedness; // 0.0 = sirova, 1.0 = gotova

void main()
{
    vec4 texColor = texture(uTex, TexCoord);
    
    vec3 cookedColor = mix(vec3(1.0, 0.6, 0.6), vec3(0.45, 0.22, 0.05), cookedness);
    
    FragColor = vec4(texColor.rgb * cookedColor, texColor.a);

}
