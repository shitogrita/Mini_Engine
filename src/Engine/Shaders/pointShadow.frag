#version 330 core

in vec3 vWorldPosition;

uniform vec3 uLightPosition;
uniform float uFarPlane;

/**
 * @brief Fragment Shader Point Shadow Map.
 *
 * Depth Cubemap должен хранить не стандартную перспективную
 * глубину OpenGL, а линейное расстояние:
 *
 * Point Light -> Fragment
 *
 * Расстояние нормализуется через uFarPlane,
 * поэтому в texture сохраняется значение [0, 1].
 */
void main() {
    float lightDistance = length(vWorldPosition - uLightPosition);

    lightDistance /= uFarPlane;

    gl_FragDepth = lightDistance;
}