#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp> 
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
using namespace std;

GLFWwindow* Window = nullptr;
GLuint Shader_programm = 0;
GLuint Vao = 0;
int nVertices;
unsigned int texture1;
int WIDTH = 800;
int HEIGHT = 600;

float Tempo_entre_frames = 0.0f;

// Variáveis da câmera
float Cam_speed = 5.0f;
glm::vec3 Cam_pos = glm::vec3(0.0f, 0.0f, 2.0f);
glm::vec3 Cam_front = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 Cam_up = glm::vec3(0.0f, 1.0f, 0.0f);

float Cam_yaw = 0.0f; // Iniciar em -90 para olhar direto no eixo Z negativo
float Cam_pitch = 0.0f;

double lastX = WIDTH / 2.0;
double lastY = HEIGHT / 2.0;
bool primeiro_mouse = true;

// Variável para controlar o Field of View (Mecânica de Zoom)
float Cam_fov = 67.0f;


int loadSimpleOBJ(string filePATH, int &nVertices)
{
    std:: vector<glm::vec3> vertices;
    std:: vector<glm::vec2> textCoords;
    std:: vector<glm::vec3> normals; 

    std:: vector<GLfloat> vBuffer;

    std::ifstream dadosEntrada(filePATH.c_str());
    if(!dadosEntrada.is_open()){
        std::cerr << "ERRO: Não foi possivel abrir o arquivo OBJ: " << filePATH << std::endl;
        return -1;
    } 

    std::string linha;
    while(std::getline(dadosEntrada, linha)){
        std::istringstream ssLinha(linha);
        std::string word;
        ssLinha >> word;

        if (word == "v") {
            glm::vec3 vertice;
            ssLinha >> vertice.x >> vertice.y >> vertice.z;
            vertices.push_back(vertice);

        } else if (word == "vt") {
            glm::vec2 vt;
            ssLinha >> vt.x >> vt.y;
            textCoords.push_back(vt);

        } else if (word == "vn") {
            glm::vec3 normal;
            ssLinha >> normal.x >> normal.y >> normal.z;
            normals.push_back(normal);

        } else if (word == "f") {

            while (ssLinha >> word) {
                int vi = 0, ti = 0, ni = 0;

                std::istringstream ss(word);
                std::string index;

                if (std::getline(ss, index, '/')) 
                    vi = !index.empty() ? std::stoi(index) - 1 : -1;
                if (std::getline(ss, index, '/')) 
                    ti = !index.empty() ? std::stoi(index) - 1 : -1;
                if (std::getline(ss, index, '/')) 
                    ni = !index.empty() ? std::stoi(index) - 1 : -1;

             
                vBuffer.push_back(vertices[vi].x);
                vBuffer.push_back(vertices[vi].y);
                vBuffer.push_back(vertices[vi].z);
              
                vBuffer.push_back(textCoords[ti].s);
                vBuffer.push_back(textCoords[ti].t);
                
             
                vBuffer.push_back(normals[ni].x);
                vBuffer.push_back(normals[ni].y);
                vBuffer.push_back(normals[ni].z);
                
            }
        }
    }

    dadosEntrada.close();

    std::cout << "Gerando o Buffer de geometria do OBJ" << std::endl;
    GLuint VBO, VAO;

    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vBuffer.size() * sizeof(GLfloat), vBuffer.data(), GL_STATIC_DRAW);

    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    int stride = 8 * sizeof(GLfloat); 

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, stride, (void*)(5 * sizeof(GLfloat)));
    glEnableVertexAttribArray(2);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    nVertices = vBuffer.size() / 8;

    std::cout << "Buffer gerado com " << nVertices << " vertices." << std::endl;

    return VAO;
}


std::string leShaderDoArquivo(const char* caminhoArquivo) {
    std::ifstream arquivoShader(caminhoArquivo);
    if (!arquivoShader.is_open()) {
        std::cerr << "ERRO: Nao foi possivel abrir o arquivo do shader: " << caminhoArquivo << std::endl;
        return "";
    }
    std::stringstream shaderStream;
    shaderStream << arquivoShader.rdbuf();
    arquivoShader.close();
    return shaderStream.str();
}

