#version 330 core 

in vec2 TextCoord; 
out vec4 FragColor;

uniform sampler2D textura1; 

void main(){
    FragColor = texture(textura1, TextCoord);
}

