#pragma once
#include <SFML/Graphics.hpp>
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <vector>
#include <iostream>
#include <fstream>
#include <string>

struct Model {
    std::vector<glm::vec3> vertices;
    std::vector<glm::vec2> texCoords;
    std::vector<unsigned int> indices;

    GLuint vao = 0;
    GLuint vbo = 0;
    GLuint ebo = 0;
    GLuint texture = 0;

    int indexCount = 0;
    std::string name;
};

bool LoadOBJModel(const std::string& filename, Model& model);
GLuint LoadTextureFromFile(const std::string& filename);
bool InitializeModelGL(Model& model, const std::string& textureFile = "");
