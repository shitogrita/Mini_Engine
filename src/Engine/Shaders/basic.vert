#version 330 core

layout(location = 0) in vec3 aPos;
uniform mat4 uMVP;
uniform float uPointSize; // размер вершин


void main() {
  gl_Position = uMVP * vec4(aPos, 1.0);
  vEdgeT = aEdgeT; //VBO
  gl_PointSize = uPointSize;
}

