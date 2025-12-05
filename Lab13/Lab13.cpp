#include <GL/glew.h>
#include <SFML/OpenGL.hpp>
#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <iostream>
#include <vector>
#include <fstream>
#include <cmath>
#include <cstring>
#include <cstdint>

struct Vec3 {
    float x, y, z;
    Vec3() : x(0), y(0), z(0) {}
    Vec3(float x, float y, float z) : x(x), y(y), z(z) {}

    Vec3 operator+(const Vec3& other) const {
        return Vec3(x + other.x, y + other.y, z + other.z);
    }

    Vec3 operator-(const Vec3& other) const {
        return Vec3(x - other.x, y - other.y, z - other.z);
    }

    Vec3 operator*(float scalar) const {
        return Vec3(x * scalar, y * scalar, z * scalar);
    }

    float length() const {
        return std::sqrt(x * x + y * y + z * z);
    }

    Vec3 normalize() const {
        float len = length();
        if (len > 0) {
            return Vec3(x / len, y / len, z / len);
        }
        return Vec3(0, 0, 0);
    }

    static Vec3 cross(const Vec3& a, const Vec3& b) {
        return Vec3(
            a.y * b.z - a.z * b.y,
            a.z * b.x - a.x * b.z,
            a.x * b.y - a.y * b.x
        );
    }

    static float dot(const Vec3& a, const Vec3& b) {
        return a.x * b.x + a.y * b.y + a.z * b.z;
    }
};

struct Vec2 {
    float x, y;
    Vec2() : x(0), y(0) {}
    Vec2(float x, float y) : x(x), y(y) {}
};

// Матрица 4x4
struct Mat4 {
    float m[16]; // Храним в column-major порядке для OpenGL

    Mat4() {
        identity();
    }

    void identity() {
        std::memset(m, 0, sizeof(m));
        m[0] = m[5] = m[10] = m[15] = 1.0f;
    }

    static Mat4 perspective(float fov, float aspect, float znear, float zfar) {
        Mat4 result;
        float f = 1.0f / std::tan(fov * 0.5f * 3.14159265f / 180.0f);

        result.m[0] = f / aspect;
        result.m[1] = 0;
        result.m[2] = 0;
        result.m[3] = 0;

        result.m[4] = 0;
        result.m[5] = f;
        result.m[6] = 0;
        result.m[7] = 0;

        result.m[8] = 0;
        result.m[9] = 0;
        result.m[10] = (zfar + znear) / (zfar - znear);
        result.m[11] = -1.0f;

        result.m[12] = 0;
        result.m[13] = 0;
        result.m[14] = 2.0f * zfar * znear / (zfar - znear);
        result.m[15] = 0;

        return result;
    }

    static Mat4 lookAt(const Vec3& eye, const Vec3& center, const Vec3& up) {
        Mat4 result;

        Vec3 f = (center - eye).normalize();
        Vec3 u = up.normalize();
        Vec3 s = Vec3::cross(f, u).normalize();
        u = Vec3::cross(s, f);

        result.m[0] = s.x;
        result.m[1] = u.x;
        result.m[2] = -f.x;
        result.m[3] = 0.0f;

        result.m[4] = s.y;
        result.m[5] = u.y;
        result.m[6] = -f.y;
        result.m[7] = 0.0f;

        result.m[8] = s.z;
        result.m[9] = u.z;
        result.m[10] = -f.z;
        result.m[11] = 0.0f;

        result.m[12] = -Vec3::dot(s, eye);
        result.m[13] = -Vec3::dot(u, eye);
        result.m[14] = Vec3::dot(f, eye);
        result.m[15] = 1.0f;

        return result;
    }

    static Mat4 translate(float x, float y, float z) {
        Mat4 result;
        result.identity();
        result.m[12] = x;
        result.m[13] = y;
        result.m[14] = z;
        return result;
    }

    static Mat4 scale(float x, float y, float z) {
        Mat4 result;
        result.identity();
        result.m[0] = x;
        result.m[5] = y;
        result.m[10] = z;
        return result;
    }

