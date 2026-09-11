#version 330 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;

uniform mat4 uMVP;
uniform mat4 uModel;
uniform float uPointSize;

out vec3 vFragPos;
out vec3 vNormal;

void main() {
    gl_Position = uMVP * vec4(aPos, 1.0);

    // Позиция вершины в мировом пространстве.
    // Нужна для определения направления от поверхности к источнику света.
    vFragPos = vec3(uModel * vec4(aPos, 1.0));

    // Переводим нормаль из локального пространства объекта
    // в мировое пространство.
    //
    // Используется normal matrix, чтобы нормали оставались
    // корректными при масштабировании объекта.
    vNormal = mat3(transpose(inverse(uModel))) * aNormal;

    // vEdgeT = aEdgeT; //VBO
    gl_PointSize = uPointSize;
}