void carregarTextura(string filePATH){

    stbi_set_flip_vertically_on_load(true);

    int width, height, nrChannels;
    unsigned char *data = stbi_load(filePATH.c_str(), &width, &height, &nrChannels, 0);

    if (data){

        GLenum format;
        if (nrChannels == 1)
            format = GL_RED;
        else if (nrChannels == 3)
            format = GL_RGB; 
        else if (nrChannels == 4)
            format = GL_RGBA;  
        glGenTextures(1, &texture1);
        glBindTexture(GL_TEXTURE_2D, texture1);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    else{
        std::cout << "Falha ao ler a textura" << std::endl;
    }

    stbi_image_free(data);

  
}

void redimensionaCallback(GLFWwindow* window, int w, int h) {
    WIDTH = w;
    HEIGHT = h;
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    if (primeiro_mouse) {
        lastX = xpos;
        lastY = ypos;
        primeiro_mouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; 
    
    lastX = xpos;
    lastY = ypos;

    float sensibilidade = 0.1f;
    xoffset *= sensibilidade;
    yoffset *= sensibilidade;

    Cam_yaw -= xoffset;
    Cam_pitch += yoffset;

    if (Cam_pitch > 89.0f) Cam_pitch = 89.0f;
    if (Cam_pitch < -89.0f) Cam_pitch = -89.0f;
}

void inicializaOpenGL() {
    if (!glfwInit()) exit(EXIT_FAILURE);

    Window = glfwCreateWindow(WIDTH, HEIGHT, "Câmera e Minimapa", NULL, NULL);
    if (!Window) {
        glfwTerminate();
        exit(EXIT_FAILURE);
    }

    glfwSetWindowSizeCallback(Window, redimensionaCallback);
    glfwSetCursorPosCallback(Window, mouse_callback);
    glfwSetInputMode(Window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwMakeContextCurrent(Window);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) exit(EXIT_FAILURE);
}



void inicializaObjetos() {
    glGenVertexArrays(1, &Vao);
    glBindVertexArray(Vao);

    // VBO dos vértices do cubo
    float points[] = {
        0.5f,  1.5f,  0.5f,  0.5f, -0.5f,  0.5f, -0.5f, -0.5f,  0.5f,
       -0.5f,  0.5f,  0.5f,  0.5f,  0.5f,  0.5f, -0.5f, -0.5f,  0.5f,
        0.5f,  0.5f, -0.5f,  0.5f, -0.5f, -0.5f, -0.5f, -0.5f, -0.5f,
       -0.5f,  0.5f, -0.5f,  0.5f,  0.5f, -0.5f, -0.5f, -0.5f, -0.5f,
       -0.5f, -0.5f,  0.5f, -0.5f,  0.5f,  0.5f, -0.5f, -0.5f, -0.5f,
       -0.5f, -0.5f, -0.5f, -0.5f,  0.5f, -0.5f, -0.5f,  0.5f,  0.5f,
        0.5f, -0.5f,  0.5f,  0.5f,  0.5f,  0.5f,  0.5f, -0.5f, -0.5f,
        0.5f, -0.5f, -0.5f,  0.5f,  0.5f, -0.5f,  0.5f,  0.5f,  0.5f,
       -0.5f, -0.5f,  0.5f,  0.5f, -0.5f,  0.5f,  0.5f, -0.5f, -0.5f,
        0.5f, -0.5f, -0.5f, -0.5f, -0.5f, -0.5f, -0.5f, -0.5f,  0.5f,
       -0.5f,  0.5f,  0.5f,  0.5f,  0.5f,  0.5f,  0.5f,  0.5f, -0.5f,
        0.5f,  0.5f, -0.5f, -0.5f,  0.5f, -0.5f, -0.5f,  0.5f,  0.5f,
    };
    
    GLuint pvbo;
    glGenBuffers(1, &pvbo);
    glBindBuffer(GL_ARRAY_BUFFER, pvbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(points), points, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

    // VBO das cores
    float cores[] = {
        1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f,
        0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f,
        1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f,
        1.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f,
    };
    
    GLuint cvbo;
    glGenBuffers(1, &cvbo);
    glBindBuffer(GL_ARRAY_BUFFER, cvbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cores), cores, GL_STATIC_DRAW);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, (void*)0); 

}

void inicializaShaders() {
    std::string vertexCode = leShaderDoArquivo("../assets/shaders/vertex_shader.glsl");
    std::string fragmentCode = leShaderDoArquivo("../assets/shaders/fragment_shader.glsl");

    const char* vertex_shader = vertexCode.c_str();
    const char* fragment_shader = fragmentCode.c_str();

    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, &vertex_shader, NULL);
    glCompileShader(vs);

    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, &fragment_shader, NULL);
    glCompileShader(fs);

    Shader_programm = glCreateProgram();
    glAttachShader(Shader_programm, vs);
    glAttachShader(Shader_programm, fs);
    glLinkProgram(Shader_programm);

    glDeleteShader(vs);
    glDeleteShader(fs);
}