    static Mat4 rotateY(float angle) {
        Mat4 result;
        result.identity();
        float c = std::cos(angle);
        float s = std::sin(angle);

        result.m[0] = c;
        result.m[2] = s;
        result.m[8] = -s;
        result.m[10] = c;

        return result;
    }

    static Mat4 rotateX(float angle) {
        Mat4 result;
        result.identity();
        float c = std::cos(angle);
        float s = std::sin(angle);

        result.m[5] = c;
        result.m[6] = -s;
        result.m[9] = s;
        result.m[10] = c;

        return result;
    }

    Mat4 operator*(const Mat4& other) const {
        Mat4 result;

        for (int i = 0; i < 4; ++i) {
            for (int j = 0; j < 4; ++j) {
                result.m[i * 4 + j] = 0.0f;
                for (int k = 0; k < 4; ++k) {
                    result.m[i * 4 + j] += m[i * 4 + k] * other.m[k * 4 + j];
                }
            }
        }

        return result;
    }
};

// Структура для хранения данных модели OBJ
struct Model {
    std::vector<Vec3> vertices;
    std::vector<Vec2> texCoords;
    std::vector<unsigned int> indices;

    GLuint vao = 0;
    GLuint vbo = 0;
    GLuint ebo = 0;
    GLuint texture = 0;

    int indexCount = 0;
    std::string name; 
};

// Функция для загрузки модели OBJ
bool LoadOBJModel(const std::string& filename, Model& model) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cout << "Failed to open file: " << filename << std::endl;
        return false;
    }

    std::vector<Vec3> tempVertices;
    std::vector<Vec2> tempTexCoords;
    std::vector<unsigned int> vertexIndices, texIndices;

    std::string line;
    while (std::getline(file, line)) {
        if (line.substr(0, 2) == "v ") {
            // Вершина
            Vec3 vertex;
            sscanf_s(line.c_str(), "v %f %f %f", &vertex.x, &vertex.y, &vertex.z);
            tempVertices.push_back(vertex);
        }
        else if (line.substr(0, 3) == "vt ") {
            // Текстурная координата
            Vec2 texCoord;
            sscanf_s(line.c_str(), "vt %f %f", &texCoord.x, &texCoord.y);
            tempTexCoords.push_back(texCoord);
        }
        else if (line.substr(0, 2) == "f ") {
            // Полигон (упрощенная обработка только для треугольников)
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
                // Пробуем другой формат
                matches = sscanf_s(line.c_str(), "f %d// %d// %d//",
                    &vertexIndex[0], &vertexIndex[1], &vertexIndex[2]);
                if (matches == 3) {
                    for (int i = 0; i < 3; i++) {
                        vertexIndices.push_back(vertexIndex[i] - 1);
                        texIndices.push_back(0); // Пустые текстурные координаты
                    }
                }
            }
        }
    }

    file.close();

    // Заполняем данные модели
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
            model.texCoords.push_back(Vec2(0, 0));
        }

        model.indices.push_back(i);
    }

    model.indexCount = model.indices.size();

    std::cout << "Loaded model: " << filename
        << " (vertices: " << model.vertices.size()
        << ", indices: " << model.indices.size() << ")" << std::endl;

    return !model.vertices.empty();
}

// Функция для загрузки текстуры из файла JPG
GLuint LoadTextureFromFile(const std::string& filename) {
    // Загружаем изображение с помощью SFML
    sf::Image image;
    if (!image.loadFromFile(filename)) {
        std::cout << "Failed to load texture: " << filename << std::endl;

        const int width = 64;
        const int height = 64;
        std::uint8_t pixels[width * height * 4];

        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                int index = (y * width + x) * 4;

                pixels[index] = 220 + (rand() % 35);     // R
                pixels[index + 1] = 20 + (rand() % 30);  // G
                pixels[index + 2] = 20 + (rand() % 30);  // B
                pixels[index + 3] = 255;                 // A
            }
        }

        GLuint texture;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

        return texture;
    }

    // Получаем данные изображения
    const std::uint8_t* pixels = image.getPixelsPtr(); // Используем std::uint8_t
    unsigned int width = image.getSize().x;
    unsigned int height = image.getSize().y;

    // Конвертируем из RGBA (формат SFML) в формат, понятный OpenGL
    // SFML хранит пиксели в RGBA порядке, что совместимо с GL_RGBA

    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    // Загружаем текстуру в OpenGL
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

    // Настраиваем параметры текстуры
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    // Генерируем mipmaps для лучшего качества
    glGenerateMipmap(GL_TEXTURE_2D);

    std::cout << "Loaded texture: " << filename
        << " (" << width << "x" << height << ")" << std::endl;

    return texture;
}

