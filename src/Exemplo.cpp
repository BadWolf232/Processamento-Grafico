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


// Variáveis globais
GLFWwindow* Window = nullptr; // Ponteiro para a janela GLFW
GLuint Shader_programm = 0; // ID do programa de shader
GLuint Vao = 0; // ID do Vao
GLuint Shader_color = 0; // ID do shader de cor
GLuint Shader_texture = 0; // ID do shader de textura
int nVertices; // Número de vértices



bool wireframeMode = false; // Variável para alternar entre wireframe e modo completo
bool teclaGPress = false;  // Variável para controlar o estado da tecla 'G'

// Variáveis para armazenar os IDs dos modelos para a animação do moinho de vento
int id_moinho_base = -1; 
int id_moinho_pas = -1; 

struct TipoModelo { // Estrutura para armazenar informações sobre cada tipo de modelo, unindo as diversos modelos
    GLuint Vao;
    int nVertices;
    GLuint Textura;
    bool temTextura;
};

struct InstanciaModelo { // Estrutura para armazenar informações sobre cada instância de modelo
    int tipoIndex;
    glm::vec3 posicao;
    glm::vec3 escala;
    float rotacaoY;
};

std::vector<TipoModelo> tiposModelos; // Vetor para armazenar os tipos de modelos
std::vector<InstanciaModelo> instancias; // Vetor para armazenar as instâncias de modelos
GLuint texColormap; // ID da textura do colormap

int WIDTH = 1000; // Largura da janela
int HEIGHT = 800; // Altura da janela

float Tempo_frames = 0.0f; //Variável para armazenar o tempo entre os frames

float Cam_velo = 5.0f; // Velocidade da câmera
glm::vec3 Cam_posicao = glm::vec3(0.0f, 0.0f, 2.0f); // Posição da câmera
glm::vec3 Cam_dir = glm::vec3(0.0f, 0.0f, -1.0f); // Direção da câmera
glm::vec3 Cam_cima = glm::vec3(0.0f, 1.0f, 0.0f); // Vetor para cima da câmera

float Cam_yrot = 0.0f; // Ângulo de rotação da câmera no eixo Y
float Cam_xrot = 0.0f; // Ângulo de rotação da câmera no eixo X

double lastX = WIDTH / 2.0; // Posição X do último movimento do mouse
double lastY = HEIGHT / 2.0; // Posição Y do último movimento do mouse
bool primeiro_mouse = true; // Verificação do primeiro movimento do mouse

float Cam_fov = 67.0f; // Fov da camera

