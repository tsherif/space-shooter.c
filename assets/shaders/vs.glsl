#version 300 es

layout (location=0) in vec2 vertexPosition;
layout (location=1) in vec2 pixelOffset;
layout (location=2) in vec2 panelIndex;
layout (location=3) in float scale;
layout (location=4) in float alpha;
layout (location=5) in float whiteOut;

uniform vec2 panelPixelSize;
uniform vec2 spriteSheetDimensions;
uniform vec2 pixelClipSize;

out vec2 vUV;
out float vAlpha;
out float vWhiteOut;

void main() {
    vec2 uv = vertexPosition;
    vUV = (uv + panelIndex) / spriteSheetDimensions;
    vWhiteOut = whiteOut;
    vAlpha = alpha;
    vec2 clipOffset = pixelOffset * pixelClipSize - 1.0;
    gl_Position = vec4((vertexPosition * panelPixelSize * pixelClipSize * scale + clipOffset) * vec2(1.0, -1.0), 0.0, 1.0);
}
