#include <SFML/Window.hpp>
#include <SFML/Graphics.hpp>
#include <GL/glew.h>
#include <iostream>
#include <string>
#include <cmath>
#include <vector>
#include <optional>

// Вершинный шейдер
const char* vertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;

out vec3 ourColor;

uniform vec3 offset;
uniform mat4 rotation;
uniform vec3 scale;

void main() {
    vec3 scaledPos = aPos * scale;
    gl_Position = rotation * vec4(scaledPos + offset, 1.0);
    ourColor = aColor;
}
)";

// Фрагментный шейдер
const char* fragmentShaderSource = R"(
#version 330 core
in vec3 ourColor;
out vec4 FragColor;

void main() {
    FragColor = vec4(ourColor, 1.0);
}
)";

// Функция для компиляции шейдера и проверки ошибок
GLuint compileShader(GLenum type, const char* source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    // Проверка на ошибки компиляции
    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        GLchar infoLog[512];
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        std::cout << "Ошибка компиляции шейдера: " << infoLog << std::endl;
        return 0;
    }
    return shader;
}

// Функция для создания шейдерной программы
GLuint createShaderProgram() {
    GLuint vertexShader = compileShader(GL_VERTEX_SHADER, vertexShaderSource);
    if (!vertexShader) return 0;

    GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentShaderSource);
    if (!fragmentShader) return 0;

    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    // Проверка на ошибки линковки
    GLint success;
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        GLchar infoLog[512];
        glGetProgramInfoLog(shaderProgram, 512, nullptr, infoLog);
        std::cout << "Ошибка линковки шейдерной программы: " << infoLog << std::endl;
        return 0;
    }

    // Удаляем шейдеры после линковки
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return shaderProgram;
}

// Функция для поворота вокруг оси Z
void rotateZ(float angle, float* matrix) {
    float rad = angle * 3.14159265f / 180.0f;
    float cosA = cos(rad);
    float sinA = sin(rad);

    matrix[0] = cosA;  matrix[4] = -sinA; matrix[8]  = 0.0f; matrix[12] = 0.0f;
    matrix[1] = sinA;  matrix[5] = cosA;  matrix[9]  = 0.0f; matrix[13] = 0.0f;
    matrix[2] = 0.0f;  matrix[6] = 0.0f;  matrix[10] = 1.0f; matrix[14] = 0.0f;
    matrix[3] = 0.0f;  matrix[7] = 0.0f;  matrix[11] = 0.0f; matrix[15] = 1.0f;
}

// Функция для поворота вокруг оси X
void rotateX(float angle, float* matrix) {
    float rad = angle * 3.14159265f / 180.0f;
    float cosA = cos(rad);
    float sinA = sin(rad);

    matrix[0] = 1.0f; matrix[4] = 0.0f;   matrix[8]  = 0.0f;    matrix[12] = 0.0f;
    matrix[1] = 0.0f; matrix[5] = cosA;   matrix[9]  = -sinA;   matrix[13] = 0.0f;
    matrix[2] = 0.0f; matrix[6] = sinA;   matrix[10] = cosA;    matrix[14] = 0.0f;
    matrix[3] = 0.0f; matrix[7] = 0.0f;   matrix[11] = 0.0f;    matrix[15] = 1.0f;
}

// Функция для поворота вокруг оси Y
void rotateY(float angle, float* matrix) {
    float rad = angle * 3.14159265f / 180.0f;
    float cosA = cos(rad);
    float sinA = sin(rad);

    matrix[0] = cosA;  matrix[4] = 0.0f; matrix[8]  = sinA;  matrix[12] = 0.0f;
    matrix[1] = 0.0f;  matrix[5] = 1.0f; matrix[9]  = 0.0f;  matrix[13] = 0.0f;
    matrix[2] = -sinA; matrix[6] = 0.0f; matrix[10] = cosA;  matrix[14] = 0.0f;
    matrix[3] = 0.0f;  matrix[7] = 0.0f; matrix[11] = 0.0f;  matrix[15] = 1.0f;
}