// Функция для инициализации модели в OpenGL
bool InitializeModelGL(Model& model, const std::string& textureFile = "") {
    // Создаем VAO, VBO и EBO
    glGenVertexArrays(1, &model.vao);
    glGenBuffers(1, &model.vbo);
    glGenBuffers(1, &model.ebo);

    glBindVertexArray(model.vao);

    // Копируем данные вершин и текстурных координат в один буфер
    std::vector<float> vertexData;
    vertexData.reserve(model.vertices.size() * 5);

    for (size_t i = 0; i < model.vertices.size(); i++) {
        // Позиция вершины
        vertexData.push_back(model.vertices[i].x);
        vertexData.push_back(model.vertices[i].y);
        vertexData.push_back(model.vertices[i].z);

        // Текстурные координаты
        if (i < model.texCoords.size()) {
            vertexData.push_back(model.texCoords[i].x);
            vertexData.push_back(model.texCoords[i].y);
        }
        else {
            vertexData.push_back(0.0f);
            vertexData.push_back(0.0f);
        }
    }

    // Загружаем данные вершин
    glBindBuffer(GL_ARRAY_BUFFER, model.vbo);
    glBufferData(GL_ARRAY_BUFFER, vertexData.size() * sizeof(float), vertexData.data(), GL_STATIC_DRAW);

    // Загружаем индексы
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, model.ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, model.indices.size() * sizeof(unsigned int),
        model.indices.data(), GL_STATIC_DRAW);

    // Настраиваем атрибуты вершин
    // Атрибут позиции (0)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Атрибут текстурных координат (1)
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // Создаем текстуру
    if (!textureFile.empty()) {
        model.texture = LoadTextureFromFile(textureFile);
    }
    else {
        // Если имя файла текстуры не указано, создаем текстуру по умолчанию
        model.texture = LoadTextureFromFile("default_texture.jpg");
    }

    glBindVertexArray(0);

    return true;
}

// Вершинный шейдер
const char* vertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;

out vec2 TexCoord;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    gl_Position = projection * view * model * vec4(aPos, 1.0);
    TexCoord = aTexCoord;
}
)";

// Фрагментный шейдер
const char* fragmentShaderSource = R"(
#version 330 core
in vec2 TexCoord;
out vec4 FragColor;

uniform sampler2D texture1;

void main()
{
    FragColor = texture(texture1, TexCoord);
}
)";

// Компиляция шейдера
GLuint CompileShader(GLenum type, const char* source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);

    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        std::cout << "Shader compilation failed: " << infoLog << std::endl;
        return 0;
    }

    return shader;
}

// Создание шейдерной программы
GLuint CreateShaderProgram() {
    GLuint vertexShader = CompileShader(GL_VERTEX_SHADER, vertexShaderSource);
    GLuint fragmentShader = CompileShader(GL_FRAGMENT_SHADER, fragmentShaderSource);

    if (!vertexShader || !fragmentShader) {
        return 0;
    }

    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    GLint success;
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cout << "Shader program linking failed: " << infoLog << std::endl;
        return 0;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return shaderProgram;
}

// Функция для установки uniform матрицы
void SetUniformMatrix4(GLuint program, const char* name, const Mat4& matrix) {
    GLint location = glGetUniformLocation(program, name);
    if (location != -1) {
        glUniformMatrix4fv(location, 1, GL_FALSE, matrix.m);
    }
}

