#include <SFML/Window.hpp>
#include <SFML/Graphics.hpp>
#include <GL/glew.h>
#include <iostream>
#include <string>
#include <cmath>
#include <vector>
#include <optional>
#include <fstream>
#include <sstream>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

std::string readShaderFile(const std::string& filePath) {
    std::ifstream shaderFile(filePath);
    if (!shaderFile.is_open()) {
        std::cout << "Error: Could not open shader file: " << filePath << std::endl;
        return "";
    }

    std::stringstream shaderStream;
    shaderStream << shaderFile.rdbuf();
    shaderFile.close();

    return shaderStream.str();
}

GLuint compileShader(GLenum type, const char* source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    int success;
    char infoLog[512];
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        std::cout << "Shader compilation error:\n" << infoLog << std::endl;
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
        std::cout << "Shader program linking error:\n" << infoLog << std::endl;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return shaderProgram;
}

GLuint loadTextureSFML(const std::string& filePath) {
    sf::Image image;
    if (!image.loadFromFile(filePath)) {
        std::cout << "Error loading texture with SFML: " << filePath << std::endl;
        return 0;
    }

    image.flipVertically();

    sf::Vector2u size = image.getSize();
    const std::uint8_t* pixels = image.getPixelsPtr();

    GLenum format = GL_RGBA;

    GLuint textureID = 0;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    glTexImage2D(GL_TEXTURE_2D, 0, format, size.x, size.y, 0, format, GL_UNSIGNED_BYTE, pixels);

    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glBindTexture(GL_TEXTURE_2D, 0);

    return textureID;
}

void hsvToRgb(float h, float s, float v, float& r, float& g, float& b) {
    int i = static_cast<int>(h / 60.0f) % 6;
    float f = (h / 60.0f) - i;
    float p = v * (1.0f - s);
    float q = v * (1.0f - f * s);
    float t = v * (1.0f - (1.0f - f) * s);

    switch (i) {
    case 0: r = v; g = t; b = p; break;
    case 1: r = q; g = v; b = p; break;
    case 2: r = p; g = v; b = t; break;
    case 3: r = p; g = q; b = v; break;
    case 4: r = t; g = p; b = v; break;
    case 5: r = v; g = p; b = q; break;
    }
}

void createCircleVertices(std::vector<float>& vertices, std::vector<unsigned int>& indices,
    float radius = 0.5f, int segments = 64) {
    vertices.clear();
    indices.clear();

    vertices.push_back(0.0f);
    vertices.push_back(0.0f);
    vertices.push_back(0.0f);

    vertices.push_back(1.0f); 
    vertices.push_back(1.0f); 
    vertices.push_back(1.0f); 

    for (int i = 0; i <= segments; ++i) {
        float angle = (2.0f * 3.14159265f * i) / segments;
        float x = radius * cos(angle);
        float y = radius * sin(angle);

        float h = (360.0f * i) / segments;
        float r, g, b;
        hsvToRgb(h, 1.0f, 1.0f, r, g, b);

        vertices.push_back(x);
        vertices.push_back(y);
        vertices.push_back(0.0f);

        vertices.push_back(r);
        vertices.push_back(g);
        vertices.push_back(b);
    }

    for (int i = 1; i <= segments; ++i) {
        indices.push_back(0);      
        indices.push_back(i);      
        indices.push_back(i + 1);  
    }

    indices.push_back(0);
    indices.push_back(segments);
    indices.push_back(1);
}

