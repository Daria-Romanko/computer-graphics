#include "model.h"
#include "model.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

static std::string GetDirectoryFromPath(const std::string& path)
{
    size_t slashPos = path.find_last_of("/\\");
    if (slashPos == std::string::npos) return ".";
    return path.substr(0, slashPos);
}

bool LoadOBJModel(const std::string& filename, Model& model)
{
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(
        filename,
        aiProcess_Triangulate |
        aiProcess_JoinIdenticalVertices |
        aiProcess_GenSmoothNormals |
        aiProcess_FlipUVs
    );

    if (!scene || !scene->mRootNode) {
        std::cerr << "ASSIMP: Failed to load " << filename << "\n"
            << importer.GetErrorString() << std::endl;
        return false;
    }

    model.vertices.clear();
    model.texCoords.clear();
    model.normals.clear();
    model.indices.clear();
    model.texture = 0;

    const aiMesh* mesh = scene->mMeshes[0];

    for (unsigned int i = 0; i < mesh->mNumVertices; ++i) {
        const aiVector3D& v = mesh->mVertices[i];
        model.vertices.emplace_back(v.x, v.y, v.z);

        glm::vec2 uv(0.0f);
        if (mesh->HasTextureCoords(0)) {
            const aiVector3D& t = mesh->mTextureCoords[0][i];
            uv = glm::vec2(t.x, t.y);
        }
        model.texCoords.push_back(uv);

        glm::vec3 n(0.0f, 0.0f, 1.0f);
        if (mesh->HasNormals()) {
            const aiVector3D& nn = mesh->mNormals[i];
            n = glm::vec3(nn.x, nn.y, nn.z);
        }
        model.normals.push_back(n);
    }

    for (unsigned int i = 0; i < mesh->mNumFaces; ++i) {
        const aiFace& face = mesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; ++j)
            model.indices.push_back(face.mIndices[j]);
    }

    model.indexCount = static_cast<int>(model.indices.size());

    std::cout << "Loaded model via Assimp: " << filename
        << " (vertices: " << model.vertices.size()
        << ", indices: " << model.indices.size() << ")\n";

    std::string directory = GetDirectoryFromPath(filename);

    if (mesh->mMaterialIndex >= 0 && mesh->mMaterialIndex < (int)scene->mNumMaterials) {
        aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

        if (material->GetTextureCount(aiTextureType_DIFFUSE) > 0) {
            aiString texPathAI;
            if (material->GetTexture(aiTextureType_DIFFUSE, 0, &texPathAI) == AI_SUCCESS) {
                std::string texRelPath = texPathAI.C_Str();
                std::string fullTexPath = directory + "/" + texRelPath;

                std::cout << "Trying texture from MTL: " << fullTexPath << "\n";
                model.texture = LoadTextureFromFile(fullTexPath);
                if (model.texture == 0) {
                    std::cerr << "Failed to load diffuse texture from MTL: "
                        << fullTexPath << std::endl;
                }
            }
        }
    }

    return true;
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
    vertexData.reserve(model.vertices.size() * 8);

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

        if (i < model.normals.size()) {
            vertexData.push_back(model.normals[i].x);
            vertexData.push_back(model.normals[i].y);
            vertexData.push_back(model.normals[i].z);
        }
        else {
            vertexData.push_back(0.0f);
            vertexData.push_back(0.0f);
            vertexData.push_back(1.0f);
        }
    }

    glBindBuffer(GL_ARRAY_BUFFER, model.vbo);
    glBufferData(GL_ARRAY_BUFFER, vertexData.size() * sizeof(float), vertexData.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, model.ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, model.indices.size() * sizeof(unsigned int), model.indices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(5 * sizeof(float)));
    glEnableVertexAttribArray(2);

    if (!textureFile.empty()) {
        if (model.texture != 0) {
            glDeleteTextures(1, &model.texture);
            model.texture = 0;
        }

        model.texture = LoadTextureFromFile(textureFile);
        if (model.texture == 0) {
            std::cout << "InitializeModelGL: failed to load texture '" << textureFile
                << "' for model " << model.name << std::endl;
        }
    }
    else {
        if (model.texture == 0) {
            std::cout << "InitializeModelGL: no texture file and no texture from MTL for model "
                << model.name << std::endl;
        }
    }

    model.indexCount = static_cast<int>(model.indices.size());

    glBindVertexArray(0);
    return true;
}

void DestroyModelGL(Model& model)
{
    if (model.vbo)     glDeleteBuffers(1, &model.vbo);
    if (model.ebo)     glDeleteBuffers(1, &model.ebo);
    if (model.vao)     glDeleteVertexArrays(1, &model.vao);
    if (model.texture) glDeleteTextures(1, &model.texture);

    model.vbo = model.ebo = model.vao = model.texture = 0;
    model.indexCount = 0;

    model.vertices.clear();
    model.texCoords.clear();
    model.indices.clear();
}

void DrawModel(const Model& model)
{
    if (model.vao == 0 || model.indexCount == 0)
        return;

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, model.texture);

    glBindVertexArray(model.vao);
    glDrawElements(GL_TRIANGLES, model.indexCount, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
}