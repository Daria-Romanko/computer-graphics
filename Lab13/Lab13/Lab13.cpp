#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <vector>
#include <fstream>
#include <cmath>
#include <cstring>
#include <cstdint>
#include <random>
#include <sstream>
#include <algorithm>

// ================== КЛАСС КАМЕРЫ ==================
class Camera {
private:
    glm::vec3 position;
    glm::vec3 front;
    glm::vec3 up;
    glm::vec3 right;
    glm::vec3 worldUp;

    float yaw;      // Поворот влево-вправо
    float pitch;    // Поворот вверх-вниз

    float movementSpeed;
    float mouseSensitivity;
    float zoom;

public:
    Camera(glm::vec3 startPosition = glm::vec3(0.0f, 0.0f, 10.0f),
           glm::vec3 startUp = glm::vec3(0.0f, 1.0f, 0.0f),
           float startYaw = -90.0f,
           float startPitch = 0.0f)
        : position(startPosition)
        , worldUp(startUp)
        , yaw(startYaw)
        , pitch(startPitch)
        , movementSpeed(5.0f)
        , mouseSensitivity(0.1f)
        , zoom(45.0f) {
        updateCameraVectors();
    }

    glm::mat4 getViewMatrix() const {
        return glm::lookAt(position, position + front, up);
    }

    float getZoom() const { return zoom; }
    void setZoom(float value) { zoom = glm::clamp(value, 1.0f, 90.0f); }
    
    glm::vec3 getPosition() const { return position; }
    glm::vec3 getFront() const { return front; }

    void processKeyboard(int key, float deltaTime) {
        float velocity = movementSpeed * deltaTime;
        
        switch (key) {
            case GLFW_KEY_W:
                position += front * velocity;
                break;
            case GLFW_KEY_S:
                position -= front * velocity;
                break;
            case GLFW_KEY_A:
                position -= right * velocity;
                break;
            case GLFW_KEY_D:
                position += right * velocity;
                break;
            case GLFW_KEY_SPACE:
                position += worldUp * velocity;
                break;
            case GLFW_KEY_LEFT_SHIFT:
                position -= worldUp * velocity;
                break;
        }
    }

    void processMouseMovement(float xoffset, float yoffset, bool constrainPitch = true) {
        xoffset *= mouseSensitivity;
        yoffset *= mouseSensitivity;

        yaw += xoffset;
        pitch += yoffset;

        if (constrainPitch) {
            if (pitch > 89.0f) pitch = 89.0f;
            if (pitch < -89.0f) pitch = -89.0f;
        }

        updateCameraVectors();
    }

    void processMouseScroll(float yoffset) {
        zoom -= yoffset;
        if (zoom < 1.0f) zoom = 1.0f;
        if (zoom > 90.0f) zoom = 90.0f;
    }

private:
    void updateCameraVectors() {
        glm::vec3 newFront;
        newFront.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        newFront.y = sin(glm::radians(pitch));
        newFront.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
        front = glm::normalize(newFront);

        right = glm::normalize(glm::cross(front, worldUp));
        up = glm::normalize(glm::cross(right, front));
    }
};

// ================== СТРУКТУРА МОДЕЛИ ==================
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

