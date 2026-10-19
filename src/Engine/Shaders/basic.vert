#version 330 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;

/*
 * Один mat4 занимает четыре attribute locations:
 *
 * 4
 * 5
 * 6
 * 7
 */
layout(location = 4) in mat4 aInstanceModel;

uniform mat4 uMVP;
uniform mat4 uModel;

uniform mat4 uViewProjection;
uniform int uUseInstancing;

uniform float uPointSize;
uniform int uLightingEnabled;

out vec3 vFragPos;
out vec3 vNormal;
out vec2 vTexCoord;

void main() {
    mat4 model;

    if (uUseInstancing == 1) {
        model = aInstanceModel;
        gl_Position = uViewProjection * model * vec4(aPos, 1.0);
    } else {
        model = uModel;
        gl_Position = uMVP * vec4(aPos, 1.0);
    }

    if (uLightingEnabled == 1) {
        vFragPos = vec3(
            model * vec4(aPos, 1.0)
        );

        vNormal =
            mat3(
                transpose(
                    inverse(model)
                )
            ) *
            aNormal;
    } else {
        vFragPos = vec3(0.0);
        vNormal = vec3(0.0, 1.0, 0.0);
    }

    vTexCoord = aTexCoord;

    gl_PointSize = uPointSize;
}