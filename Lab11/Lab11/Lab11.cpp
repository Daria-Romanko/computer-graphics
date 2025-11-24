#include <GL/glew.h>
#include <SFML/Window.hpp>
#include <SFML/OpenGL.hpp>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <cmath>

#define _USE_MATH_DEFINES
#include <math.h>

using namespace std;

// Функция для инициализации GLEW
bool initGLEW() {
    GLenum err = glewInit();
    if (GLEW_OK != err) {
        cerr << "Error initializing GLEW: " << glewGetErrorString(err) << endl;
        return false;
    }
    cout << "GLEW initialized successfully" << endl;
    cout << "GLEW version: " << glewGetString(GLEW_VERSION) << endl;
    cout << "OpenGL version: " << glGetString(GL_VERSION) << endl;
    return true;
}

// Функция для логирования шейдеров
void ShaderLog(unsigned int shader) {
    int infologLen = 0;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infologLen);
    if (infologLen > 1) {
        int charsWritten = 0;
        vector<char> infoLog(infologLen);
        glGetShaderInfoLog(shader, infologLen, &charsWritten, infoLog.data());
        cout << "InfoLog: " << infoLog.data() << endl;
    }
}

// Функция для логирования шейдерной программы
void ProgramLog(unsigned int program) {
    int infologLen = 0;
    glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infologLen);
    if (infologLen > 1) {
        int charsWritten = 0;
        vector<char> infoLog(infologLen);
        glGetProgramInfoLog(program, infologLen, &charsWritten, infoLog.data());
        cout << "Program InfoLog: " << infoLog.data() << endl;
    }
}

// Функция для компиляции шейдера
GLuint compileShader(GLenum shaderType, const string& shaderSource) {
    // Создаем объект шейдера
    GLuint shader = glCreateShader(shaderType);
    if (shader == 0) {
        cerr << "Ошибка: Failed to create shader of type " << shaderType << endl;
        return 0;
    }

    // Устанавливаем исходный код шейдера
    const char* source = shaderSource.c_str();
    glShaderSource(shader, 1, &source, nullptr);

    // Компилируем шейдер
    glCompileShader(shader);

    // Проверяем статус компиляции
    GLint compileStatus;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compileStatus);

    string shaderTypeStr = (shaderType == GL_VERTEX_SHADER) ? "VERTEX" : "FRAGMENT";

    if (compileStatus != GL_TRUE) {
        cerr << "Ошибка компиляции " << shaderTypeStr << " шейдера:" << endl;
        ShaderLog(shader);
        glDeleteShader(shader);
        return 0;
    }

    cout << shaderTypeStr << " шейдер скомпилирован успешно" << endl;
    ShaderLog(shader);

    return shader;
}

// Функция для создания шейдерной программы
GLuint createShaderProgram(const string& vertexShaderSource, const string& fragmentShaderSource) {
    // Компилируем вершинный шейдер
    GLuint vertexShader = compileShader(GL_VERTEX_SHADER, vertexShaderSource);
    if (vertexShader == 0) {
        cerr << "Не удалось скомпилировать вершинный шейдер" << endl;
        return 0;
    }

    // Компилируем фрагментный шейдер
    GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentShaderSource);
    if (fragmentShader == 0) {
        cerr << "Не удалось скомпилировать фрагментный шейдер" << endl;
        glDeleteShader(vertexShader);
        return 0;
    }

    // Создаем шейдерную программу
    GLuint shaderProgram = glCreateProgram();
    if (shaderProgram == 0) {
        cerr << "Ошибка: Не удалось создать шейдерную программу" << endl;
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        return 0;
    }

    // Присоединяем шейдеры к программе
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);

    // Линкуем программу
    glLinkProgram(shaderProgram);

    // Проверяем статус линковки
    GLint linkStatus;
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &linkStatus);

    if (linkStatus != GL_TRUE) {
        cerr << "Ошибка линковки шейдерной программы:" << endl;
        ProgramLog(shaderProgram);
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        glDeleteProgram(shaderProgram);
        return 0;
    }

    cout << "Шейдерная программа слинкована успешно" << endl;
    ProgramLog(shaderProgram);

    // Удаляем шейдеры после линковки
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return shaderProgram;
}