void atualizaDirecaoCamera() {
    glm::vec3 front;
    front.x = sin(glm::radians(-Cam_yaw)) * cos(glm::radians(Cam_pitch));
    front.y = sin(glm::radians(Cam_pitch)); 
    front.z = -cos(glm::radians(-Cam_yaw)) * cos(glm::radians(Cam_pitch));
    Cam_front = glm::normalize(front);
}

void trataTeclado() {
    if (glfwGetKey(Window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(Window, true);
    }

    // Controle de Zoom: Se a tecla Z estiver pressionada, diminui o FOV da lente
    if (glfwGetKey(Window, GLFW_KEY_Z) == GLFW_PRESS) {
        Cam_fov = 20.0f; // Visão de aproximação
    } else {
        Cam_fov = 67.0f; // Visão normal padrão
    }

    glm::vec3 Cam_right = glm::normalize(glm::cross(Cam_front, Cam_up));

    if (glfwGetKey(Window, GLFW_KEY_A) == GLFW_PRESS)
        Cam_pos -= Cam_right * Cam_speed * Tempo_entre_frames;
    if (glfwGetKey(Window, GLFW_KEY_D) == GLFW_PRESS)
        Cam_pos += Cam_right * Cam_speed * Tempo_entre_frames;
    if (glfwGetKey(Window, GLFW_KEY_W) == GLFW_PRESS)
        Cam_pos += Cam_front * Cam_speed * Tempo_entre_frames;
    if (glfwGetKey(Window, GLFW_KEY_S) == GLFW_PRESS)
        Cam_pos -= Cam_front * Cam_speed * Tempo_entre_frames;
    if (glfwGetKey(Window, GLFW_KEY_E) == GLFW_PRESS)
        Cam_pos.y += Cam_speed * Tempo_entre_frames;
    if (glfwGetKey(Window, GLFW_KEY_Q) == GLFW_PRESS)
        Cam_pos.y -= Cam_speed * Tempo_entre_frames;
}

// Função auxiliar para desenhar o cenário em diferentes views
void desenhaCenario() {
    GLint transformLoc = glGetUniformLocation(Shader_programm, "model");
    glm::mat4 transformacao;

    // 1. CHÃO (Mantém o topo Amarelo)
    transformacao = glm::mat4(1.0f);
    transformacao = glm::translate(transformacao, glm::vec3(0.0f, -2.0f, -10.0f));
    transformacao = glm::scale(transformacao, glm::vec3(30.0f, 0.5f, 30.0f));
    glUniformMatrix4fv(transformLoc, 1, GL_FALSE, glm::value_ptr(transformacao));
    glDrawArrays(GL_TRIANGLES, 0, 36);

    // 2. PAREDE ESQUERDA (Rotacionada 90 graus no eixo Z para o topo ficar Ciano)
    transformacao = glm::mat4(1.0f);
    transformacao = glm::translate(transformacao, glm::vec3(-8.0f, 0.0f, -10.0f));
    transformacao = glm::rotate(transformacao, glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    // Como rotacionamos em Z, a escala local de X vira a altura global (Y)
    transformacao = glm::scale(transformacao, glm::vec3(4.0f, 1.0f, 15.0f)); 
    glUniformMatrix4fv(transformLoc, 1, GL_FALSE, glm::value_ptr(transformacao));
    glDrawArrays(GL_TRIANGLES, 0, 36);

    // 3. PILAR DIREITO (Rotacionado -90 graus no eixo X para o topo ficar Vermelho)
    transformacao = glm::mat4(1.0f);
    transformacao = glm::translate(transformacao, glm::vec3(5.0f, 0.0f, -5.0f));
    transformacao = glm::rotate(transformacao, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    // Como rotacionamos em X, a escala local de Z vira a altura global (Y)
    transformacao = glm::scale(transformacao, glm::vec3(2.0f, 2.0f, 5.0f));
    glUniformMatrix4fv(transformLoc, 1, GL_FALSE, glm::value_ptr(transformacao));
    glDrawArrays(GL_TRIANGLES, 0, 36);

    // 4. ITEM EM DESTAQUE (Cubo girando no centro do cenário)
    transformacao = glm::mat4(1.0f);
    transformacao = glm::translate(transformacao, glm::vec3(0.0f, 0.0f, -8.0f));
    // Aplica a rotação contínua baseada no tempo
    transformacao = glm::rotate(transformacao, (float)glfwGetTime(), glm::vec3(0.5f, 1.0f, 0.0f));
    glUniformMatrix4fv(transformLoc, 1, GL_FALSE, glm::value_ptr(transformacao));
    glDrawArrays(GL_TRIANGLES, 0, 36);
}

void inicializaRenderizacao() {
    double tempo_anterior = glfwGetTime();

    glEnable(GL_DEPTH_TEST);
    
    while (!glfwWindowShouldClose(Window)) {
        double tempo_frame_atual = glfwGetTime();
        Tempo_entre_frames = (float)(tempo_frame_atual - tempo_anterior);
        tempo_anterior = tempo_frame_atual;

        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        glUseProgram(Shader_programm);
        glBindVertexArray(Vao);
        
        trataTeclado();
        atualizaDirecaoCamera();

        // -------------------------------------------------------------
        // ETAPA 1: VISÃO PRINCIPAL EM PERSPECTIVA (TELA CHEIA)
        // -------------------------------------------------------------
        
        // Define que a viewport ocupará toda a janela
        glViewport(0, 0, WIDTH, HEIGHT);

        // Matriz de Visualização usando a posição controlada pelo usuário
        glm::mat4 viewPerspectiva = glm::lookAt(Cam_pos, Cam_pos + Cam_front, Cam_up);
        GLint viewLoc = glGetUniformLocation(Shader_programm, "view");
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(viewPerspectiva));

        // Matriz de Projeção utilizando a variável Cam_fov dinamicamente para o Zoom
        float aspecto = (float)WIDTH / (float)HEIGHT;
        glm::mat4 projPerspectiva = glm::perspective(glm::radians(Cam_fov), aspecto, 0.1f, 100.0f);
        GLint projLoc = glGetUniformLocation(Shader_programm, "proj");
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projPerspectiva));

        desenhaCenario();

        // -------------------------------------------------------------
        // ETAPA 2: MINIMAPA ORTOGRÁFICO (PICTURE-IN-PICTURE)
        // -------------------------------------------------------------

        // Antes de desenhar novamente, limpamos apenas o Z-Buffer 
        // para que os objetos do minimapa não se misturem com a tela principal
        glClear(GL_DEPTH_BUFFER_BIT);

        // Define a nova viewport no canto superior direito
        int mapa_size = 200; // Tamanho do quadrado do minimapa
        glViewport(WIDTH - mapa_size, HEIGHT - mapa_size, mapa_size, mapa_size);

        // Recalcula a Matriz de Visualização
        // Colocamos a câmera bem alto no eixo Y, olhando para o centro do mapa (0, 0, 0)
        // O vetor 'up' agora é o eixo Z, para que a tela não gire incorretamente ao olhar para baixo
        glm::vec3 mapa_pos = glm::vec3(0.0f, 30.0f, 0.0f);
        glm::vec3 mapa_front = glm::vec3(0.0f, -1.0f, 0.0f);
        glm::vec3 mapa_up = glm::vec3(0.0f, 0.0f, -1.0f); 
        
        glm::mat4 viewOrtografica = glm::lookAt(mapa_pos, mapa_pos + mapa_front, mapa_up);
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(viewOrtografica));

        // Recalcula a Matriz de Projeção para Ortográfica (sem ponto de fuga)
        float limite = 15.0f; // Define a "largura" e "altura" do que será capturado pela câmera
        glm::mat4 projOrtografica = glm::ortho(-limite, limite, -limite, limite, 0.1f, 100.0f);
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projOrtografica));

        desenhaCenario();

        glfwPollEvents();
        glfwSwapBuffers(Window);
    }
    
    glfwTerminate();
}

int main() {
    inicializaOpenGL();
    inicializaObjetos();
    inicializaShaders();
    inicializaRenderizacao();
    return 0;
}