// Função para carregar um modelo OBJ simples
int loadSimpleOBJ(string filePATH, int &nVertices){
    std:: vector<glm::vec3> vertices;
    std:: vector<glm::vec2> textCoords;
    std:: vector<glm::vec3> normals; 

    std:: vector<GLfloat> vBuffer;

    std::ifstream dadosEntrada(filePATH.c_str());
    if(!dadosEntrada.is_open()){
        std::cerr << "ERRO: Não foi possivel abrir o arquivo OBJ: " << filePATH << std::endl;
        return -1;
    } 
    // Processar o arquivo OBJ
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
    
    // Processar as faces do modelo
    std::string linha;
    int numeroLinha = 0;
    // Ler cada linha do arquivo OBJ
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

            // Triangularizar a face se ela tiver mais de 3 vértices
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

    // Gerar o buffer de geometria do OBJ
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


// Função para ler o conteúdo de um arquivo de shader
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

// Função para carregar uma textura
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

// Função para adicionar um modelo à lista de tipos de modelos
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
        "../assets/obj/building-b.obj",
        "../assets/obj/Textures/colormap.png"
    );

    int water_tower = adicionaModelo(
        "../assets/obj/water-tower.obj",
        "../assets/obj/Textures/colormap.png"
    ); 

    int chimney = adicionaModelo(
        "../assets/obj/chimney-medium.obj",
        "../assets/obj/Textures/colormap.png"
    );

    int building_m = adicionaModelo(
        "../assets/obj/building-m.obj",
       "../assets/obj/Textures/colormap.png"
    );

    id_moinho_base = adicionaModelo(
        "../assets/obj/windmill_base.obj",
        "../assets/obj/Textures/colormap.png"
    );

    id_moinho_pas = adicionaModelo(
        "../assets/obj/windmill_pas.obj",
        "../assets/obj/Textures/colormap.png"
    );

    int tank_large = adicionaModelo(
        "../assets/obj/detail-tank-large.obj",
        "../assets/obj/Textures/colormap.png"
    );


    // Coloca as instâncias dos modelos na vila

    instancias.push_back({tank_large, glm::vec3(0.0f, -1.75f, 0.0f), glm::vec3(1.0f), 0.0f});

    instancias.push_back({building_a, glm::vec3(-8.0f, -1.75f, -10.0f), glm::vec3(1.0f), -90.0f});
    instancias.push_back({building_a, glm::vec3(-8.0f, -1.75f, -14.0f), glm::vec3(1.0f), -90.0f});
    instancias.push_back({building_a, glm::vec3(-8.0f, -1.75f, -20.0f), glm::vec3(1.0f), -90.0f});
    instancias.push_back({building_a, glm::vec3(-8.0f, -1.75f, -4.0f), glm::vec3(1.0f), -90.0f});

    instancias.push_back({building_b, glm::vec3(8.0f, -1.75f, -10.0f), glm::vec3(1.0f), 90.0f});
    instancias.push_back({building_b, glm::vec3(8.0f, -1.75f, -14.0f), glm::vec3(1.0f), 90.0f});
    instancias.push_back({building_b, glm::vec3(8.0f, -1.75f, -20.0f), glm::vec3(1.0f), 90.0f});
    instancias.push_back({building_b, glm::vec3(8.0f, -1.75f, -4.0f), glm::vec3(1.0f), 90.0f});


    instancias.push_back({building_m, glm::vec3(-9.0f, -1.75f, -24.0f), glm::vec3(1.0f), 0.0f});
    instancias.push_back({building_m, glm::vec3(-3.0f, -1.75f,  -24.0f), glm::vec3(1.0f), 0.0f});
    instancias.push_back({building_m, glm::vec3( 3.0f, -1.75f,  -24.0f), glm::vec3(1.0f), 0.0f});
    instancias.push_back({building_m, glm::vec3( 9.0f, -1.75f,  -24.0f), glm::vec3(1.0f), 0.0f});
    
    instancias.push_back({water_tower, glm::vec3( -8.0f, -1.75f,  -2.0f), glm::vec3(1.0f), 0.0f}); 
    instancias.push_back({water_tower, glm::vec3( -8.0f, -1.75f,  -8.0f), glm::vec3(1.0f), 0.0f}); 
    instancias.push_back({water_tower, glm::vec3( -8.0f, -1.75f,  -12.0f), glm::vec3(1.0f), 0.0f}); 
    instancias.push_back({water_tower, glm::vec3( -8.0f, -1.75f,  -18.0f), glm::vec3(1.0f), 0.0f}); 

    instancias.push_back({id_moinho_base, glm::vec3(-10.0f, -1.75f, 2.0f), glm::vec3(1.0f), 45.0f}); 
    instancias.push_back({id_moinho_pas, glm::vec3(-10.0f, -1.75f, 2.0f), glm::vec3(1.0f), 45.0f}); 

    instancias.push_back({id_moinho_base, glm::vec3( 10.0f, -1.75f,  2.0f), glm::vec3(1.0f), -45.0f}); 
    instancias.push_back({id_moinho_pas, glm::vec3( 10.0f, -1.75f, 2.0f), glm::vec3(1.0f), -45.0f}); 

    instancias.push_back({chimney, glm::vec3( 9.0f, -1.75f,  -4.0f), glm::vec3(1.0f), 0.0f}); 
    instancias.push_back({chimney, glm::vec3( 9.0f, -1.75f,  -10.0f), glm::vec3(1.0f), 0.0f}); 
    instancias.push_back({chimney, glm::vec3( 9.0f, -1.75f,  -14.0f), glm::vec3(1.0f), 0.0f}); 
    instancias.push_back({chimney, glm::vec3( 9.0f, -1.75f,  -20.0f), glm::vec3(1.0f), 0.0f}); 
    
}
// Função para carregar uma textura
void redimensionaCallback(GLFWwindow* window, int w, int h) {
    WIDTH = w;
    HEIGHT = h;
}
// Função de callback para o movimento do mouse
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

    Cam_yrot -= xoffset;
    Cam_xrot += yoffset;

    if (Cam_xrot > 89.0f) Cam_xrot = 89.0f;
    if (Cam_xrot < -89.0f) Cam_xrot = -89.0f;
}


