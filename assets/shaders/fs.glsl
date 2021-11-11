#version 450

in vec2 vUV;
in float vAlpha;
in float vWhiteOut;

uniform sampler2D spriteSheet;

out vec4 fragColor;

void main() {
    fragColor = texture(spriteSheet, vUV);
    if (vWhiteOut > 0.0) fragColor.rgb = vec3(1.0);
    fragColor.a *= vAlpha;
    fragColor.rgb *= fragColor.a;
}
