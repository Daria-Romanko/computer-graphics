#include <GL/glew.h>
#include <SFML/OpenGL.hpp>
#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/constants.hpp>
#include <iostream>
#include <vector>
#include <fstream>
#include <cmath>
#include <cstring>
#include <cstdint>
#include <random>
#include <sstream>

// Структура для хранения данных модели OBJ
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

// Функция для загрузки модели OBJ
bool LoadOBJModel(const std::string& filename, Model& model) {
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
            // Вершина
            glm::vec3 vertex;
            sscanf_s(line.c_str(), "v %f %f %f", &vertex.x, &vertex.y, &vertex.z);
            tempVertices.push_back(vertex);
        }
        else if (line.substr(0, 3) == "vt ") {
            // Текстурная координата
            glm::vec2 texCoord;
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
            model.texCoords.push_back(glm::vec2(0, 0));
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
    const std::uint8_t* pixels = image.getPixelsPtr();
    unsigned int width = image.getSize().x;
    unsigned int height = image.getSize().y;

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

// Функция для загрузки шейдера из файла
std::string LoadShaderFromFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cout << "Failed to open shader file: " << filename << std::endl;
        return "";
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    file.close();

    std::cout << "Loaded shader from: " << filename << std::endl;
    return buffer.str();
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

// Создание шейдерной программы из файлов
GLuint CreateShaderProgramFromFiles(const std::string& vertexShaderFile, const std::string& fragmentShaderFile) {
    // Загружаем шейдеры из файлов
    std::string vertexShaderSource = LoadShaderFromFile(vertexShaderFile);
    std::string fragmentShaderSource = LoadShaderFromFile(fragmentShaderFile);

    if (vertexShaderSource.empty() || fragmentShaderSource.empty()) {
        std::cout << "Failed to load shader files." << std::endl;
        return -1;   
    }

    GLuint vertexShader = CompileShader(GL_VERTEX_SHADER, vertexShaderSource.c_str());
    GLuint fragmentShader = CompileShader(GL_FRAGMENT_SHADER, fragmentShaderSource.c_str());

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

    std::cout << "Shader program created successfully" << std::endl;
    return shaderProgram;
}

// Функция для создания простой цилиндрической модели (заглушка)
Model CreateCylinderModel(float radius, float height, int segments, const std::string& name) {
    Model model;
    model.name = name;

    // Вершины для цилиндрической модели
    for (int j = 0; j <= segments; j++) {
        float angle = 2.0f * 3.14159265f * j / segments;
        float x = radius * cos(angle);
        float z = radius * sin(angle);

        // Верхнее кольцо
        model.vertices.push_back(glm::vec3(x, height / 2, z));
        // Нижнее кольцо
        model.vertices.push_back(glm::vec3(x, -height / 2, z));

        // Текстурные координаты
        model.texCoords.push_back(glm::vec2(j / (float)segments, 0));
        model.texCoords.push_back(glm::vec2(j / (float)segments, 1));
    }

    // Индексы для боковой поверхности
    for (int j = 0; j < segments; j++) {
        int base = j * 2;
        model.indices.push_back(base);
        model.indices.push_back(base + 1);
        model.indices.push_back(base + 2);

        model.indices.push_back(base + 1);
        model.indices.push_back(base + 3);
        model.indices.push_back(base + 2);
    }

    // Верхняя и нижняя крышки
    int centerTop = model.vertices.size();
    model.vertices.push_back(glm::vec3(0, height / 2, 0));
    model.texCoords.push_back(glm::vec2(0.5, 0.5));

    int centerBottom = model.vertices.size();
    model.vertices.push_back(glm::vec3(0, -height / 2, 0));
    model.texCoords.push_back(glm::vec2(0.5, 0.5));

    for (int j = 0; j < segments; j++) {
        int v1 = j * 2;
        int v2 = ((j + 1) % segments) * 2;

        // Верхняя крышка
        model.indices.push_back(centerTop);
        model.indices.push_back(v1);
        model.indices.push_back(v2);

        // Нижняя крышка
        model.indices.push_back(centerBottom);
        model.indices.push_back(v1 + 1);
        model.indices.push_back(v2 + 1);
    }

    model.indexCount = model.indices.size();

    return model;
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

    // Создаем шейдерную программу из файлов
    std::cout << "\n=== Loading shaders ===" << std::endl;
    GLuint shaderProgram = CreateShaderProgramFromFiles("vert.txt", "frag.txt");
    if (!shaderProgram) {
        std::cerr << "Failed to create shader program" << std::endl;
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
            centralOilDrum = Model();
        }
    }
    else {
        std::cout << "Failed to load oil-drum_col.obj. Creating placeholder..." << std::endl;

        // Создаем простую модель бочки-заглушки с использованием GLM
        centralOilDrum = CreateCylinderModel(1.0f, 2.0f, 16, "Placeholder Oil Drum");

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
            Model placeholder = CreateCylinderModel(0.3f, 1.5f, 12,
                "Placeholder Fire Extinguisher " + std::to_string(i + 1));

            // Инициализируем с текстурой
            if (InitializeModelGL(placeholder, "fire_extinguisher_texture.jpg")) {
                fireExtinguisherModels.push_back(placeholder);
            }
        }
    }

    if (fireExtinguisherModels.empty()) {
        std::cerr << "Failed to create fire extinguisher models" << std::endl;
    }

    // Создаем 100+ "планет" (огнетушителей) с разными позициями
    const int NUM_PLANETS = 100;
    std::vector<glm::vec3> planetPositions;
    std::vector<float> planetRotations;
    std::vector<int> planetModelIndices;

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> angleDist(0.0f, 2.0f * 3.14159265f);
    std::uniform_real_distribution<float> heightDist(-2.0f, 2.0f);
    std::uniform_int_distribution<int> modelIndexDist(0, fireExtinguisherModels.empty() ? 0 : fireExtinguisherModels.size() - 1);

    for (int i = 0; i < NUM_PLANETS; i++) {
        // Случайные позиции в эллиптических орбитах вокруг центральной бочки
        float radius = 8.0f + (i % 15) * 1.5f;
        float angle = angleDist(gen);
        float height = heightDist(gen);

        planetPositions.push_back(glm::vec3(
            radius * std::cos(angle),
            height,
            radius * std::sin(angle)
        ));

        planetRotations.push_back(0.0f);
        planetModelIndices.push_back(fireExtinguisherModels.empty() ? 0 : modelIndexDist(gen));
    }

    std::cout << "\n=== Starting simulation ===" << std::endl;
    std::cout << "Central object: Oil Drum" << std::endl;
    std::cout << "Orbiting objects: " << NUM_PLANETS << " fire extinguishers" << std::endl;
    std::cout << "Close window or press ESC to exit" << std::endl;

    // Получаем uniform-локации
    GLuint modelLoc = glGetUniformLocation(shaderProgram, "model");
    GLuint viewLoc = glGetUniformLocation(shaderProgram, "view");
    GLuint projectionLoc = glGetUniformLocation(shaderProgram, "projection");

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

        // Настраиваем матрицы вида и проекции с использованием GLM
        float time = clock.getElapsedTime().asSeconds();

        // Камера вращается вокруг всей сцены
        float cameraRadius = 30.0f;
        glm::vec3 cameraPos(
            cameraRadius * std::sin(time * 0.08f),
            8.0f,
            cameraRadius * std::cos(time * 0.08f)
        );

        // Камера смотрит на центральную бочку
        glm::vec3 center(0, 0, 0);
        glm::vec3 up(0, 1, 0);

        // Создаем view и projection матрицы с помощью GLM
        glm::mat4 view = glm::lookAt(cameraPos, center, up);
        glm::mat4 projection = glm::perspective(
            glm::radians(45.0f),  // FOV
            800.0f / 600.0f,      // Aspect ratio
            0.1f,                 // Near plane
            200.0f                // Far plane
        );

        // Устанавливаем view и projection матрицы
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));

        // === РИСУЕМ ЦЕНТРАЛЬНУЮ БОЧКУ ===
        if (centralOilDrum.vao != 0) {
            // Привязываем VAO модели бочки
            glBindVertexArray(centralOilDrum.vao);

            // Активируем текстуру
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, centralOilDrum.texture);

            // Создаем трансформацию для центральной бочки с использованием GLM
            glm::mat4 modelMat = glm::mat4(1.0f);

            // Бочка стоит в центре
            modelMat = glm::translate(modelMat, glm::vec3(0, -1.0f, 0));

            // Медленное вращение бочки вокруг своей оси
            modelMat = glm::rotate(modelMat, time * 0.2f, glm::vec3(0, 1, 0));

            // Масштаб бочки (больше, чем огнетушители)
            modelMat = glm::scale(modelMat, glm::vec3(2.0f, 2.0f, 2.0f));

            // Устанавливаем матрицу model
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelMat));

            // Рисуем модель
            glDrawElements(GL_TRIANGLES, centralOilDrum.indexCount, GL_UNSIGNED_INT, 0);
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

                    // Создаем трансформацию для этого экземпляра с использованием GLM
                    glm::mat4 modelMat = glm::mat4(1.0f);

                    // Позиционируем экземпляр по кругу (ближняя орбита вокруг бочки)
                    float angle = (instance * 72.0f + modelIndex * 36.0f) * glm::pi<float>() / 180.0f;
                    float radius = 5.0f; // Ближе к центру

                    glm::vec3 position(
                        radius * std::cos(angle + time * 0.5f),
                        0,
                        radius * std::sin(angle + time * 0.5f)
                    );

                    // Устанавливаем позицию
                    modelMat = glm::translate(modelMat, position);

                    // Вращение вокруг своей оси
                    modelMat = glm::rotate(modelMat, time * 1.0f + instance * 0.3f, glm::vec3(0, 1, 0));

                    // Масштаб (больше для ближних моделей)
                    modelMat = glm::scale(modelMat, glm::vec3(0.8f, 0.8f, 0.8f));

                    // Устанавливаем матрицу model
                    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelMat));

                    // Рисуем модель
                    glDrawElements(GL_TRIANGLES, fireExtinguisherModels[modelIndex].indexCount, GL_UNSIGNED_INT, 0);
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

                // Создаем трансформацию для планеты с использованием GLM
                glm::mat4 modelMat = glm::mat4(1.0f);

                // Позиция (орбитальное движение вокруг центральной бочки)
                float orbitSpeed = 0.01f * ((i % 7) + 1);
                glm::vec3 pos = planetPositions[i];

                // Преобразуем позицию через матрицу орбиты
                float x = pos.x * std::cos(time * orbitSpeed) - pos.z * std::sin(time * orbitSpeed);
                float z = pos.x * std::sin(time * orbitSpeed) + pos.z * std::cos(time * orbitSpeed);

                // Устанавливаем позицию
                modelMat = glm::translate(modelMat, glm::vec3(x, pos.y, z));

                // Вращение вокруг своей оси
                modelMat = glm::rotate(modelMat, planetRotations[i], glm::vec3(0, 1, 0));

                // Масштаб (случайный размер для разнообразия)
                float scaleFactor = 0.15f + 0.08f * (i % 10);
                modelMat = glm::scale(modelMat, glm::vec3(scaleFactor, scaleFactor, scaleFactor));

                // Устанавливаем матрицу model
                glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelMat));

                // Рисуем модель
                glDrawElements(GL_TRIANGLES, fireExtinguisherModels[modelIndex].indexCount, GL_UNSIGNED_INT, 0);
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
    std::cout << "Cleaned up shader program" << std::endl;

    std::cout << "Program terminated successfully" << std::endl;

    return 0;
}