// ================== ФУНКЦИИ ДЛЯ ЗАГРУЗКИ ==================
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
            glm::vec3 vertex;
            if (sscanf(line.c_str(), "v %f %f %f", &vertex.x, &vertex.y, &vertex.z) == 3) {
                tempVertices.push_back(vertex);
            }
        }
        else if (line.substr(0, 3) == "vt ") {
            glm::vec2 texCoord;
            if (sscanf(line.c_str(), "vt %f %f", &texCoord.x, &texCoord.y) == 2) {
                tempTexCoords.push_back(texCoord);
            }
        }
        else if (line.substr(0, 2) == "f ") {
            unsigned int vertexIndex[3], texIndex[3];
            int matches = sscanf(line.c_str(), "f %d/%d %d/%d %d/%d",
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
                matches = sscanf(line.c_str(), "f %d// %d// %d//",
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

    std::cout << "Loaded model: " << filename
        << " (vertices: " << model.vertices.size()
        << ", indices: " << model.indices.size() << ")" << std::endl;

    return !model.vertices.empty();
}

GLuint LoadTextureFromFile(const std::string& filename) {
    // Простая заглушка для текстуры
    std::cout << "Loading texture: " << filename << std::endl;
    
    const int width = 64;
    const int height = 64;
    std::uint8_t pixels[width * height * 4];

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int index = (y * width + x) * 4;
            // Простой паттерн для теста
            pixels[index] = (x * 4) % 256;     // R
            pixels[index + 1] = (y * 4) % 256; // G
            pixels[index + 2] = ((x + y) * 2) % 256; // B
            pixels[index + 3] = 255;           // A
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

bool InitializeModelGL(Model& model, const std::string& textureFile = "") {
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
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, model.indices.size() * sizeof(unsigned int),
        model.indices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    model.texture = LoadTextureFromFile(textureFile);

    glBindVertexArray(0);

    return true;
}

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

GLuint CreateShaderProgramFromFiles(const std::string& vertexShaderFile, const std::string& fragmentShaderFile) {
    std::string vertexShaderSource = LoadShaderFromFile(vertexShaderFile);
    std::string fragmentShaderSource = LoadShaderFromFile(fragmentShaderFile);

    if (vertexShaderSource.empty() || fragmentShaderSource.empty()) {
        std::cout << "Failed to load shader files. Using default shaders." << std::endl;
        
        // Простые шейдеры по умолчанию
        vertexShaderSource = R"(
            #version 330 core
            layout (location = 0) in vec3 aPos;
            layout (location = 1) in vec2 aTexCoord;
            
            out vec2 TexCoord;
            
            uniform mat4 model;
            uniform mat4 view;
            uniform mat4 projection;
            
            void main() {
                gl_Position = projection * view * model * vec4(aPos, 1.0);
                TexCoord = aTexCoord;
            }
        )";
        
        fragmentShaderSource = R"(
            #version 330 core
            in vec2 TexCoord;
            out vec4 FragColor;
            
            uniform sampler2D texture1;
            
            void main() {
                FragColor = texture(texture1, TexCoord);
            }
        )";
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

Model CreateCylinderModel(float radius, float height, int segments, const std::string& name) {
    Model model;
    model.name = name;

    for (int j = 0; j <= segments; j++) {
        float angle = 2.0f * 3.14159265f * j / segments;
        float x = radius * cos(angle);
        float z = radius * sin(angle);

        model.vertices.push_back(glm::vec3(x, height / 2, z));
        model.vertices.push_back(glm::vec3(x, -height / 2, z));

        model.texCoords.push_back(glm::vec2(j / (float)segments, 0));
        model.texCoords.push_back(glm::vec2(j / (float)segments, 1));
    }

    for (int j = 0; j < segments; j++) {
        int base = j * 2;
        model.indices.push_back(base);
        model.indices.push_back(base + 1);
        model.indices.push_back(base + 2);

        model.indices.push_back(base + 1);
        model.indices.push_back(base + 3);
        model.indices.push_back(base + 2);
    }

    int centerTop = model.vertices.size();
    model.vertices.push_back(glm::vec3(0, height / 2, 0));
    model.texCoords.push_back(glm::vec2(0.5, 0.5));

    int centerBottom = model.vertices.size();
    model.vertices.push_back(glm::vec3(0, -height / 2, 0));
    model.texCoords.push_back(glm::vec2(0.5, 0.5));

    for (int j = 0; j < segments; j++) {
        int v1 = j * 2;
        int v2 = ((j + 1) % segments) * 2;

        model.indices.push_back(centerTop);
        model.indices.push_back(v1);
        model.indices.push_back(v2);

        model.indices.push_back(centerBottom);
        model.indices.push_back(v1 + 1);
        model.indices.push_back(v2 + 1);
    }

    model.indexCount = model.indices.size();

    return model;
}

// Глобальные переменные для обратных вызовов GLFW
Camera camera(glm::vec3(0.0f, 5.0f, 20.0f));
bool firstMouse = true;
float lastX = 400.0f;
float lastY = 300.0f;

void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // обратный порядок для Y
    lastX = xpos;
    lastY = ypos;

    camera.processMouseMovement(xoffset, yoffset);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    camera.processMouseScroll(yoffset);
}

// ================== ГЛАВНАЯ ФУНКЦИЯ ==================
int main() {
    // Инициализация GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }

    // Настройка GLFW
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_SAMPLES, 4); // MSAA

    // Создание окна
    GLFWwindow* window = glfwCreateWindow(800, 600, "Solar System with Interactive Camera", NULL, NULL);
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);

    // Захват мыши
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // Инициализируем GLEW
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        std::cerr << "Failed to initialize GLEW" << std::endl;
        return -1;
    }

    // Настраиваем OpenGL
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_MULTISAMPLE);

    // Загружаем шейдеры
    std::cout << "\n=== Loading shaders ===" << std::endl;
    GLuint shaderProgram = CreateShaderProgramFromFiles("vert.txt", "frag.txt");
    if (!shaderProgram) {
        std::cerr << "Failed to create shader program" << std::endl;
        return -1;
    }

    // Загружаем центральную модель
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
        centralOilDrum = CreateCylinderModel(1.0f, 2.0f, 16, "Placeholder Oil Drum");
        if (InitializeModelGL(centralOilDrum, "oil-drum_col_texture.jpg")) {
            std::cout << "Created placeholder oil drum model" << std::endl;
        }
        else {
            std::cout << "Failed to create placeholder oil drum model" << std::endl;
            centralOilDrum = Model();
        }
    }

    // Создаем модели огнетушителей
    std::vector<Model> fireExtinguisherModels;

    std::cout << "\n=== Loading fire extinguisher models ===" << std::endl;
    for (int i = 0; i < 5; i++) {
        Model fireExtinguisher;
        fireExtinguisher.name = "Fire Extinguisher " + std::to_string(i + 1);

        if (LoadOBJModel("fire_extinguisher.obj", fireExtinguisher)) {
            if (InitializeModelGL(fireExtinguisher, "fire_extinguisher_texture.jpg")) {
                fireExtinguisherModels.push_back(fireExtinguisher);
                std::cout << "Loaded fire extinguisher model " << i + 1 << std::endl;
            }
        }
        else {
            std::cout << "Failed to load fire_extinguisher.obj. Creating placeholder model..." << std::endl;
            Model placeholder = CreateCylinderModel(0.3f, 1.5f, 12,
                "Placeholder Fire Extinguisher " + std::to_string(i + 1));
            if (InitializeModelGL(placeholder, "fire_extinguisher_texture.jpg")) {
                fireExtinguisherModels.push_back(placeholder);
            }
        }
    }

    // Создаем "планеты"
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
    std::cout << "Camera Controls:" << std::endl;
    std::cout << "  WASD - Move forward/backward/left/right" << std::endl;
    std::cout << "  Space/Shift - Move up/down" << std::endl;
    std::cout << "  Mouse - Look around" << std::endl;
    std::cout << "  ESC - Exit" << std::endl;
    std::cout << "\nClose window or press ESC to exit" << std::endl;

    // Получаем uniform-локации
    GLuint modelLoc = glGetUniformLocation(shaderProgram, "model");
    GLuint viewLoc = glGetUniformLocation(shaderProgram, "view");
    GLuint projectionLoc = glGetUniformLocation(shaderProgram, "projection");

    // Таймер для плавного движения
    double lastTime = glfwGetTime();
    float time = 0.0f;

    // Основной цикл
    while (!glfwWindowShouldClose(window)) {
        // Вычисляем deltaTime
        double currentTime = glfwGetTime();
        float deltaTime = currentTime - lastTime;
        lastTime = currentTime;
        time += deltaTime;

        // Обработка ввода
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window, true);

        // Управление камерой
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
            camera.processKeyboard(GLFW_KEY_W, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
            camera.processKeyboard(GLFW_KEY_S, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
            camera.processKeyboard(GLFW_KEY_A, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
            camera.processKeyboard(GLFW_KEY_D, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
            camera.processKeyboard(GLFW_KEY_SPACE, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
            camera.processKeyboard(GLFW_KEY_LEFT_SHIFT, deltaTime);

        // Очистка буферов
        glClearColor(0.0f, 0.0f, 0.05f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Используем шейдерную программу
        glUseProgram(shaderProgram);

        // Получаем матрицы из камеры
        glm::mat4 view = camera.getViewMatrix();
        int width, height;
        glfwGetWindowSize(window, &width, &height);
        glm::mat4 projection = glm::perspective(
            glm::radians(camera.getZoom()),
            (float)width / (float)height,
            0.1f,
            200.0f
        );

        // Устанавливаем view и projection матрицы
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));

        // === РИСУЕМ ЦЕНТРАЛЬНУЮ БОЧКУ ===
        if (centralOilDrum.vao != 0) {
            glBindVertexArray(centralOilDrum.vao);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, centralOilDrum.texture);

            glm::mat4 modelMat = glm::mat4(1.0f);
            modelMat = glm::translate(modelMat, glm::vec3(0, -1.0f, 0));
            modelMat = glm::rotate(modelMat, time * 0.2f, glm::vec3(0, 1, 0));
            modelMat = glm::scale(modelMat, glm::vec3(2.0f, 2.0f, 2.0f));

            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelMat));
            glDrawElements(GL_TRIANGLES, centralOilDrum.indexCount, GL_UNSIGNED_INT, 0);
        }

        // === РИСУЕМ 5 БЛИЖАЙШИХ ОГНЕТУШИТЕЛЕЙ ===
        if (!fireExtinguisherModels.empty()) {
            for (int instance = 0; instance < 5; instance++) {
                for (size_t modelIndex = 0; modelIndex < std::min(fireExtinguisherModels.size(), (size_t)2); modelIndex++) {
                    glBindVertexArray(fireExtinguisherModels[modelIndex].vao);
                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, fireExtinguisherModels[modelIndex].texture);

                    glm::mat4 modelMat = glm::mat4(1.0f);
                    float angle = (instance * 72.0f + modelIndex * 36.0f) * glm::pi<float>() / 180.0f;
                    float radius = 5.0f;

                    glm::vec3 position(
                        radius * std::cos(angle + time * 0.5f),
                        0,
                        radius * std::sin(angle + time * 0.5f)
                    );

                    modelMat = glm::translate(modelMat, position);
                    modelMat = glm::rotate(modelMat, time * 1.0f + instance * 0.3f, glm::vec3(0, 1, 0));
                    modelMat = glm::scale(modelMat, glm::vec3(0.8f, 0.8f, 0.8f));

                    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelMat));
                    glDrawElements(GL_TRIANGLES, fireExtinguisherModels[modelIndex].indexCount, GL_UNSIGNED_INT, 0);
                }
            }
        }

        // === РИСУЕМ 100+ "ПЛАНЕТ" ===
        if (!fireExtinguisherModels.empty()) {
            for (int i = 0; i < NUM_PLANETS; i++) {
                int modelIndex = planetModelIndices[i];
                glBindVertexArray(fireExtinguisherModels[modelIndex].vao);
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, fireExtinguisherModels[modelIndex].texture);

                planetRotations[i] += 0.005f * (i % 10 + 1);

                glm::mat4 modelMat = glm::mat4(1.0f);
                float orbitSpeed = 0.01f * ((i % 7) + 1);
                glm::vec3 pos = planetPositions[i];

                float x = pos.x * std::cos(time * orbitSpeed) - pos.z * std::sin(time * orbitSpeed);
                float z = pos.x * std::sin(time * orbitSpeed) + pos.z * std::cos(time * orbitSpeed);

                modelMat = glm::translate(modelMat, glm::vec3(x, pos.y, z));
                modelMat = glm::rotate(modelMat, planetRotations[i], glm::vec3(0, 1, 0));

                float scaleFactor = 0.15f + 0.08f * (i % 10);
                modelMat = glm::scale(modelMat, glm::vec3(scaleFactor, scaleFactor, scaleFactor));

                glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelMat));
                glDrawElements(GL_TRIANGLES, fireExtinguisherModels[modelIndex].indexCount, GL_UNSIGNED_INT, 0);
            }
        }

        glBindVertexArray(0);
        
        // Обмен буферов и опрос событий
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Очистка ресурсов
    std::cout << "\n=== Cleaning up resources ===" << std::endl;

    if (centralOilDrum.vao != 0) {
        glDeleteVertexArrays(1, &centralOilDrum.vao);
        glDeleteBuffers(1, &centralOilDrum.vbo);
        glDeleteBuffers(1, &centralOilDrum.ebo);
        glDeleteTextures(1, &centralOilDrum.texture);
        std::cout << "Cleaned up central oil drum model" << std::endl;
    }

    for (auto& model : fireExtinguisherModels) {
        glDeleteVertexArrays(1, &model.vao);
        glDeleteBuffers(1, &model.vbo);
        glDeleteBuffers(1, &model.ebo);
        glDeleteTextures(1, &model.texture);
    }
    std::cout << "Cleaned up " << fireExtinguisherModels.size() << " fire extinguisher models" << std::endl;

    glDeleteProgram(shaderProgram);
    std::cout << "Cleaned up shader program" << std::endl;

    glfwDestroyWindow(window);
    glfwTerminate();

    std::cout << "Program terminated successfully" << std::endl;

    return 0;
}