// Função para inicializar o OpenGL e criar a janela
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


// Função que vai incializar os objetos primitivos, adcionando os triangulos e cores que vão moldar o chão da vila
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
 
    0.2f, 0.5f, 0.2f, 0.2f, 0.5f, 0.2f, 0.2f, 0.5f, 0.2f, 0.2f, 0.5f, 0.2f, 0.2f, 0.5f, 0.2f, 0.2f, 0.5f, 0.2f,
    0.2f, 0.5f, 0.2f, 0.2f, 0.5f, 0.2f, 0.2f, 0.5f, 0.2f, 0.2f, 0.5f, 0.2f, 0.2f, 0.5f, 0.2f, 0.2f, 0.5f, 0.2f,
    0.2f, 0.5f, 0.2f, 0.2f, 0.5f, 0.2f, 0.2f, 0.5f, 0.2f, 0.2f, 0.5f, 0.2f, 0.2f, 0.5f, 0.2f, 0.2f, 0.5f, 0.2f,
    0.2f, 0.5f, 0.2f, 0.2f, 0.5f, 0.2f, 0.2f, 0.5f, 0.2f, 0.2f, 0.5f, 0.2f, 0.2f, 0.5f, 0.2f, 0.2f, 0.5f, 0.2f,
    0.2f, 0.5f, 0.2f, 0.2f, 0.5f, 0.2f, 0.2f, 0.5f, 0.2f, 0.2f, 0.5f, 0.2f, 0.2f, 0.5f, 0.2f, 0.2f, 0.5f, 0.2f,
    0.2f, 0.5f, 0.2f, 0.2f, 0.5f, 0.2f, 0.2f, 0.5f, 0.2f, 0.2f, 0.5f, 0.2f, 0.2f, 0.5f, 0.2f, 0.2f, 0.5f, 0.2f,
};
    
    GLuint cvbo;
    glGenBuffers(1, &cvbo);
    glBindBuffer(GL_ARRAY_BUFFER, cvbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cores), cores, GL_STATIC_DRAW);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, (void*)0); 

}


void inicializaShaders() {

    // Carrega o código dos shaders e vertex utilizados no chão da vila a partir dos arquivos
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


    // Carrega o código dos shaders e fragment utilizados para a aplicação de texturas a partir dos arquivos

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
    front.x = sin(glm::radians(-Cam_yrot)) * cos(glm::radians(Cam_xrot));
    front.y = sin(glm::radians(Cam_xrot)); 
    front.z = -cos(glm::radians(-Cam_yrot)) * cos(glm::radians(Cam_xrot));
    Cam_dir = glm::normalize(front);
}


