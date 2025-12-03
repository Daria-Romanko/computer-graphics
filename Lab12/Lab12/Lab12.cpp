#include <SFML/Window.hpp>
#include <SFML/Graphics.hpp>
#include <GL/glew.h>
#include <iostream>
#include <string>
#include <cmath>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// Вершинный шейдер для тетраэдра
const char* vertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;

out vec3 ourColor;

uniform vec3 offset;
uniform mat4 rotation;

void main()
{
    gl_Position = rotation * vec4(aPos + offset, 1.0);
    ourColor = aColor;
}
)";

// Фрагментный шейдер для тетраэдра
const char* fragmentShaderSource = R"(
#version 330 core
out vec4 FragColor;
in vec3 ourColor;

void main()
{
    FragColor = vec4(ourColor, 1.0);
}
)";

const char* vertexShaderTexSrc = R"(
layout (location = 0) in vec3 position;
layout (location = 2) in vec2 texCoord;

out vec3 color;
out vec2 TexCoord;

uniform vec3 offset;
uniform mat4 rotation;

void main()
{
    gl_Position = rotation * vec4(position + offset, 1.0);
    TexCoord = texCoord;
    color = position + vec3(0.5);
}
)";


const char* fragmentShaderTexColorSrc = R"(
in vec3 color;
in vec2 TexCoord;
out vec4 FragColor;

uniform sampler2D texture;
uniform float colorMix;

void main()
{
    vec4 gradColor = vec4(color, 1.0);          
    vec4 texColor = texture(texture, TexCoord);
    vec4 tinted = texColor * gradColor;        

    FragColor = mix(gradColor, tinted, colorMix);
}
)";


const char* fragmentShaderTwoTexSrc = R"(
in vec2 TexCoord;
out vec4 FragColor;

uniform sampler2D texture1;
uniform sampler2D texture2;
uniform float textureMix;

void main()
{
    vec4 t1 = texture(texture1, TexCoord);
    vec4 t2 = texture(texture2, TexCoord);
    FragColor = mix(t1, t2, textureMix);
}
)";


GLuint compileShader(GLenum type, const char* source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    int success;
    char infoLog[512];
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        std::cout << "Ошибка компиляции шейдера:\n" << infoLog << std::endl;
    }

    return shader;
}

GLuint createShaderProgram(const char* vsSource, const char* fsSource) {
    GLuint vertexShader = compileShader(GL_VERTEX_SHADER, vsSource);
    GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, fsSource);

    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    int success;
    char infoLog[512];
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram, 512, nullptr, infoLog);
        std::cout << "Ошибка линковки шейдерной программы:\n" << infoLog << std::endl;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return shaderProgram;
}