// Функция для создания вершин четырехугольника (2 треугольника)
vector<float> createQuadVertices(float aspectRatio) {
    // Учитываем соотношение сторон для корректного отображения
    float xScale = (aspectRatio > 1.0f) ? 1.0f / aspectRatio : 1.0f;
    float yScale = (aspectRatio < 1.0f) ? aspectRatio : 1.0f;

    return {
        // Первый треугольник
        -0.9f * xScale, -0.4f * yScale, 0.0f,  // левый нижний
        -0.5f * xScale, -0.4f * yScale, 0.0f,  // правый нижний  
        -0.9f * xScale,  0.4f * yScale, 0.0f,  // левый верхний

        // Второй треугольник
        -0.5f * xScale, -0.4f * yScale, 0.0f,  // правый нижний
        -0.5f * xScale,  0.4f * yScale, 0.0f,  // правый верхний
        -0.9f * xScale,  0.4f * yScale, 0.0f   // левый верхний
    };
}

// Функция для создания вершин веера треугольников (симметричный сектор)
vector<float> createTriangleFanVertices(float aspectRatio) {
    vector<float> vertices;

    // Учитываем соотношение сторон для корректного отображения
    float xScale = (aspectRatio > 1.0f) ? 1.0f / aspectRatio : 1.0f;
    float yScale = (aspectRatio < 1.0f) ? aspectRatio : 1.0f;

    // Центральная точка (сдвинута вправо)
    vertices.insert(vertices.end(), { -0.1f * xScale, 0.0f * yScale, 0.0f });

    // Создаем симметричный веер треугольников (одинаковые правая и левая стороны)
    int segments = 8; // 8 сегментов для веера
    float startAngle = -M_PI / 3.0f; // Начальный угол (-60 градусов)
    float endAngle = M_PI / 3.0f;    // Конечный угол (60 градусов)
    float totalAngle = endAngle - startAngle;

    for (int i = 0; i <= segments; i++) {
        float angle = startAngle + totalAngle * i / segments;
        float x = 0.3f * cos(angle) * xScale;
        float y = 0.3f * sin(angle) * yScale;
        vertices.insert(vertices.end(), { x - 0.1f * xScale, y, 0.0f }); // Сдвигаем по X
    }

    return vertices;
}

// Функция для создания вершин правильного пятиугольника
vector<float> createPentagonVertices(float aspectRatio) {
    vector<float> vertices;

    // Учитываем соотношение сторон для корректного отображения
    float xScale = (aspectRatio > 1.0f) ? 1.0f / aspectRatio : 1.0f;
    float yScale = (aspectRatio < 1.0f) ? aspectRatio : 1.0f;

    int sides = 5;
    float radius = 0.3f;
    float centerX = 0.7f * xScale;
    float centerY = 0.0f * yScale;

    // Центральная точка для TRIANGLE_FAN
    vertices.insert(vertices.end(), { centerX, centerY, 0.0f });

    // Вершины пятиугольника по кругу
    for (int i = 0; i <= sides; i++) {
        float angle = 2.0f * M_PI * i / sides - M_PI / 2.0f;
        float x = radius * cos(angle) * xScale + centerX;
        float y = radius * sin(angle) * yScale + centerY;
        vertices.insert(vertices.end(), { x, y, 0.0f });
    }

    return vertices;
}

