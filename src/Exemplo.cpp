#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <vector>
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
GLuint Shader_color = 0;
GLuint Shader_texture = 0;
int nVertices;

struct TipoModelo {
    GLuint Vao;
    int nVertices;
    GLuint Textura;
    bool temTextura;
};

struct InstanciaModelo {
    int tipoIndex;
    glm::vec3 posicao;
    glm::vec3 escala;
    float rotacaoY;
};

std::vector<TipoModelo> tiposModelos;
std::vector<InstanciaModelo> instancias;
GLuint texColormap;


int WIDTH = 800;
int HEIGHT = 600;

float Tempo_entre_frames = 0.0f;


float Cam_speed = 5.0f;
glm::vec3 Cam_pos = glm::vec3(0.0f, 0.0f, 2.0f);
glm::vec3 Cam_front = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 Cam_up = glm::vec3(0.0f, 1.0f, 0.0f);

float Cam_yaw = 0.0f;
float Cam_pitch = 0.0f;

double lastX = WIDTH / 2.0;
double lastY = HEIGHT / 2.0;
bool primeiro_mouse = true;

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

    auto resolveIndex = [](int index, std::size_t size) -> int {
        if (index > 0) {
            index -= 1;
        } else if (index < 0) {
            index = static_cast<int>(size) + index;
        } else {
            return -1;
        }

        return index >= 0 && index < static_cast<int>(size) ? index : -1;
    };

    struct FaceVertex {
        int vertex = -1;
        int texCoord = -1;
        int normal = -1;
    };

    std::string linha;
    int numeroLinha = 0;
    while(std::getline(dadosEntrada, linha)){
        ++numeroLinha;
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
            std::vector<FaceVertex> face;

            while (ssLinha >> word) {
                std::istringstream ss(word);
                std::string index;
                FaceVertex faceVertex;

                try {
                    if (std::getline(ss, index, '/') && !index.empty()) {
                        faceVertex.vertex = resolveIndex(
                            std::stoi(index), vertices.size()
                        );
                    }
                    if (std::getline(ss, index, '/') && !index.empty()) {
                        faceVertex.texCoord = resolveIndex(
                            std::stoi(index), textCoords.size()
                        );
                    }
                    if (std::getline(ss, index, '/') && !index.empty()) {
                        faceVertex.normal = resolveIndex(
                            std::stoi(index), normals.size()
                        );
                    }
                } catch (const std::exception&) {
                    std::cerr << "ERRO: indice invalido no OBJ " << filePATH
                              << " (linha " << numeroLinha << ")" << std::endl;
                    return -1;
                }

                if (faceVertex.vertex < 0) {
                    std::cerr << "ERRO: vertice invalido no OBJ " << filePATH
                              << " (linha " << numeroLinha << ")" << std::endl;
                    return -1;
                }

                face.push_back(faceVertex);
            }

            if (face.size() < 3) {
                std::cerr << "ERRO: face sem vertices suficientes no OBJ "
                          << filePATH << " (linha " << numeroLinha << ")"
                          << std::endl;
                return -1;
            }

            // Converte faces com quatro ou mais vertices em triangulos.
            for (std::size_t i = 1; i + 1 < face.size(); ++i) {
                const FaceVertex triangulo[] = { face[0], face[i], face[i + 1] };

                for (const FaceVertex& faceVertex : triangulo) {
                    const glm::vec3& vertice = vertices[faceVertex.vertex];
                    vBuffer.push_back(vertice.x);
                    vBuffer.push_back(vertice.y);
                    vBuffer.push_back(vertice.z);

                    if (faceVertex.texCoord >= 0) {
                        const glm::vec2& textura = textCoords[faceVertex.texCoord];
                        vBuffer.push_back(textura.s);
                        vBuffer.push_back(textura.t);
                    } else {
                        vBuffer.push_back(0.0f);
                        vBuffer.push_back(0.0f);
                    }

                    if (faceVertex.normal >= 0) {
                        const glm::vec3& normal = normals[faceVertex.normal];
                        vBuffer.push_back(normal.x);
                        vBuffer.push_back(normal.y);
                        vBuffer.push_back(normal.z);
                    } else {
                        vBuffer.push_back(0.0f);
                        vBuffer.push_back(0.0f);
                        vBuffer.push_back(0.0f);
                    }
                }
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

    if (nVertices == 0) {
        std::cerr << "ERRO: o OBJ nao possui faces validas: " << filePATH
                  << std::endl;
        return -1;
    }

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

GLuint carregarTextura(string filePATH){

    stbi_set_flip_vertically_on_load(true);

    int width, height, nrChannels;
    unsigned char *data = stbi_load(filePATH.c_str(), &width, &height, &nrChannels, 0);
    GLuint textura = 0;

    if (data){

        GLenum format;
        if (nrChannels == 1)
            format = GL_RED;
        else if (nrChannels == 3)
            format = GL_RGB; 
        else if (nrChannels == 4)
            format = GL_RGBA;  
        glGenTextures(1, &textura);
        glBindTexture(GL_TEXTURE_2D, textura);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    else{
        std::cout << "Falha ao ler a textura" << std::endl;
    }

    stbi_image_free(data);

    return textura;
}

int adicionaModelo(const std::string& caminhoOBJ, const std::string& caminhoTextura = ""){
    int nv = 0;

    GLuint vao = loadSimpleOBJ(caminhoOBJ, nv);

    if (vao == 0 || nv <= 0) {
        std::cerr << "Erro ao carregar modelo: "
                  << caminhoOBJ << std::endl;

        return -1;
    }

    GLuint textura = 0;
    bool temTextura = false;

    if (!caminhoTextura.empty()) {
        textura = carregarTextura(caminhoTextura);

        if (textura != 0) {
            temTextura = true;
        }
    }

    TipoModelo modelo;

    modelo.Vao = vao;
    modelo.nVertices = nv;
    modelo.Textura = textura;
    modelo.temTextura = temTextura;

    tiposModelos.push_back(modelo);

    return static_cast<int>(tiposModelos.size() - 1);
}


void carregaModelosObj(){

    texColormap = carregarTextura("../assets/obj/Textures/colormap.png");

    int nv; 

    int building_a = adicionaModelo(
        "../assets/obj/building-a.obj",
        "../assets/obj/Textures/colormap.png"
    );

    int building_b = adicionaModelo(
        "../assets/obj/building-b.obj"
        
    );


    int windmill = adicionaModelo(
        "../assets/obj/windmill.obj"
    
    );

    auto adicionaInstancia = [](int tipoIndex, const glm::vec3& posicao) {
        if (tipoIndex < 0) {
            return;
        }

        instancias.push_back({
            tipoIndex,
            posicao,
            glm::vec3(1.0f),
            0.0f
        });
    };

    adicionaInstancia(building_a, glm::vec3(0.0f, -1.75f, -10.0f));
    adicionaInstancia(building_b, glm::vec3(5.0f, -1.75f, -8.0f));
    adicionaInstancia(windmill, glm::vec3(10.0f, -1.75f, -6.0f));
    
    
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

    Window = glfwCreateWindow(WIDTH, HEIGHT, "Vila Industrial", NULL, NULL);
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
    
float cores[] = {
 
    0.45f, 0.29f, 0.15f, 0.45f, 0.29f, 0.15f, 0.45f, 0.29f, 0.15f, 0.45f, 0.29f, 0.15f, 0.45f, 0.29f, 0.15f, 0.45f, 0.29f, 0.15f,
    0.45f, 0.29f, 0.15f, 0.45f, 0.29f, 0.15f, 0.45f, 0.29f, 0.15f, 0.45f, 0.29f, 0.15f, 0.45f, 0.29f, 0.15f, 0.45f, 0.29f, 0.15f,
    0.45f, 0.29f, 0.15f, 0.45f, 0.29f, 0.15f, 0.45f, 0.29f, 0.15f, 0.45f, 0.29f, 0.15f, 0.45f, 0.29f, 0.15f, 0.45f, 0.29f, 0.15f,
    0.45f, 0.29f, 0.15f, 0.45f, 0.29f, 0.15f, 0.45f, 0.29f, 0.15f, 0.45f, 0.29f, 0.15f, 0.45f, 0.29f, 0.15f, 0.45f, 0.29f, 0.15f,
    0.45f, 0.29f, 0.15f, 0.45f, 0.29f, 0.15f, 0.45f, 0.29f, 0.15f, 0.45f, 0.29f, 0.15f, 0.45f, 0.29f, 0.15f, 0.45f, 0.29f, 0.15f,
    0.2f, 0.5f, 0.2f, 0.2f, 0.5f, 0.2f, 0.2f, 0.5f, 0.2f, 0.2f, 0.5f, 0.2f, 0.2f, 0.5f, 0.2f, 0.2f, 0.5f, 0.3f,
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



    std::string vertexCodeTexture = leShaderDoArquivo("../assets/shaders/vertex_texture.glsl");
    std::string fragmentCodeTexture = leShaderDoArquivo("../assets/shaders/fragment_texture_shader.glsl");

    const char* vertex_texture_shader = vertexCodeTexture.c_str();
    const char* fragment_texture_shader = fragmentCodeTexture.c_str();

    GLuint vs_texture = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs_texture, 1, &vertex_texture_shader, NULL);
    glCompileShader(vs_texture);

    GLuint fs_texture = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs_texture, 1, &fragment_texture_shader, NULL);
    glCompileShader(fs_texture);

    Shader_texture = glCreateProgram();
    glAttachShader(Shader_texture, vs_texture);
    glAttachShader(Shader_texture, fs_texture);
    glLinkProgram(Shader_texture);

    GLint successTex;
    char infoLogTex[512];
    glGetProgramiv(Shader_texture, GL_LINK_STATUS, &successTex);
    if (!successTex) {
        glGetProgramInfoLog(Shader_texture, 512, NULL, infoLogTex);
        std::cerr << "Erro ao linkar Shader_texture:\n" << infoLogTex << std::endl;
    }

    glDeleteShader(vs_texture);
    glDeleteShader(fs_texture);

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

    if (glfwGetKey(Window, GLFW_KEY_Z) == GLFW_PRESS) {
        Cam_fov = 20.0f;
    } else {
        Cam_fov = 67.0f;
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

void desenhaCenario(glm::mat4 view , glm::mat4 proj ) {
    GLint transformLoc = glGetUniformLocation(Shader_programm, "model");
    glm::mat4 transformacao;

    transformacao = glm::mat4(1.0f);
    transformacao = glm::translate(transformacao, glm::vec3(0.0f, -2.0f, -10.0f));
    transformacao = glm::scale(transformacao, glm::vec3(30.0f, 0.5f, 30.0f));
    glUniformMatrix4fv(transformLoc, 1, GL_FALSE, glm::value_ptr(transformacao));
    glDrawArrays(GL_TRIANGLES, 0, 36);

}

void desenhaModeloOBJ(glm::mat4 view, glm::mat4 proj) {

    glUseProgram(Shader_texture);

    glActiveTexture(GL_TEXTURE0);
    glUniform1i(glGetUniformLocation(Shader_texture, "textura1"), 0); 

    for (InstanciaModelo& instancia : instancias) {
        TipoModelo& tipo = tiposModelos[instancia.tipoIndex];

        glm::mat4 model = glm::mat4(1.0f);

        model = glm::translate(
            model,
            instancia.posicao
        );

        model = glm::rotate(
            model,
            glm::radians(instancia.rotacaoY),
            glm::vec3(0.0f, 1.0f, 0.0f)
        );

        model = glm::scale(
            model,
            instancia.escala
        );


        glUniformMatrix4fv(
            glGetUniformLocation(Shader_texture, "model"),
            1,
            GL_FALSE,
            glm::value_ptr(model)
        );

        glUniformMatrix4fv(
            glGetUniformLocation(Shader_texture, "view"),
            1,
            GL_FALSE,
            glm::value_ptr(view)
        );

        glUniformMatrix4fv(
            glGetUniformLocation(Shader_texture, "proj"),
            1,
            GL_FALSE,
            glm::value_ptr(proj)
        );


        // Textura do modelo
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, tipo.Textura);


        
        glBindVertexArray(tipo.Vao);

        
        glDrawArrays(
            GL_TRIANGLES,
            0,
            tipo.nVertices
        );
    }
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

        
        glViewport(0, 0, WIDTH, HEIGHT);

       
        glm::mat4 viewPerspectiva = glm::lookAt(Cam_pos, Cam_pos + Cam_front, Cam_up);
        GLint viewLoc = glGetUniformLocation(Shader_programm, "view");
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(viewPerspectiva));

        float aspecto = (float)WIDTH / (float)HEIGHT;
        glm::mat4 projPerspectiva = glm::perspective(glm::radians(Cam_fov), aspecto, 0.1f, 100.0f);
        GLint projLoc = glGetUniformLocation(Shader_programm, "proj");
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projPerspectiva));

        desenhaCenario(viewPerspectiva, projPerspectiva);
        desenhaModeloOBJ(viewPerspectiva, projPerspectiva);

    
        glClear(GL_DEPTH_BUFFER_BIT);

        
        int mapa_size = 200; 
        glViewport(WIDTH - mapa_size, HEIGHT - mapa_size, mapa_size, mapa_size);

    
        glm::vec3 mapa_pos = glm::vec3(0.0f, 30.0f, 0.0f);
        glm::vec3 mapa_front = glm::vec3(0.0f, -1.0f, 0.0f);
        glm::vec3 mapa_up = glm::vec3(0.0f, 0.0f, -1.0f); 
        
        glm::mat4 viewOrtografica = glm::lookAt(mapa_pos, mapa_pos + mapa_front, mapa_up);
        float limite = 15.0f; 
        glm::mat4 projOrtografica = glm::ortho(-limite, limite, -limite, limite, 0.1f, 100.0f);


        glUseProgram(Shader_programm);
        glBindVertexArray(Vao);
        glUniformMatrix4fv(glGetUniformLocation(Shader_programm, "view"), 1, GL_FALSE, glm::value_ptr(viewOrtografica));
        glUniformMatrix4fv(glGetUniformLocation(Shader_programm, "proj"), 1, GL_FALSE, glm::value_ptr(projOrtografica));


        desenhaCenario(viewOrtografica, projOrtografica);
        desenhaModeloOBJ(viewOrtografica, projOrtografica);
        

        glfwPollEvents();
        glfwSwapBuffers(Window);
    }
    
    glfwTerminate();
}

int main() {
    inicializaOpenGL();
    inicializaObjetos();
    inicializaShaders();
    carregaModelosObj();
    inicializaRenderizacao();
    return 0;
}