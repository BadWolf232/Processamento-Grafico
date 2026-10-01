#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor; // Recebe a cor do VBO

out vec3 ourColor; // Repassa a cor para o Fragment Shader

uniform mat4 model;
uniform mat4 view;
uniform mat4 proj;

void main() {
    gl_Position = proj * view * model * vec4(aPos, 1.0);
    ourColor = aColor; 
}