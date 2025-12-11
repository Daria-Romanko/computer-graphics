#include "model.h"

bool LoadOBJModel(const std::string& filename, Model& model)
{
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cout << "Failed to open file: " << filename << std::endl;
        return false;
    }

    std::vector<glm::vec3> tempVertices;
    std::vector<glm::vec2> tempTexCoords;
    std::vector<unsigned int> vertexIndices, texIndices;

    std::string line;
    while (std::getline(file, line)) {
        if (line.substr(0, 2) == "v ") {
            glm::vec3 vertex;
            sscanf_s(line.c_str(), "v %f %f %f", &vertex.x, &vertex.y, &vertex.z);
            tempVertices.push_back(vertex);
        }
        else if (line.substr(0, 3) == "vt ") {
            glm::vec2 texCoord;
            sscanf_s(line.c_str(), "vt %f %f", &texCoord.x, &texCoord.y);
            tempTexCoords.push_back(texCoord);
        }
        else if (line.substr(0, 2) == "f ") {
            unsigned int vertexIndex[3], texIndex[3];
            int matches = sscanf_s(line.c_str(), "f %d/%d %d/%d %d/%d",
                &vertexIndex[0], &texIndex[0],
                &vertexIndex[1], &texIndex[1],
                &vertexIndex[2], &texIndex[2]);

            if (matches == 6) {
                for (int i = 0; i < 3; i++) {
                    vertexIndices.push_back(vertexIndex[i] - 1);
                    texIndices.push_back(texIndex[i] - 1);
                }
            }
            else {
                matches = sscanf_s(line.c_str(), "f %d// %d// %d//",
                    &vertexIndex[0], &vertexIndex[1], &vertexIndex[2]);
                if (matches == 3) {
                    for (int i = 0; i < 3; i++) {
                        vertexIndices.push_back(vertexIndex[i] - 1);
                        texIndices.push_back(0);
                    }
                }
            }
        }
    }

    file.close();

    for (size_t i = 0; i < vertexIndices.size(); i++) {
        unsigned int vertexIndex = vertexIndices[i];
        unsigned int texIndex = (i < texIndices.size()) ? texIndices[i] : 0;

        if (vertexIndex < tempVertices.size()) {
            model.vertices.push_back(tempVertices[vertexIndex]);
        }

        if (texIndex < tempTexCoords.size()) {
            model.texCoords.push_back(tempTexCoords[texIndex]);
        }
        else {
            model.texCoords.push_back(glm::vec2(0, 0));
        }

        model.indices.push_back(i);
    }

    model.indexCount = model.indices.size();

    std::cout << "Loaded model: " << filename << " (vertices: " << model.vertices.size()  << ", indices: " << model.indices.size() << ")" << std::endl;

    return !model.vertices.empty();
}

GLuint LoadTextureFromFile(const std::string& filename)
{
    sf::Image image;
    if (!image.loadFromFile(filename)) {
        std::cout << "Failed to load texture: " << filename << std::endl;
        return 0;
    }

    const std::uint8_t* pixels = image.getPixelsPtr();
    unsigned int width = image.getSize().x;
    unsigned int height = image.getSize().y;

    GLuint texture = 0;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glGenerateMipmap(GL_TEXTURE_2D);

    std::cout << "Loaded texture: " << filename << " (" << width << "x" << height << ")" << std::endl;

    return texture;
}

bool InitializeModelGL(Model& model, const std::string& textureFile)
{
    glGenVertexArrays(1, &model.vao);
    glGenBuffers(1, &model.vbo);
    glGenBuffers(1, &model.ebo);

    glBindVertexArray(model.vao);

    std::vector<float> vertexData;
    vertexData.reserve(model.vertices.size() * 5);

    for (size_t i = 0; i < model.vertices.size(); i++) {
        vertexData.push_back(model.vertices[i].x);
        vertexData.push_back(model.vertices[i].y);
        vertexData.push_back(model.vertices[i].z);

        if (i < model.texCoords.size()) {
            vertexData.push_back(model.texCoords[i].x);
            vertexData.push_back(model.texCoords[i].y);
        }
        else {
            vertexData.push_back(0.0f);
            vertexData.push_back(0.0f);
        }
    }

    glBindBuffer(GL_ARRAY_BUFFER, model.vbo);
    glBufferData(GL_ARRAY_BUFFER, vertexData.size() * sizeof(float), vertexData.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, model.ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, model.indices.size() * sizeof(unsigned int), model.indices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    if (textureFile.empty()) {
        std::cout << "InitializeModelGL: texture file is empty for model " << model.name << std::endl;
        glBindVertexArray(0);
        return false;
    }

    model.texture = LoadTextureFromFile(textureFile);
    if (model.texture == 0) {
        std::cout << "InitializeModelGL: failed to load texture '" << textureFile
            << "' for model " << model.name << std::endl;
        glBindVertexArray(0);
        return false;
    }

    glBindVertexArray(0);
    return true;
}