GLuint loadTexture(const char* path) {
    int width, height, nrChannels;
    stbi_set_flip_vertically_on_load(true);
    unsigned char* data = stbi_load(path, &width, &height, &nrChannels, 0);
    if (!data) {
        std::cout << "Ошибка загрузки текстуры: " << path << std::endl;
        return 0;
    }

    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    glTexImage2D(GL_TEXTURE_2D, 0, 3, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    stbi_image_free(data);

    return texture;
}

int main() {
    sf::Window window(sf::VideoMode({ 800, 600 }), "3D figures");
    window.setFramerateLimit(60);

    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        std::cout << "Ошибка инициализации GLEW!" << std::endl;
        return -1;
    }

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glClearDepth(1.f);

    float vertices[] = {
        // Позиции         // Цвета
        -0.5f, -0.5f,  0.0f,  1.0f, 0.0f, 0.0f,  // Красная вершина (передняя)
         0.5f, -0.5f,  0.0f,  0.0f, 1.0f, 0.0f,  // Зеленая вершина (правая)
         0.0f, -0.5f,  0.8f,  0.0f, 0.0f, 1.0f,  // Синяя вершина (задняя)
         0.0f,  0.5f,  0.4f,  1.0f, 1.0f, 0.0f   // Желтая вершина (верхняя)
    };

    unsigned int indices[] = {
        0, 1, 2,  // Основание
        0, 1, 3,  // Боковая грань
        1, 2, 3,  // Боковая грань
        2, 0, 3   // Боковая грань
    };

    GLuint VBO, VAO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // позиция
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // цвет
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);

    GLuint shaderProgram = createShaderProgram(vertexShaderSource, fragmentShaderSource);
    GLuint offsetLocation = glGetUniformLocation(shaderProgram, "offset");
    GLuint rotationLocation = glGetUniformLocation(shaderProgram, "rotation");

    float cubeVertices[] = {
        -0.5f,-0.5f, 0.5f,     1.0f,0.0f,0.0f,      0.0f,0.0f,  
         0.5f,-0.5f, 0.5f,     0.0f,1.0f,0.0f,      1.0f,0.0f,  
         0.5f, 0.5f, 0.5f,     0.0f,0.0f,1.0f,      1.0f,1.0f,  
        -0.5f, 0.5f, 0.5f,     1.0f,1.0f,0.0f,      0.0f,1.0f,  

        -0.5f,-0.5f,-0.5f,     1.0f,0.0f,1.0f,      0.0f,0.0f,  
         0.5f,-0.5f,-0.5f,     0.0f,1.0f,1.0f,      1.0f,0.0f,  
         0.5f, 0.5f,-0.5f,     0.5f,0.5f,0.5f,      1.0f,1.0f,  
        -0.5f, 0.5f,-0.5f,     1.0f,0.5f,0.0f,      0.0f,1.0f,  

        -0.5f,-0.5f,-0.5f,     1.0f,0.0f,0.0f,      0.0f,0.0f,  
        -0.5f,-0.5f, 0.5f,     0.0f,1.0f,0.0f,      1.0f,0.0f,  
        -0.5f, 0.5f, 0.5f,     0.0f,0.0f,1.0f,      1.0f,1.0f,  
        -0.5f, 0.5f,-0.5f,     1.0f,1.0f,0.0f,      0.0f,1.0f,  

         0.5f,-0.5f,-0.5f,     1.0f,0.0f,0.0f,      0.0f,0.0f,  
         0.5f,-0.5f, 0.5f,     0.0f,1.0f,0.0f,      1.0f,0.0f,  
         0.5f, 0.5f, 0.5f,     0.0f,0.0f,1.0f,      1.0f,1.0f,  
         0.5f, 0.5f,-0.5f,     1.0f,1.0f,0.0f,      0.0f,1.0f,  

         -0.5f, 0.5f,-0.5f,     1.0f,0.0f,0.0f,      0.0f,0.0f, 
          0.5f, 0.5f,-0.5f,     0.0f,1.0f,0.0f,      1.0f,0.0f, 
          0.5f, 0.5f, 0.5f,     0.0f,0.0f,1.0f,      1.0f,1.0f,
         -0.5f, 0.5f, 0.5f,     1.0f,1.0f,0.0f,      0.0f,1.0f,

         -0.5f,-0.5f,-0.5f,     1.0f,0.0f,0.0f,      0.0f,0.0f,
          0.5f,-0.5f,-0.5f,     0.0f,1.0f,0.0f,      1.0f,0.0f, 
          0.5f,-0.5f, 0.5f,     0.0f,0.0f,1.0f,      1.0f,1.0f,  
         -0.5f,-0.5f, 0.5f,     1.0f,1.0f,0.0f,      0.0f,1.0f   
    };

    unsigned int cubeIndices[] = {
        0, 1, 2,  0, 2, 3,
        4, 5, 6,  4, 6, 7,
        8, 9,10,  8,10,11,
        12,13,14, 12,14,15,
        16,17,18, 16,18,19,
        20,21,22, 20,22,23
    };

    GLuint cubeVAO, cubeVBO, cubeEBO;
    glGenVertexArrays(1, &cubeVAO);
    glGenBuffers(1, &cubeVBO);
    glGenBuffers(1, &cubeEBO);

    glBindVertexArray(cubeVAO);

    glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), cubeVertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cubeEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(cubeIndices), cubeIndices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);

    GLuint shaderTexColor = createShaderProgram(vertexShaderTexSrc, fragmentShaderTexColorSrc);
    GLuint shaderTwoTex = createShaderProgram(vertexShaderTexSrc, fragmentShaderTwoTexSrc);

    GLuint texColorOffsetLoc = glGetUniformLocation(shaderTexColor, "offset");
    GLuint texColorMixLoc = glGetUniformLocation(shaderTexColor, "colorMix");
    GLuint texColorSamplerLoc = glGetUniformLocation(shaderTexColor, "ourTexture");
    GLuint texColorRotationLoc = glGetUniformLocation(shaderTexColor, "rotation");

    GLuint twoTexOffsetLoc = glGetUniformLocation(shaderTwoTex, "offset");
    GLuint twoTexMixLoc = glGetUniformLocation(shaderTwoTex, "textureMix");
    GLuint twoTexSampler1Loc = glGetUniformLocation(shaderTwoTex, "texture1");
    GLuint twoTexSampler2Loc = glGetUniformLocation(shaderTwoTex, "texture2");
    GLuint twoTexRotationLoc = glGetUniformLocation(shaderTwoTex, "rotation");

    GLuint texture1 = loadTexture("hamster.jpg");
    GLuint texture2 = loadTexture("simpson.jpg");
    GLuint texture3 = loadTexture("grass.jpg");


    if (!texture1 || !texture2) {
        std::cout << "Не удалось загрузить одну или обе текстуры!" << std::endl;
    }

    float offsets[3][3] = {
        {0.0f, 0.0f, 0.0f},   // тетраэдр
        {0.0f, 0.0f, 0.0f},   // куб с текстурой+градиентом
        {0.0f, 0.0f, 0.0f}    // куб с двумя текстурами
    };

    float moveSpeed = 0.05f;
    float colorMix = 0.5f; 
    float textureMix = 0.5f;  
    int   currentFigure = 1;   

    glm::mat4 rotation = glm::mat4(1.0f);
    rotation = glm::rotate(rotation, glm::radians(45.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    rotation = glm::rotate(rotation, glm::radians(30.0f), glm::vec3(1.0f, 0.0f, 0.0f));

    while (window.isOpen()) {
        for (auto event = window.pollEvent(); event.has_value(); event = window.pollEvent()) {
            const auto& e = event.value();

            if (e.is<sf::Event::Closed>()) {
                window.close();
            }
            else if (const auto* keyEvent = e.getIf<sf::Event::KeyPressed>()) {

                int idx = currentFigure - 1;

                switch (keyEvent->scancode) {
                    // выбор фигуры
                case sf::Keyboard::Scancode::Num1:
                    currentFigure = 1;
                    break;
                case sf::Keyboard::Scancode::Num2:
                    currentFigure = 2;
                    break;
                case sf::Keyboard::Scancode::Num3:
                    currentFigure = 3;
                    break;

                    // смещения
                case sf::Keyboard::Scancode::W:
                    offsets[idx][1] += moveSpeed;
                    break;
                case sf::Keyboard::Scancode::S:
                    offsets[idx][1] -= moveSpeed;
                    break;
                case sf::Keyboard::Scancode::A:
                    offsets[idx][0] -= moveSpeed;
                    break;
                case sf::Keyboard::Scancode::D:
                    offsets[idx][0] += moveSpeed;
                    break;
                case sf::Keyboard::Scancode::Q:
                    offsets[idx][2] += moveSpeed;
                    break;
                case sf::Keyboard::Scancode::E:
                    offsets[idx][2] -= moveSpeed;
                    break;
                case sf::Keyboard::Scancode::R:
                    offsets[idx][0] = offsets[idx][1] = offsets[idx][2] = 0.0f;
                    break;

                    // смешивание цвета/текстур
                case sf::Keyboard::Scancode::Up:
                    if (currentFigure == 2) {
                        colorMix += 0.05f;
                        if (colorMix > 1.0f) colorMix = 1.0f;
                    }
                    else if (currentFigure == 3) {
                        textureMix += 0.05f;
                        if (textureMix > 1.0f) textureMix = 1.0f;
                    }
                    break;
                case sf::Keyboard::Scancode::Down:
                    if (currentFigure == 2) {
                        colorMix -= 0.05f;
                        if (colorMix < 0.0f) colorMix = 0.0f;
                    }
                    else if (currentFigure == 3) {
                        textureMix -= 0.05f;
                        if (textureMix < 0.0f) textureMix = 0.0f;
                    }
                    break;

                default:
                    break;
                }
            }
        }

        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        if (currentFigure == 1) {
            glUseProgram(shaderProgram);
            glUniform3f(offsetLocation, offsets[0][0], offsets[0][1], offsets[0][2]);
            glUniformMatrix4fv(rotationLocation, 1, GL_FALSE, glm::value_ptr(rotation));

            glBindVertexArray(VAO);
            glDrawElements(GL_TRIANGLES, 12, GL_UNSIGNED_INT, 0);
            glBindVertexArray(0);
        }
        else if (currentFigure == 2) {
            glUseProgram(shaderTexColor);
            glUniformMatrix4fv(texColorRotationLoc, 1, GL_FALSE, glm::value_ptr(rotation));
            glUniform3f(texColorOffsetLoc, offsets[1][0], offsets[1][1], offsets[1][2]);
            glUniform1f(texColorMixLoc, colorMix);

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, texture1);
            glUniform1i(texColorSamplerLoc, 0);

            glBindVertexArray(cubeVAO);
            glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
            glBindVertexArray(0);
        }
        else if (currentFigure == 3) {
            glUseProgram(shaderTwoTex);
            glUniformMatrix4fv(twoTexRotationLoc, 1, GL_FALSE, glm::value_ptr(rotation));
            glUniform3f(twoTexOffsetLoc, offsets[2][0], offsets[2][1], offsets[2][2]);
            glUniform1f(twoTexMixLoc, textureMix);

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, texture2);
            glUniform1i(twoTexSampler1Loc, 0);

            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, texture3);
            glUniform1i(twoTexSampler2Loc, 1);

            glBindVertexArray(cubeVAO);
            glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
            glBindVertexArray(0);
        }

        window.display();
    }

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);

    glDeleteVertexArrays(1, &cubeVAO);
    glDeleteBuffers(1, &cubeVBO);
    glDeleteBuffers(1, &cubeEBO);

    glDeleteProgram(shaderProgram);
    glDeleteProgram(shaderTexColor);
    glDeleteProgram(shaderTwoTex);

    glDeleteTextures(1, &texture1);
    glDeleteTextures(1, &texture2);

    return 0;
}