// Função para tratar a entrada do teclado
void trataTeclado() {
    
    // Botão interação para alternar entre o modo wireframe e o modo preenchido
    if (glfwGetKey(Window, GLFW_KEY_G) == GLFW_PRESS) {
        if(!teclaGPress){
        wireframeMode = !wireframeMode;
        if (wireframeMode) {
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        } else {
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        }
        teclaGPress = true;
        }else{
            teclaGPress = false;
        }
    }

    glm::vec3 Cam_right = glm::normalize(glm::cross(Cam_dir, Cam_cima));

    if (glfwGetKey(Window, GLFW_KEY_A) == GLFW_PRESS)
        Cam_posicao -= Cam_right * Cam_velo * Tempo_frames;
    if (glfwGetKey(Window, GLFW_KEY_D) == GLFW_PRESS)
        Cam_posicao += Cam_right * Cam_velo * Tempo_frames;
    if (glfwGetKey(Window, GLFW_KEY_W) == GLFW_PRESS)
        Cam_posicao += Cam_dir * Cam_velo * Tempo_frames;
    if (glfwGetKey(Window, GLFW_KEY_S) == GLFW_PRESS)
        Cam_posicao -= Cam_dir * Cam_velo * Tempo_frames;
    if (glfwGetKey(Window, GLFW_KEY_E) == GLFW_PRESS)
        Cam_posicao.y += Cam_velo * Tempo_frames;
    if (glfwGetKey(Window, GLFW_KEY_Q) == GLFW_PRESS)
        Cam_posicao.y -= Cam_velo * Tempo_frames;
        
    if (glfwGetKey(Window, GLFW_KEY_F) == GLFW_PRESS) {
        glfwSetWindowShouldClose(Window, true);
    }

    if (glfwGetKey(Window, GLFW_KEY_Z) == GLFW_PRESS) {
        Cam_fov = 20.0f;
    } else {
        Cam_fov = 67.0f;
    }
}

// Função para desenhar o cenário (chão da vila)
void desenhaCenario(glm::mat4 view , glm::mat4 proj ) {
    GLint transformLoc = glGetUniformLocation(Shader_programm, "model");
    glm::mat4 transformacao;

    transformacao = glm::mat4(1.0f);
    transformacao = glm::translate(transformacao, glm::vec3(0.0f, -2.0f, -10.0f));
    transformacao = glm::scale(transformacao, glm::vec3(25.0f, 0.5f, 30.0f));
    glUniformMatrix4fv(transformLoc, 1, GL_FALSE, glm::value_ptr(transformacao));
    glDrawArrays(GL_TRIANGLES, 0, 36);

}

// Função para desenhar os modelos OBJ carregados
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

        // Condicional que irá realizar a rotação das pás do moinho, fazendo com que elas girem em torno do eixo X
        if (instancia.tipoIndex == id_moinho_pas) {
            //move a origem do moinho para o centro da base antes de rotacionar
            model = glm::translate(model, glm::vec3(0.0f, 1.676f, 0.0f));
            //rotaciona o moinho em torno do eixo X
            model = glm::rotate(model, (float)glfwGetTime() * 1.5f, glm::vec3(1.0f, 0.0f, 0.0f));
            //desloca o moinho de volta para a posição original
            model = glm::translate(model, glm::vec3(0.0f, -1.676f, 0.0f));

        }

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


// Função principal de renderização, que contém o loop principal do programa
void renderizacao() {
    double tempoAnterior = glfwGetTime();

    glEnable(GL_DEPTH_TEST);
    
    while (!glfwWindowShouldClose(Window)) {
        double tempoFrameAtual = glfwGetTime();
        Tempo_frames = (float)(tempoFrameAtual - tempoAnterior);
        tempoAnterior = tempoFrameAtual;

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // Limpa o buffer de cor e profundidade antes de desenhar a cena
        
        glUseProgram(Shader_programm);
        glBindVertexArray(Vao);
        
        trataTeclado();
        atualizaDirecaoCamera();

        // Renderiza a camera principal
        glViewport(0, 0, WIDTH, HEIGHT);

       
        glm::mat4 viewPerspectiva = glm::lookAt(Cam_posicao, Cam_posicao + Cam_dir, Cam_cima);
        GLint viewLoc = glGetUniformLocation(Shader_programm, "view");
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(viewPerspectiva));

        float aspecto = (float)WIDTH / (float)HEIGHT;
        glm::mat4 projPerspectiva = glm::perspective(glm::radians(Cam_fov), aspecto, 0.1f, 100.0f);
        GLint projLoc = glGetUniformLocation(Shader_programm, "proj");
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projPerspectiva));

        desenhaCenario(viewPerspectiva, projPerspectiva);
        desenhaModeloOBJ(viewPerspectiva, projPerspectiva);

    
        glClear(GL_DEPTH_BUFFER_BIT); // Limpa o buffer de profundidade antes de desenhar a miniatura

        // Renderiza o minimapa no canto superior direito
        int mapa_size = 300; 
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
    renderizacao();
    return 0;
}