// Функция для умножения двух матриц 4x4
void multiplyMatrices(const float* a, const float* b, float* result) {
    for (int i = 0; i < 4; ++i) { // строки a
        for (int j = 0; j < 4; ++j) { // столбцы b
            result[i * 4 + j] =
                a[i * 4 + 0] * b[0 * 4 + j] +
                a[i * 4 + 1] * b[1 * 4 + j] +
                a[i * 4 + 2] * b[2 * 4 + j] +
                a[i * 4 + 3] * b[3 * 4 + j];
        }
    }
}

// Функция создания матрицы поворота на основе углов вокруг трех осей
void createRotationMatrix(float angleX, float angleY, float angleZ, float* matrix) {
    float rotX[16], rotY[16], rotZ[16], temp[16];

    rotateX(angleX, rotX);
    rotateY(angleY, rotY);
    rotateZ(angleZ, rotZ);

    // Сначала умножаем rotY на rotX: temp = rotY * rotX
    multiplyMatrices(rotY, rotX, temp);

    // Затем умножаем rotZ на temp: matrix = rotZ * temp
    multiplyMatrices(rotZ, temp, matrix);
}

// Функция для преобразования HSV в RGB (H в градусах, S и V от 0 до 1)
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

// Создание вершин для круга
void createCircleVertices(std::vector<float>& vertices, std::vector<unsigned int>& indices,
                         float radius = 0.5f, int segments = 36) {
    vertices.clear();
    indices.clear();

    // Центральная вершина круга (центр)
    vertices.push_back(0.0f); // x
    vertices.push_back(0.0f); // y
    vertices.push_back(0.0f); // z (плоский круг в z = 0)

    // Цвет центра (белый)
    vertices.push_back(1.0f); // r
    vertices.push_back(1.0f); // g
    vertices.push_back(1.0f); // b

    // Вершины окружности
    for (int i = 0; i <= segments; ++i) {
        float angle = (2.0f * 3.14159265f * i) / segments;
        float x = radius * cos(angle);
        float y = radius * sin(angle);

        // HSV: цвет зависит от угла
        float h = (360.0f * i) / segments;
        float r, g, b;
        hsvToRgb(h, 1.0f, 1.0f, r, g, b);

        // Позиция вершины
        vertices.push_back(x);
        vertices.push_back(y);
        vertices.push_back(0.0f); // z

        // Цвет вершины
        vertices.push_back(r);
        vertices.push_back(g);
        vertices.push_back(b);
    }

    // Индексы треугольников
    for (int i = 1; i <= segments; ++i) {
        indices.push_back(0);      // Центр
        indices.push_back(i);      // Текущая вершина
        indices.push_back(i + 1);  // Следующая вершина
    }
    // Последний треугольник
    indices.push_back(0);
    indices.push_back(segments);
    indices.push_back(1);
}

