#version 330 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;

uniform mat4 uMVP;
uniform mat4 uModel;
uniform float uPointSize;
uniform int uLightingEnabled;

out vec3 vFragPos;
out vec3 vNormal;
out vec2 vTexCoord;

void main() {
    gl_Position = uMVP * vec4(aPos, 1.0);

    if (uLightingEnabled == 1) {
        vFragPos = vec3(uModel * vec4(aPos, 1.0));
        vNormal = mat3(transpose(inverse(uModel))) * aNormal;
    } else {
        vFragPos = vec3(0.0);
        vNormal = vec3(0.0, 1.0, 0.0);
    }
    vTexCoord = aTexCoord;

    // vEdgeT = aEdgeT; //VBO
    gl_PointSize = uPointSize;
}