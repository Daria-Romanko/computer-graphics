in vec3 color;
in vec2 TexCoord;
out vec4 FragColor;

uniform sampler2D texture;
uniform float colorMix;

void main()
{
    vec4 gradColor = vec4(color, 1.0);          
    vec4 texColor = texture(texture, TexCoord);
    vec4 tinted = texColor * gradColor;        

    FragColor = mix(gradColor, tinted, colorMix);
}