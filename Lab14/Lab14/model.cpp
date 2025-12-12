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

static std::string ExtractFileName(const std::string& path)
{
    size_t slash1 = path.find_last_of("/\\");
    if (slash1 != std::string::npos)
        return path.substr(slash1 + 1);
    return path;
}

static GLuint LoadMaterialTexture(aiMaterial* material, const std::string& directory, const std::string& objBaseName)
{
    aiString texPathAI;

    if (material->GetTextureCount(aiTextureType_DIFFUSE) > 0 &&
        material->GetTexture(aiTextureType_DIFFUSE, 0, &texPathAI) == AI_SUCCESS)
    {
        std::string rawPath = texPathAI.C_Str();
        std::string fileName = ExtractFileName(rawPath);
        std::string fullPath = directory + "/" + fileName;

        GLuint tex = LoadTextureFromFile(fullPath);
        if (tex != 0)
            return tex;

        std::cerr << "Warning: could not load texture: " << fullPath << "\n";
    }

    static const char* exts[] = { ".png", ".jpg", ".jpeg", ".bmp", ".tga" };

    for (const char* ext : exts)
    {
        std::string fallback = directory + "/" + objBaseName + ext;
        GLuint tex = LoadTextureFromFile(fallback);
        if (tex != 0) {
            std::cout << "Loaded fallback texture: " << fallback << "\n";
            return tex;
        }
    }

    return 0;
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
    model.subMeshes.clear();

    std::string directory = GetDirectoryFromPath(filename);
    std::string fileOnly = filename.substr(filename.find_last_of("/\\") + 1);
    std::string baseName = fileOnly.substr(0, fileOnly.find_last_of('.'));

    size_t vertexOffset = 0;

    for (unsigned int m = 0; m < scene->mNumMeshes; m++)
    {
        aiMesh* mesh = scene->mMeshes[m];

        SubMesh sub;
        sub.indexOffset = model.indices.size();

        for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
            model.vertices.emplace_back(mesh->mVertices[i].x,
                mesh->mVertices[i].y,
                mesh->mVertices[i].z);

            if (mesh->HasTextureCoords(0)) {
                aiVector3D uv = mesh->mTextureCoords[0][i];
                model.texCoords.emplace_back(uv.x, uv.y);
            }
            else {
                model.texCoords.emplace_back(0.f, 0.f);
            }

            if (mesh->HasNormals()) {
                aiVector3D n = mesh->mNormals[i];
                model.normals.emplace_back(n.x, n.y, n.z);
            }
            else {
                model.normals.emplace_back(0.f, 0.f, 1.f);
            }
        }

        for (unsigned int f = 0; f < mesh->mNumFaces; f++) {
            aiFace face = mesh->mFaces[f];
            for (unsigned int j = 0; j < face.mNumIndices; j++)
                model.indices.push_back(face.mIndices[j] + vertexOffset);
        }

        vertexOffset += mesh->mNumVertices;

        sub.indexCount = model.indices.size() - sub.indexOffset;

        aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
        sub.texture = LoadMaterialTexture(material, directory, baseName);

        model.subMeshes.push_back(sub);
    }

    model.indexCount = model.indices.size();

    std::cout << "Loaded OBJ with " << model.subMeshes.size()
        << " materials, " << model.vertices.size()
        << " vertices, " << model.indices.size() << " indices.\n";

    return true;
}

GLuint LoadTextureFromFile(const std::string& filename)
{
    sf::Image img;
    if (!img.loadFromFile(filename)) {
        std::cerr << "Failed to load texture: " << filename << std::endl;
        return 0;
    }

    GLuint tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);

    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RGBA,
        img.getSize().x,
        img.getSize().y,
        0,
        GL_RGBA,
        GL_UNSIGNED_BYTE,
        img.getPixelsPtr()
    );

    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    std::cout << "Loaded texture: " << filename << std::endl;

    return tex;
}

bool InitializeModelGL(Model& model, const std::string& texFile)
{
    glGenVertexArrays(1, &model.vao);
    glGenBuffers(1, &model.vbo);
    glGenBuffers(1, &model.ebo);

    glBindVertexArray(model.vao);

    std::vector<float> vert;
    vert.reserve(model.vertices.size() * 8);

    for (size_t i = 0; i < model.vertices.size(); i++)
    {
        vert.push_back(model.vertices[i].x);
        vert.push_back(model.vertices[i].y);
        vert.push_back(model.vertices[i].z);

        vert.push_back(model.texCoords[i].x);
        vert.push_back(model.texCoords[i].y);

        vert.push_back(model.normals[i].x);
        vert.push_back(model.normals[i].y);
        vert.push_back(model.normals[i].z);
    }

    glBindBuffer(GL_ARRAY_BUFFER, model.vbo);
    glBufferData(GL_ARRAY_BUFFER, vert.size() * sizeof(float), vert.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, model.ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, model.indices.size() * sizeof(unsigned int), model.indices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(5 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);
    return true;
}

void DestroyModelGL(Model& model)
{
    for (auto& sm : model.subMeshes) {
        if (sm.texture)
            glDeleteTextures(1, &sm.texture);
    }

    if (model.vbo) glDeleteBuffers(1, &model.vbo);
    if (model.ebo) glDeleteBuffers(1, &model.ebo);
    if (model.vao) glDeleteVertexArrays(1, &model.vao);

    model.vbo = model.ebo = model.vao = 0;
}

void DrawModel(const Model& model)
{
    glBindVertexArray(model.vao);

    for (const SubMesh& sm : model.subMeshes)
    {
        glBindTexture(GL_TEXTURE_2D, sm.texture);
        glDrawElements(
            GL_TRIANGLES,
            sm.indexCount,
            GL_UNSIGNED_INT,
            (void*)(sm.indexOffset * sizeof(unsigned int))
        );
    }

    glBindVertexArray(0);
}