int main() {
    // Настройки контекста OpenGL для SFML
    sf::ContextSettings settings;
    settings.depthBits = 24;
    settings.stencilBits = 8;
    settings.antiAliasingLevel = 4;
    settings.majorVersion = 3;
    settings.minorVersion = 3;
    settings.attributeFlags = sf::ContextSettings::Core;

    // Создание окна SFML (SFML 3)
    sf::Window window;
    window.create(sf::VideoMode({800u, 600u}), "Gradient Circle with Scaling",
                  sf::Style::Default, sf::State::Windowed, settings);
    window.setFramerateLimit(60);

    // Активация контекста OpenGL
    if (!window.setActive(true)) {
        std::cout << "Не удалось активировать контекст OpenGL!" << std::endl;
        return -1;
    }

    // Инициализация GLEW
    GLenum err = glewInit();
    if (GLEW_OK != err) {
        std::cout << "Ошибка инициализации GLEW: " << glewGetErrorString(err) << std::endl;
        return -1;
    }

    // Проверка версии OpenGL
    std::cout << "OpenGL версия: " << glGetString(GL_VERSION) << std::endl;
    // Вершины и цвета для тетраэдра
    float tetrahedronVertices[] = {
        // Позиции         // Цвета
         0.0f,  0.5f,  0.0f, 1.0f, 0.0f, 0.0f,  // Вершина 1 (красный)
        -0.5f, -0.5f,  0.5f, 0.0f, 1.0f, 0.0f,  // Вершина 2 (зелёный)
         0.5f, -0.5f,  0.5f, 0.0f, 0.0f, 1.0f,  // Вершина 3 (синий)
         0.0f, -0.5f, -0.5f, 1.0f, 1.0f, 0.0f   // Вершина 4 (желтый)
    };

    unsigned int tetrahedronIndices[] = {
        0, 1, 2,  // Лицо перед
        0, 2, 3,  // Лицо справа
        0, 3, 1,  // Лицо слева
        1, 3, 2   // Основание
    };

    // Создание VAO, VBO и EBO для тетраэдра
    GLuint tetrahedronVAO, tetrahedronVBO, tetrahedronEBO;
    glGenVertexArrays(1, &tetrahedronVAO);
    glGenBuffers(1, &tetrahedronVBO);
    glGenBuffers(1, &tetrahedronEBO);

    // Привязка VAO для тетраэдра
    glBindVertexArray(tetrahedronVAO);

    // Копирование вершин в VBO
    glBindBuffer(GL_ARRAY_BUFFER, tetrahedronVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(tetrahedronVertices), tetrahedronVertices, GL_STATIC_DRAW);

    // Копирование индексов в EBO
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, tetrahedronEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(tetrahedronIndices), tetrahedronIndices, GL_STATIC_DRAW);

    // Установка атрибутов вершин для тетраэдра
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // Создание шейдерной программы
    GLuint shaderProgram = createShaderProgram();
    if (shaderProgram == 0) {
        std::cout << "Не удалось создать шейдерную программу!" << std::endl;
        return -1;
    }

    // Настройка OpenGL
    glEnable(GL_DEPTH_TEST);

    // Переменные для управления
    float offset[3] = { 0.0f, 0.0f, 0.0f };
    float moveSpeed = 0.01f;
    float scale[3] = { 1.0f, 1.0f, 1.0f };  // Масштаб по осям X, Y, Z
    float scaleSpeed = 0.02f;

    // Получение location uniform переменных
    GLint offsetLocation   = glGetUniformLocation(shaderProgram, "offset");
    GLint rotationLocation = glGetUniformLocation(shaderProgram, "rotation");
    GLint scaleLocation    = glGetUniformLocation(shaderProgram, "scale");

    if (offsetLocation == -1  || rotationLocation == -1 || scaleLocation == -1) {
        std::cout << "Не удалось получить location одной из uniform переменных!" << std::endl;
    }

    // Вершины и индексы для круга
    std::vector<float> circleVertices;
    std::vector<unsigned int> circleIndices;
    createCircleVertices(circleVertices, circleIndices, 0.5f, 64);

    // Создание VBO, VAO и EBO для круга
    GLuint circleVBO, circleVAO, circleEBO;
    glGenVertexArrays(1, &circleVAO);
    glGenBuffers(1, &circleVBO);
    glGenBuffers(1, &circleEBO);

    // Привязка VAO для круга
    glBindVertexArray(circleVAO);

    // Копирование вершин в VBO
    glBindBuffer(GL_ARRAY_BUFFER, circleVBO);
    glBufferData(GL_ARRAY_BUFFER, circleVertices.size() * sizeof(float),
                 circleVertices.data(), GL_STATIC_DRAW);

    // Копирование индексов в EBO
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, circleEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, circleIndices.size() * sizeof(unsigned int),
                 circleIndices.data(), GL_STATIC_DRAW);

    // Установка атрибутов вершин для круга
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // Основной цикл
    float angleX = 0.0f;
    float angleY = 0.0f;
    float angleZ = 0.0f;

    while (window.isOpen()) {
        // Обработка событий (новый API SFML 3)
        while (const std::optional<sf::Event> event = window.pollEvent()) {
            // Закрытие окна
            if (event->is<sf::Event::Closed>()) {
                window.close();
            }
            // Нажатие клавиш
            else if (const auto* key = event->getIf<sf::Event::KeyPressed>()) {
                using Key = sf::Keyboard::Key;

                // Обработка нажатий клавиш для перемещения
                if (key->code == Key::W)       offset[1] += moveSpeed; // Вверх
                else if (key->code == Key::S)  offset[1] -= moveSpeed; // Вниз
                else if (key->code == Key::A)  offset[0] -= moveSpeed; // Влево
                else if (key->code == Key::D)  offset[0] += moveSpeed; // Вправо
                else if (key->code == Key::Q)  offset[2] -= moveSpeed; // Вглубь
                else if (key->code == Key::E)  offset[2] += moveSpeed; // Наружу

                else if (key->code == Key::R) {
                    // Сброс позиции и масштаба
                    offset[0] = 0.0f;
                    offset[1] = 0.0f;
                    offset[2] = 0.0f;
                    scale[0] = 1.0f;
                    scale[1] = 1.0f;
                    scale[2] = 1.0f;
                }
                // Масштабирование по осям
                else if (key->code == Key::Num1) scale[0] += scaleSpeed; // Масштаб по X
                else if (key->code == Key::Num2) scale[0] -= scaleSpeed; // Масштаб по X
                else if (key->code == Key::Num3) scale[1] += scaleSpeed; // Масштаб по Y
                else if (key->code == Key::Num4) scale[1] -= scaleSpeed; // Масштаб по Y
                else if (key->code == Key::Num5) scale[2] += scaleSpeed; // Масштаб по Z
                else if (key->code == Key::Num6) scale[2] -= scaleSpeed; // Масштаб по Z

                else if (key->code == Key::Escape) window.close();
            }
            // Изменение размера окна
            else if (const auto* resized = event->getIf<sf::Event::Resized>()) {
                glViewport(
                    0,
                    0,
                    static_cast<GLsizei>(resized->size.x),
                    static_cast<GLsizei>(resized->size.y)
                );
            }
        }

        // Очистка экрана
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Использование шейдерной программы
        glUseProgram(shaderProgram);

        // Передача uniform переменных
        glUniform3fv(offsetLocation, 1, offset);
        glUniform3fv(scaleLocation, 1, scale);

        // Создание матрицы поворота
        float rotationMatrix[16];
        createRotationMatrix(angleX, angleY, angleZ, rotationMatrix);
        glUniformMatrix4fv(rotationLocation, 1, GL_FALSE, rotationMatrix);

        // Рисование тетраэдра
        glBindVertexArray(tetrahedronVAO);
        glDrawElements(GL_TRIANGLES, 12, GL_UNSIGNED_INT, 0);

        // Рисование круга
        glBindVertexArray(circleVAO);
        glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(circleIndices.size()), GL_UNSIGNED_INT, 0);

        // Обновление углов поворота
        angleX += 0.5f;
        angleY += 0.3f;
        angleZ += 0.2f;

        // Обновление окна
        window.display();
    }

    // Очистка ресурсов
    glDeleteVertexArrays(1, &tetrahedronVAO);
    glDeleteBuffers(1, &tetrahedronVBO);
    glDeleteBuffers(1, &tetrahedronEBO);
    glDeleteVertexArrays(1, &circleVAO);
    glDeleteBuffers(1, &circleVBO);
    glDeleteBuffers(1, &circleEBO);
    glDeleteProgram(shaderProgram);

    return 0;
}