int main() {
    setlocale(LC_ALL, "ru");

    // Создаем окно с помощью SFML 3.0
    sf::Window window(sf::VideoMode({ 800, 600 }), "OpenGL Figures - Quad, Fan, Pentagon");
    window.setVerticalSyncEnabled(true);

    // Инициализируем GLEW
    if (!initGLEW()) {
        return -1;
    }

    // Включаем Z-буфер
    glEnable(GL_DEPTH_TEST);

    // Шейдеры с константным цветом для плоского закрашивания
    const char* vertexShaderSource = R"(
        #version 330 core
        layout (location = 0) in vec3 aPos;
        
        void main() {
            gl_Position = vec4(aPos, 1.0);
        }
    )";

    const char* fragmentShaderSource = R"(
        #version 330 core
        out vec4 FragColor;
        
        void main() {
            // Константный цвет для плоского закрашивания
            FragColor = vec4(0.2, 0.6, 1.0, 1.0); // Синий цвет для всех фигур
        }
    )";

    // Создаем шейдерную программу
    GLuint shaderProgram = createShaderProgram(vertexShaderSource, fragmentShaderSource);

    if (shaderProgram == 0) {
        cerr << "Не удалось создать шейдерную программу!" << endl;
        return -1;
    }

    cout << "Шейдерная программа успешно создана! ID: " << shaderProgram << endl;

    // Получаем ID атрибутов из шейдерной программы
    GLint posAttrib = glGetAttribLocation(shaderProgram, "aPos");
    if (posAttrib == -1) {
        cerr << "Ошибка: Не удалось получить location атрибута aPos!" << endl;
        return -1;
    }
    cout << "Location атрибута aPos: " << posAttrib << endl;

    // Переменные для хранения вершин и VAO/VBO
    vector<float> quadVertices, fanVertices, pentagonVertices;
    GLuint VAO[3], VBO[3];
    glGenVertexArrays(3, VAO);
    glGenBuffers(3, VBO);

    // Основной цикл
    bool running = true;
    while (running) {
        // Обработка событий
        if (auto event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                running = false;
            }
            else if (const auto* resized = event->getIf<sf::Event::Resized>()) {
                glViewport(0, 0, resized->size.x, resized->size.y);

                // При изменении размера окна пересчитываем вершины с новым соотношением сторон
                float aspectRatio = static_cast<float>(resized->size.x) / resized->size.y;

                // Обновляем вершины для всех фигур
                quadVertices = createQuadVertices(aspectRatio);
                fanVertices = createTriangleFanVertices(aspectRatio);
                pentagonVertices = createPentagonVertices(aspectRatio);

                // Обновляем буферы вершин
                for (int i = 0; i < 3; i++) {
                    glBindVertexArray(VAO[i]);
                    glBindBuffer(GL_ARRAY_BUFFER, VBO[i]);

                    vector<float>* vertices = nullptr;
                    if (i == 0) vertices = &quadVertices;
                    else if (i == 1) vertices = &fanVertices;
                    else vertices = &pentagonVertices;

                    glBufferData(GL_ARRAY_BUFFER, vertices->size() * sizeof(float),
                        vertices->data(), GL_STATIC_DRAW);
                    glVertexAttribPointer(posAttrib, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
                    glEnableVertexAttribArray(posAttrib);
                }
            }
        }

        // Если вершины еще не инициализированы, создаем их
        if (quadVertices.empty()) {
            sf::Vector2u windowSize = window.getSize();
            float aspectRatio = static_cast<float>(windowSize.x) / windowSize.y;

            quadVertices = createQuadVertices(aspectRatio);
            fanVertices = createTriangleFanVertices(aspectRatio);
            pentagonVertices = createPentagonVertices(aspectRatio);

            // Инициализируем VAO и VBO
            for (int i = 0; i < 3; i++) {
                glBindVertexArray(VAO[i]);
                glBindBuffer(GL_ARRAY_BUFFER, VBO[i]);

                vector<float>* vertices = nullptr;
                if (i == 0) vertices = &quadVertices;
                else if (i == 1) vertices = &fanVertices;
                else vertices = &pentagonVertices;

                glBufferData(GL_ARRAY_BUFFER, vertices->size() * sizeof(float),
                    vertices->data(), GL_STATIC_DRAW);
                glVertexAttribPointer(posAttrib, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
                glEnableVertexAttribArray(posAttrib);
            }
        }

        // Очистка экрана
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Используем нашу шейдерную программу
        glUseProgram(shaderProgram);

        // Рисуем четырехугольник (2 треугольника = 6 вершин)
        glBindVertexArray(VAO[0]);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        // Рисуем веер треугольников (симметричный сектор круга)
        glBindVertexArray(VAO[1]);
        glDrawArrays(GL_TRIANGLE_FAN, 0, fanVertices.size() / 3);

        // Рисуем пятиугольник как TRIANGLE_FAN
        glBindVertexArray(VAO[2]);
        glDrawArrays(GL_TRIANGLE_FAN, 0, pentagonVertices.size() / 3);

        glBindVertexArray(0);

        // Отображаем на экране
        window.display();
    }

    // Очистка ресурсов
    glDeleteVertexArrays(3, VAO);
    glDeleteBuffers(3, VBO);
    glDeleteProgram(shaderProgram);

    return 0;
}