int main() {
    sf::Window window(sf::VideoMode({ 800, 600 }), "Solar System: Oil Drum Center with Fire Extinguishers", sf::Style::Default);
    window.setFramerateLimit(60);

    // Инициализируем GLEW
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        std::cerr << "Failed to initialize GLEW" << std::endl;
        return -1;
    }

    // Настраиваем OpenGL
    glEnable(GL_DEPTH_TEST);

    // Создаем шейдерную программу
    GLuint shaderProgram = CreateShaderProgram();
    if (!shaderProgram) {
        return -1;
    }

    // Загружаем центральную модель - банку
    Model centralOilDrum;
    centralOilDrum.name = "Central Oil Drum";

    std::cout << "\n=== Loading central oil drum model ===" << std::endl;
    if (LoadOBJModel("oil-drum_col.obj", centralOilDrum)) {
        if (InitializeModelGL(centralOilDrum, "oil-drum_col_texture.jpg")) {
            std::cout << "Successfully loaded central oil drum model" << std::endl;
        }
        else {
            std::cout << "Failed to initialize central oil drum model" << std::endl;
            centralOilDrum = Model(); // Сброс модели
        }
    }
    else {
        std::cout << "Failed to load oil-drum_col.obj. Creating placeholder..." << std::endl;
        // Создаем простую модель бочки-заглушки
        int segments = 16;
        float radius = 1.0f;
        float height = 2.0f;

        // Вершины для цилиндрической бочки
        for (int j = 0; j <= segments; j++) {
            float angle = 2.0f * 3.14159265f * j / segments;
            float x = radius * cos(angle);
            float z = radius * sin(angle);

            // Верхнее кольцо
            centralOilDrum.vertices.push_back(Vec3(x, height / 2, z));
            // Нижнее кольцо
            centralOilDrum.vertices.push_back(Vec3(x, -height / 2, z));

            // Текстурные координаты
            centralOilDrum.texCoords.push_back(Vec2(j / (float)segments, 0));
            centralOilDrum.texCoords.push_back(Vec2(j / (float)segments, 1));
        }

        // Индексы для боковой поверхности
        for (int j = 0; j < segments; j++) {
            int base = j * 2;
            centralOilDrum.indices.push_back(base);
            centralOilDrum.indices.push_back(base + 1);
            centralOilDrum.indices.push_back(base + 2);

            centralOilDrum.indices.push_back(base + 1);
            centralOilDrum.indices.push_back(base + 3);
            centralOilDrum.indices.push_back(base + 2);
        }

        // Верхняя и нижняя крышки
        int centerTop = centralOilDrum.vertices.size();
        centralOilDrum.vertices.push_back(Vec3(0, height / 2, 0));
        centralOilDrum.texCoords.push_back(Vec2(0.5, 0.5));

        int centerBottom = centralOilDrum.vertices.size();
        centralOilDrum.vertices.push_back(Vec3(0, -height / 2, 0));
        centralOilDrum.texCoords.push_back(Vec2(0.5, 0.5));

        for (int j = 0; j < segments; j++) {
            int v1 = j * 2;
            int v2 = ((j + 1) % segments) * 2;

            // Верхняя крышка
            centralOilDrum.indices.push_back(centerTop);
            centralOilDrum.indices.push_back(v1);
            centralOilDrum.indices.push_back(v2);

            // Нижняя крышка
            centralOilDrum.indices.push_back(centerBottom);
            centralOilDrum.indices.push_back(v1 + 1);
            centralOilDrum.indices.push_back(v2 + 1);
        }

        centralOilDrum.indexCount = centralOilDrum.indices.size();

        // Инициализируем с текстурой
        if (InitializeModelGL(centralOilDrum, "oil-drum_col_texture.jpg")) {
            std::cout << "Created placeholder oil drum model" << std::endl;
        }
        else {
            std::cout << "Failed to create placeholder oil drum model" << std::endl;
            centralOilDrum = Model();
        }
    }

    // Создаем модели огнетушителей (орбитальные объекты)
    std::vector<Model> fireExtinguisherModels;

    std::cout << "\n=== Loading fire extinguisher models ===" << std::endl;
    // Загружаем 5 экземпляров модели огнетушителя (из одного файла)
    for (int i = 0; i < 5; i++) {
        Model fireExtinguisher;
        fireExtinguisher.name = "Fire Extinguisher " + std::to_string(i + 1);

        if (LoadOBJModel("fire_extinguisher.obj", fireExtinguisher)) {
            // Инициализируем модель с текстурой
            if (InitializeModelGL(fireExtinguisher, "fire_extinguisher_texture.jpg")) {
                fireExtinguisherModels.push_back(fireExtinguisher);
                std::cout << "Loaded fire extinguisher model " << i + 1 << " with "
                    << fireExtinguisher.vertices.size() << " vertices" << std::endl;
            }
        }
        else {
            // Если не удалось загрузить файл, создаем простую модель-заглушку
            std::cout << "Failed to load fire_extinguisher.obj. Creating placeholder model..." << std::endl;

            // Создаем простую цилиндрическую модель огнетушителя
            Model placeholder;
            placeholder.name = "Placeholder Fire Extinguisher " + std::to_string(i + 1);

            // Создаем простой цилиндр
            int segments = 12;
            float radius = 0.3f;
            float height = 1.5f;

            // Вершины
            for (int j = 0; j <= segments; j++) {
                float angle = 2.0f * 3.14159265f * j / segments;
                float x = radius * cos(angle);
                float z = radius * sin(angle);

                // Верхнее кольцо
                placeholder.vertices.push_back(Vec3(x, height / 2, z));
                // Нижнее кольцо
                placeholder.vertices.push_back(Vec3(x, -height / 2, z));

                // Текстурные координаты
                placeholder.texCoords.push_back(Vec2(j / (float)segments, 0));
                placeholder.texCoords.push_back(Vec2(j / (float)segments, 1));
            }

            // Индексы для боковой поверхности
            for (int j = 0; j < segments; j++) {
                int base = j * 2;
                placeholder.indices.push_back(base);
                placeholder.indices.push_back(base + 1);
                placeholder.indices.push_back(base + 2);

                placeholder.indices.push_back(base + 1);
                placeholder.indices.push_back(base + 3);
                placeholder.indices.push_back(base + 2);
            }

            // Верхняя и нижняя крышки
            int centerTop = placeholder.vertices.size();
            placeholder.vertices.push_back(Vec3(0, height / 2, 0));
            placeholder.texCoords.push_back(Vec2(0.5, 0.5));

            int centerBottom = placeholder.vertices.size();
            placeholder.vertices.push_back(Vec3(0, -height / 2, 0));
            placeholder.texCoords.push_back(Vec2(0.5, 0.5));

            for (int j = 0; j < segments; j++) {
                int v1 = j * 2;
                int v2 = ((j + 1) % segments) * 2;

                // Верхняя крышка
                placeholder.indices.push_back(centerTop);
                placeholder.indices.push_back(v1);
                placeholder.indices.push_back(v2);

                // Нижняя крышка
                placeholder.indices.push_back(centerBottom);
                placeholder.indices.push_back(v1 + 1);
                placeholder.indices.push_back(v2 + 1);
            }

            placeholder.indexCount = placeholder.indices.size();

            // Инициализируем с текстурой
            if (InitializeModelGL(placeholder, "fire_extinguisher_texture.jpg")) {
                fireExtinguisherModels.push_back(placeholder);
            }
        }
    }

    if (fireExtinguisherModels.empty()) {
        std::cerr << "Failed to create fire extinguisher models" << std::endl;
        // Не выходим, потому что у нас есть центральная модель
    }

    // Создаем 100+ "планет" (огнетушителей) с разными позициями
    const int NUM_PLANETS = 100;
    std::vector<Vec3> planetPositions;
    std::vector<float> planetRotations;
    std::vector<int> planetModelIndices;

    for (int i = 0; i < NUM_PLANETS; i++) {
        // Случайные позиции в эллиптических орбитах вокруг центральной бочки
        float radius = 8.0f + (i % 15) * 1.5f;
        float angle = (i * 137.5f) * 3.14159265f / 180.0f;
        float height = std::sin(i * 0.3f) * 2.0f;

        planetPositions.push_back(Vec3(
            radius * std::cos(angle),
            height,
            radius * std::sin(angle)
        ));

        planetRotations.push_back(0.0f);
        if (!fireExtinguisherModels.empty()) {
            planetModelIndices.push_back(i % fireExtinguisherModels.size());
        }
        else {
            planetModelIndices.push_back(0);
        }
    }

    std::cout << "\n=== Starting simulation ===" << std::endl;
    std::cout << "Central object: Oil Drum" << std::endl;
    std::cout << "Orbiting objects: " << NUM_PLANETS << " fire extinguishers" << std::endl;
    std::cout << "Close window or press ESC to exit" << std::endl;

    // Основной цикл
    sf::Clock clock;
    bool running = true;

    while (running) {
        // Обработка событий
        while (const std::optional<sf::Event> event = window.pollEvent()) {
            if (event) {
                if (event->is<sf::Event::Closed>()) {
                    running = false;
                }
                else if (const auto* keyEvent = event->getIf<sf::Event::KeyPressed>()) {
                    if (keyEvent->scancode == sf::Keyboard::Scancode::Escape) {
                        running = false;
                    }
                }
            }
        }

        // Очистка буферов
        glClearColor(0.0f, 0.0f, 0.05f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Используем шейдерную программу
        glUseProgram(shaderProgram);

        // Настраиваем матрицы вида и проекции
        float time = clock.getElapsedTime().asSeconds();

        // Камера вращается вокруг всей сцены
        float cameraRadius = 30.0f;
        Vec3 cameraPos(
            cameraRadius * std::sin(time * 0.08f),
            8.0f,
            cameraRadius * std::cos(time * 0.08f)
        );

        // Камера смотрит на центральную бочку
        Vec3 center(0, 0, 0);
        Mat4 view = Mat4::lookAt(cameraPos, center, Vec3(0, 1, 0));
        Mat4 projection = Mat4::perspective(45.0f, 800.0f / 600.0f, 0.1f, 200.0f);

        // Устанавливаем view и projection матрицы
        SetUniformMatrix4(shaderProgram, "view", view);
        SetUniformMatrix4(shaderProgram, "projection", projection);

        // === РИСУЕМ ЦЕНТРАЛЬНУЮ БОЧКУ ===
        if (centralOilDrum.vao != 0) {
            // Привязываем VAO модели бочки
            glBindVertexArray(centralOilDrum.vao);

            // Активируем текстуру
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, centralOilDrum.texture);

            // Создаем трансформацию для центральной бочки
            Mat4 modelMat;

            // Бочка стоит в центре
            modelMat = Mat4::translate(0, -1.0f, 0);

            // Медленное вращение бочки вокруг своей оси
            Mat4 rotation = Mat4::rotateY(time * 0.2f);
            modelMat = rotation * modelMat;

            // Масштаб бочки (больше, чем огнетушители)
            Mat4 scale = Mat4::scale(2.0f, 2.0f, 2.0f);
            modelMat = modelMat * scale;

            // Устанавливаем матрицу model
            SetUniformMatrix4(shaderProgram, "model", modelMat);

            // Рисуем модель
            glDrawElements(GL_TRIANGLES, centralOilDrum.indexCount,
                GL_UNSIGNED_INT, 0);
        }

        // === РИСУЕМ 5 БЛИЖАЙШИХ ОГНЕТУШИТЕЛЕЙ (ВНУТРЕННЯЯ ОРБИТА) ===
        if (!fireExtinguisherModels.empty()) {
            for (int instance = 0; instance < 5; instance++) {
                for (size_t modelIndex = 0; modelIndex < std::min(fireExtinguisherModels.size(), (size_t)2); modelIndex++) {
                    // Привязываем VAO модели огнетушителя
                    glBindVertexArray(fireExtinguisherModels[modelIndex].vao);

                    // Активируем текстуру
                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, fireExtinguisherModels[modelIndex].texture);

                    // Создаем трансформацию для этого экземпляра
                    Mat4 modelMat;

                    // Позиционируем экземпляр по кругу (ближняя орбита вокруг бочки)
                    float angle = (instance * 72.0f + modelIndex * 36.0f) * 3.14159265f / 180.0f;
                    float radius = 5.0f; // Ближе к центру

                    modelMat = Mat4::translate(
                        radius * std::cos(angle + time * 0.5f),
                        0,
                        radius * std::sin(angle + time * 0.5f)
                    );

                    // Вращение вокруг своей оси
                    Mat4 rotation = Mat4::rotateY(time * 1.0f + instance * 0.3f);
                    modelMat = rotation * modelMat;

                    // Масштаб (больше для ближних моделей)
                    Mat4 scale = Mat4::scale(0.8f, 0.8f, 0.8f);
                    modelMat = modelMat * scale;

                    // Устанавливаем матрицу model
                    SetUniformMatrix4(shaderProgram, "model", modelMat);

                    // Рисуем модель
                    glDrawElements(GL_TRIANGLES, fireExtinguisherModels[modelIndex].indexCount,
                        GL_UNSIGNED_INT, 0);
                }
            }
        }

        // === РИСУЕМ 100+ "ПЛАНЕТ" (ОГНЕТУШИТЕЛЕЙ НА ВНЕШНИХ ОРБИТАХ) ===
        if (!fireExtinguisherModels.empty()) {
            for (int i = 0; i < NUM_PLANETS; i++) {
                int modelIndex = planetModelIndices[i];

                // Привязываем VAO модели
                glBindVertexArray(fireExtinguisherModels[modelIndex].vao);

                // Активируем текстуру
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, fireExtinguisherModels[modelIndex].texture);

                // Обновляем вращение
                planetRotations[i] += 0.005f * (i % 10 + 1);

                // Создаем трансформацию для планеты
                Mat4 modelMat;

                // Позиция (орбитальное движение вокруг центральной бочки)
                float orbitSpeed = 0.01f * ((i % 7) + 1);
                Vec3 pos = planetPositions[i];

                // Преобразуем позицию через матрицу орбиты
                float x = pos.x * std::cos(time * orbitSpeed) - pos.z * std::sin(time * orbitSpeed);
                float z = pos.x * std::sin(time * orbitSpeed) + pos.z * std::cos(time * orbitSpeed);

                modelMat = Mat4::translate(x, pos.y, z);

                // Вращение вокруг своей оси
                Mat4 rotation = Mat4::rotateY(planetRotations[i]);
                modelMat = rotation * modelMat;

                // Масштаб (случайный размер для разнообразия)
                float scaleFactor = 0.15f + 0.08f * (i % 10);
                Mat4 scale = Mat4::scale(scaleFactor, scaleFactor, scaleFactor);
                modelMat = modelMat * scale;

                // Устанавливаем матрицу model
                SetUniformMatrix4(shaderProgram, "model", modelMat);

                // Рисуем модель
                glDrawElements(GL_TRIANGLES, fireExtinguisherModels[modelIndex].indexCount,
                    GL_UNSIGNED_INT, 0);
            }
        }

        // Отключаем VAO
        glBindVertexArray(0);

        // Отображаем на экране
        window.display();
    }

    // Очистка ресурсов
    std::cout << "\n=== Cleaning up resources ===" << std::endl;

    // Очищаем центральную модель
    if (centralOilDrum.vao != 0) {
        glDeleteVertexArrays(1, &centralOilDrum.vao);
        glDeleteBuffers(1, &centralOilDrum.vbo);
        glDeleteBuffers(1, &centralOilDrum.ebo);
        glDeleteTextures(1, &centralOilDrum.texture);
        std::cout << "Cleaned up central oil drum model" << std::endl;
    }

    // Очищаем модели огнетушителей
    for (auto& model : fireExtinguisherModels) {
        glDeleteVertexArrays(1, &model.vao);
        glDeleteBuffers(1, &model.vbo);
        glDeleteBuffers(1, &model.ebo);
        glDeleteTextures(1, &model.texture);
    }
    std::cout << "Cleaned up " << fireExtinguisherModels.size() << " fire extinguisher models" << std::endl;

    glDeleteProgram(shaderProgram);

    std::cout << "Program terminated successfully" << std::endl;

    return 0;
}
