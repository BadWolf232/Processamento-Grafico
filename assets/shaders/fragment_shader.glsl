#version 330 core
in vec3 ourColor; // Recebe a cor interpolada do Vertex Shader
out vec4 FragColor;

void main() {
    FragColor = vec4(ourColor, 1.0);
}