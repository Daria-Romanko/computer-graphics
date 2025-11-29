#include <SFML/Window.hpp>
#include <SFML/Graphics.hpp>
#include <GL/glew.h>
#include <iostream>
#include <string>
#include <cmath>

// Вершинный шейдер
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

// Фрагментный шейдер
const char* fragmentShaderSource = R"(
#version 330 core
out vec4 FragColor;
in vec3 ourColor;

void main()
{
    FragColor = vec4(ourColor, 1.0);
}
)";

// Функция для компиляции шейдера
GLuint compileShader(GLenum type, const char* source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    // Проверка ошибок компиляции
    int success;
    char infoLog[512];
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        std::cout << "Ошибка компиляции шейдера:\n" << infoLog << std::endl;
    }

    return shader;
}

// Функция для создания шейдерной программы
GLuint createShaderProgram() {
    GLuint vertexShader = compileShader(GL_VERTEX_SHADER, vertexShaderSource);
    GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentShaderSource);

    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    // Проверка ошибок линковки
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

// Функция для создания матрицы вращения вокруг оси Y
void createRotationMatrixY(float angle, float* matrix) {
    float c = cos(angle);
    float s = sin(angle);

    // Инициализация как единичной матрицы
    for (int i = 0; i < 16; i++) {
        matrix[i] = 0.0f;
    }
    matrix[15] = 1.0f;

    // Установка элементов вращения вокруг Y
    matrix[0] = c;  matrix[2] = s;
    matrix[5] = 1.0f;
    matrix[8] = -s; matrix[10] = c;
    matrix[15] = 1.0f;
}

// Функция для создания матрицы вращения вокруг оси X
void createRotationMatrixX(float angle, float* matrix) {
    float c = cos(angle);
    float s = sin(angle);

    // Инициализация как единичной матрицы
    for (int i = 0; i < 16; i++) {
        matrix[i] = 0.0f;
    }
    matrix[15] = 1.0f;

    // Установка элементов вращения вокруг X
    matrix[0] = 1.0f;
    matrix[5] = c;  matrix[6] = -s;
    matrix[9] = s;  matrix[10] = c;
    matrix[15] = 1.0f;
}

int main() {

    // Создание окна SFML 3 с минимальными настройками
    sf::Window window(sf::VideoMode({ 800, 600 }), "Gradient tetrahedron");
    window.setFramerateLimit(60);

    // Инициализация GLEW
    if (glewInit() != GLEW_OK) {
        std::cout << "Ошибка инициализации GLEW!" << std::endl;
        return -1;
    }

    // Вершины тетраэдра (координаты и цвета) - изначально повернуты для лучшего обзора
    float vertices[] = {
        // Позиции         // Цвета
        -0.5f, -0.5f,  0.0f,  1.0f, 0.0f, 0.0f,  // Красная вершина (передняя)
         0.5f, -0.5f,  0.0f,  0.0f, 1.0f, 0.0f,  // Зеленая вершина (правая)
         0.0f, -0.5f,  0.8f,  0.0f, 0.0f, 1.0f,  // Синяя вершина (задняя)
         0.0f,  0.5f,  0.4f,  1.0f, 1.0f, 0.0f   // Желтая вершина (верхняя)
    };

    // Индексы для построения треугольников
    unsigned int indices[] = {
        0, 1, 2,  // Основание
        0, 1, 3,  // Боковая грань
        1, 2, 3,  // Боковая грань
        2, 0, 3   // Боковая грань
    };

    // Создание VBO, VAO и EBO
    GLuint VBO, VAO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    // Привязка VAO
    glBindVertexArray(VAO);

    // Копирование вершин в VBO
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // Копирование индексов в EBO
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // Установка атрибутов вершин
    // Атрибут позиции
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // Атрибут цвета
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // Создание шейдерной программы
    GLuint shaderProgram = createShaderProgram();

    // Настройка OpenGL
    glEnable(GL_DEPTH_TEST);

    // Переменные для управления
    float offset[3] = { 0.0f, 0.0f, 0.0f };
    float moveSpeed = 0.01f;

    // Получение location uniform переменных
    GLuint offsetLocation = glGetUniformLocation(shaderProgram, "offset");
    GLuint rotationLocation = glGetUniformLocation(shaderProgram, "rotation");

    // Фиксированная матрица вращения для лучшего обзора тетраэдра
    float rotationMatrix[16];

    // Создаем фиксированное вращение вокруг Y на 45 градусов и вокруг X на 30 градусов
    float yRotationMatrix[16];
    float xRotationMatrix[16];
    float tempMatrix[16];

    // Вращение вокруг Y на 45 градусов
    createRotationMatrixY(0.7854f, yRotationMatrix); // 45 градусов в радианах

    // Вращение вокруг X на 30 градусов
    createRotationMatrixX(0.5236f, xRotationMatrix); // 30 градусов в радианах

    // Умножаем матрицы: final = Y-rotation * X-rotation
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            rotationMatrix[i * 4 + j] = 0.0f;
            for (int k = 0; k < 4; k++) {
                rotationMatrix[i * 4 + j] += yRotationMatrix[i * 4 + k] * xRotationMatrix[k * 4 + j];
            }
        }
    }

    // Основной цикл
    while (window.isOpen()) {
        // Обработка событий
        for (auto event = window.pollEvent(); event.has_value(); event = window.pollEvent()) {
            const auto& e = event.value();

            if (e.is<sf::Event::Closed>()) {
                window.close();
            }
            else if (const auto* keyEvent = e.getIf<sf::Event::KeyPressed>()) {
                // Обработка нажатий клавиш для перемещения
                switch (keyEvent->scancode) {
                case sf::Keyboard::Scancode::W:
                    offset[1] += moveSpeed; // Вверх
                    break;
                case sf::Keyboard::Scancode::S:
                    offset[1] -= moveSpeed; // Вниз
                    break;
                case sf::Keyboard::Scancode::A:
                    offset[0] -= moveSpeed; // Влево
                    break;
                case sf::Keyboard::Scancode::D:
                    offset[0] += moveSpeed; // Вправо
                    break;
                case sf::Keyboard::Scancode::Q:
                    offset[2] -= moveSpeed; // Вглубь
                    break;
                case sf::Keyboard::Scancode::E:
                    offset[2] += moveSpeed; // Наружу
                    break;
                case sf::Keyboard::Scancode::R:
                    // Сброс позиции
                    offset[0] = 0.0f;
                    offset[1] = 0.0f;
                    offset[2] = 0.0f;
                    break;
                default:
                    break;
                }
            }
        }

        // Очистка экрана
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Использование шейдерной программы
        glUseProgram(shaderProgram);

        // Передача uniform переменных
        glUniform3f(offsetLocation, offset[0], offset[1], offset[2]);
        glUniformMatrix4fv(rotationLocation, 1, GL_FALSE, rotationMatrix);

        // Рендеринг тетраэдра
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, 12, GL_UNSIGNED_INT, 0);

        // Отображение результата
        window.display();
    }

    // Освобождение ресурсов
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteProgram(shaderProgram);

    return 0;
}