int main() {
    sf::Window window(sf::VideoMode({ 800, 600 }), "3D figures");
    window.setFramerateLimit(60);

    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        std::cout << "Error initializing GLEW!" << std::endl;
        return -1;
    }

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glClearDepth(1.f);

    std::string tetrahedronVS = readShaderFile("tetrahedron.vert");
    std::string tetrahedronFS = readShaderFile("tetrahedron.frag");
    std::string circleVS = readShaderFile("circle.vert");
    std::string circleFS = readShaderFile("circle.frag");
    std::string texColorVS = readShaderFile("tex_color.vert");
    std::string texColorFS = readShaderFile("tex_color.frag");
    std::string twoTexFS = readShaderFile("two_tex.frag");

    if (tetrahedronVS.empty() || tetrahedronFS.empty() ||
        circleVS.empty() || circleFS.empty() ||
        texColorVS.empty() || texColorFS.empty() ||
        twoTexFS.empty()) {
        std::cout << "Error: One or more shader files could not be loaded!" << std::endl;
        return -1;
    }

    float tetrahedronVertices[] = {
        -0.5f, -0.5f,  0.0f,  1.0f, 0.0f, 0.0f,
         0.5f, -0.5f,  0.0f,  0.0f, 1.0f, 0.0f,
         0.0f, -0.5f,  0.8f,  0.0f, 0.0f, 1.0f,
         0.0f,  0.5f,  0.4f,  1.0f, 1.0f, 0.0f
    };

    unsigned int tetrahedronIndices[] = {
        0, 1, 2,
        0, 1, 3,
        1, 2, 3,
        2, 0, 3
    };

    GLuint tetrahedronVAO, tetrahedronVBO, tetrahedronEBO;
    glGenVertexArrays(1, &tetrahedronVAO);
    glGenBuffers(1, &tetrahedronVBO);
    glGenBuffers(1, &tetrahedronEBO);

    glBindVertexArray(tetrahedronVAO);
    glBindBuffer(GL_ARRAY_BUFFER, tetrahedronVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(tetrahedronVertices), tetrahedronVertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, tetrahedronEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(tetrahedronIndices), tetrahedronIndices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);

    std::vector<float> circleVertices;
    std::vector<unsigned int> circleIndices;
    createCircleVertices(circleVertices, circleIndices, 0.5f, 64);

    GLuint circleVAO, circleVBO, circleEBO;
    glGenVertexArrays(1, &circleVAO);
    glGenBuffers(1, &circleVBO);
    glGenBuffers(1, &circleEBO);

    glBindVertexArray(circleVAO);
    glBindBuffer(GL_ARRAY_BUFFER, circleVBO);
    glBufferData(GL_ARRAY_BUFFER, circleVertices.size() * sizeof(float), circleVertices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, circleEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, circleIndices.size() * sizeof(unsigned int), circleIndices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);

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

    GLuint shaderProgram = createShaderProgram(tetrahedronVS.c_str(), tetrahedronFS.c_str());
    GLuint shaderCircle = createShaderProgram(circleVS.c_str(), circleFS.c_str());
    GLuint shaderTexColor = createShaderProgram(texColorVS.c_str(), texColorFS.c_str());
    GLuint shaderTwoTex = createShaderProgram(texColorVS.c_str(), twoTexFS.c_str());

    GLuint offsetLocation = glGetUniformLocation(shaderProgram, "offset");
    GLuint rotationLocation = glGetUniformLocation(shaderProgram, "rotation");

    GLuint circleOffsetLocation = glGetUniformLocation(shaderCircle, "offset");
    GLuint circleRotationLocation = glGetUniformLocation(shaderCircle, "rotation");
    GLuint circleScaleLocation = glGetUniformLocation(shaderCircle, "scale");

    GLuint texColorOffsetLoc = glGetUniformLocation(shaderTexColor, "offset");
    GLuint texColorMixLoc = glGetUniformLocation(shaderTexColor, "colorMix");
    GLuint texColorSamplerLoc = glGetUniformLocation(shaderTexColor, "texture");
    GLuint texColorRotationLoc = glGetUniformLocation(shaderTexColor, "rotation");

    GLuint twoTexOffsetLoc = glGetUniformLocation(shaderTwoTex, "offset");
    GLuint twoTexMixLoc = glGetUniformLocation(shaderTwoTex, "textureMix");
    GLuint twoTexSampler1Loc = glGetUniformLocation(shaderTwoTex, "texture1");
    GLuint twoTexSampler2Loc = glGetUniformLocation(shaderTwoTex, "texture2");
    GLuint twoTexRotationLoc = glGetUniformLocation(shaderTwoTex, "rotation");

    std::cout << "Loading textures with SFML..." << std::endl;

    GLuint texture1 = loadTextureSFML("hamster.jpg");
    GLuint texture2 = loadTextureSFML("simpson.jpg");
    GLuint texture3 = loadTextureSFML("grass.jpg");

    if (!texture1 || !texture2 || !texture3) {
        std::cout << "Error: Could not load one or more textures with SFML!" << std::endl;
        std::cout << "Make sure the following files exist in the working directory:" << std::endl;
        std::cout << "1. hamster.jpg" << std::endl;
        std::cout << "2. simpson.jpg" << std::endl;
        std::cout << "3. grass.jpg" << std::endl;

        if (!texture1) {
            std::cout << "Creating fallback texture for hamster.jpg" << std::endl;
        }
    }
    else {
        std::cout << "All textures loaded successfully with SFML!" << std::endl;
    }

    float offsets[4][3] = {
        {0.0f, 0.0f, 0.0f},
        {0.0f, 0.0f, 0.0f},
        {0.0f, 0.0f, 0.0f},
        {0.0f, 0.0f, 0.0f}
    };

    float scale[4][3] = {
        {1.0f, 1.0f, 1.0f},
        {1.0f, 1.0f, 1.0f},
        {1.0f, 1.0f, 1.0f},
        {1.0f, 1.0f, 1.0f}
    };

    float moveSpeed = 0.05f;
    float scaleSpeed = 0.05f;
    float colorMix = 0.5f;
    float textureMix = 0.5f;
    int currentFigure = 1;

    glm::mat4 rotation = glm::mat4(1.0f);
    rotation = glm::rotate(rotation, glm::radians(45.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    rotation = glm::rotate(rotation, glm::radians(30.0f), glm::vec3(1.0f, 0.0f, 0.0f));

    float angleX = 0.0f;
    float angleY = 0.0f;
    float angleZ = 0.0f;

    while (window.isOpen()) {
        for (auto event = window.pollEvent(); event.has_value(); event = window.pollEvent()) {
            const auto& e = event.value();

            if (e.is<sf::Event::Closed>()) {
                window.close();
            }
            else if (const auto* keyEvent = e.getIf<sf::Event::KeyPressed>()) {

                int idx = currentFigure - 1;

                switch (keyEvent->scancode) {
                case sf::Keyboard::Scancode::Num1:
                    currentFigure = 1;
                    break;
                case sf::Keyboard::Scancode::Num2:
                    currentFigure = 2;
                    break;
                case sf::Keyboard::Scancode::Num3:
                    currentFigure = 3;
                    break;
                case sf::Keyboard::Scancode::Num4:
                    currentFigure = 4;
                    break;

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
                    if (currentFigure == 4) {
                        scale[idx][0] = scale[idx][1] = scale[idx][2] = 1.0f;
                    }
                    break;

                case sf::Keyboard::Scancode::Up:
                    if (currentFigure == 2) {
                        colorMix += 0.05f;
                        if (colorMix > 1.0f) colorMix = 1.0f;
                    }
                    else if (currentFigure == 3) {
                        textureMix += 0.05f;
                        if (textureMix > 1.0f) textureMix = 1.0f;
                    }
                    else if (currentFigure == 4) {
                        scale[idx][0] += scaleSpeed;
                        scale[idx][1] += scaleSpeed;
                        scale[idx][2] += scaleSpeed;
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
                    else if (currentFigure == 4) {
                        scale[idx][0] -= scaleSpeed;
                        if (scale[idx][0] < 0.1f) scale[idx][0] = 0.1f;
                        scale[idx][1] -= scaleSpeed;
                        if (scale[idx][1] < 0.1f) scale[idx][1] = 0.1f;
                        scale[idx][2] -= scaleSpeed;
                        if (scale[idx][2] < 0.1f) scale[idx][2] = 0.1f;
                    }
                    break;

                case sf::Keyboard::Scancode::Num7:
                    if (currentFigure == 4) scale[idx][0] += scaleSpeed;
                    break;
                case sf::Keyboard::Scancode::Num8:
                    if (currentFigure == 4) scale[idx][0] -= scaleSpeed;
                    if (scale[idx][0] < 0.1f) scale[idx][0] = 0.1f;
                    break;
                case sf::Keyboard::Scancode::Num9:
                    if (currentFigure == 4) scale[idx][1] += scaleSpeed;
                    break;
                case sf::Keyboard::Scancode::Num0:
                    if (currentFigure == 4) scale[idx][1] -= scaleSpeed;
                    if (scale[idx][1] < 0.1f) scale[idx][1] = 0.1f;
                    break;

                default:
                    break;
                }
            }
        }

        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        angleX += 0.5f;
        angleY += 0.3f;
        angleZ += 0.2f;

        glm::mat4 dynamicRotation = glm::mat4(1.0f);
        dynamicRotation = glm::rotate(dynamicRotation, glm::radians(angleX), glm::vec3(1.0f, 0.0f, 0.0f));
        dynamicRotation = glm::rotate(dynamicRotation, glm::radians(angleY), glm::vec3(0.0f, 1.0f, 0.0f));
        dynamicRotation = glm::rotate(dynamicRotation, glm::radians(angleZ), glm::vec3(0.0f, 0.0f, 1.0f));

        if (currentFigure == 1) {
            glUseProgram(shaderProgram);
            glUniform3f(offsetLocation, offsets[0][0], offsets[0][1], offsets[0][2]);
            glUniformMatrix4fv(rotationLocation, 1, GL_FALSE, glm::value_ptr(dynamicRotation));

            glBindVertexArray(tetrahedronVAO);
            glDrawElements(GL_TRIANGLES, 12, GL_UNSIGNED_INT, 0);
            glBindVertexArray(0);
        }
        else if (currentFigure == 2) {
            glUseProgram(shaderTexColor);
            glUniformMatrix4fv(texColorRotationLoc, 1, GL_FALSE, glm::value_ptr(dynamicRotation));
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
            glUniformMatrix4fv(twoTexRotationLoc, 1, GL_FALSE, glm::value_ptr(dynamicRotation));
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
        else if (currentFigure == 4) {
            glUseProgram(shaderCircle);
            glUniform3f(circleOffsetLocation, offsets[3][0], offsets[3][1], offsets[3][2]);
            glUniform3f(circleScaleLocation, scale[3][0], scale[3][1], scale[3][2]);
            glUniformMatrix4fv(circleRotationLocation, 1, GL_FALSE, glm::value_ptr(dynamicRotation));

            glBindVertexArray(circleVAO);
            glDrawElements(GL_TRIANGLES, circleIndices.size(), GL_UNSIGNED_INT, 0);
            glBindVertexArray(0);
        }

        window.display();
    }

    glDeleteVertexArrays(1, &tetrahedronVAO);
    glDeleteBuffers(1, &tetrahedronVBO);
    glDeleteBuffers(1, &tetrahedronEBO);

    glDeleteVertexArrays(1, &circleVAO);
    glDeleteBuffers(1, &circleVBO);
    glDeleteBuffers(1, &circleEBO);

    glDeleteVertexArrays(1, &cubeVAO);
    glDeleteBuffers(1, &cubeVBO);
    glDeleteBuffers(1, &cubeEBO);

    glDeleteProgram(shaderProgram);
    glDeleteProgram(shaderCircle);
    glDeleteProgram(shaderTexColor);
    glDeleteProgram(shaderTwoTex);

    glDeleteTextures(1, &texture1);
    glDeleteTextures(1, &texture2);
    glDeleteTextures(1, &texture3);

    return 0;
}