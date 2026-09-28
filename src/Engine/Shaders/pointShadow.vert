#version 330 core

layout(location = 0) in vec3 aPos;

uniform mat4 uModel;
uniform mat4 uLightVP;

out vec3 vWorldPosition;

/**
 * @brief Vertex Shader для одной стороны Point Shadow Cubemap.
 *
 * Каждая вершина переводится:
 *
 * Local Space -> World Space -> Light Clip Space.
 *
 * World Position дополнительно передаётся во Fragment Shader,
 * потому что для Point Light глубина определяется расстоянием
 * от поверхности до позиции источника света.
 */
void main() {
    vec4 worldPosition = uModel * vec4(aPos, 1.0);

    vWorldPosition = worldPosition.xyz;
    gl_Position = uLightVP * worldPosition;
}