#version 330 core
layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aTexCoord;

uniform mat4 modelMatrix;
uniform vec2 texOffset;
uniform float zoom;

out vec2 TexCoord;
out vec2 fragPos;

void main() {
    vec4 pos = modelMatrix * vec4(aPos, 0.0, 1.0)*zoom;
    gl_Position = vec4(pos.x, pos.y, 0.0, 1.0); // pour passer des [0,1] à [-1,1]
    TexCoord = aTexCoord + texOffset;
    fragPos = aPos*zoom;
}
