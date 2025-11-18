#include <SFML/Window.hpp>
#include <SFML/OpenGL.hpp>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

using namespace std;

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
        cerr << "Ошибка: Не удалось создать шейдер типа " << shaderType << endl;
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

int main() {
    // Создаем окно с помощью SFML
    sf::Window window(sf::VideoMode(800, 600), "OpenGL with SFML", sf::Style::Default, sf::ContextSettings(32));
    window.setVerticalSyncEnabled(true);

    // Инициализируем GLEW (опционально, для расширений OpenGL)
    // Но с SFML можно работать и без GLEW для базового функционала

    // Включаем Z-буфер
    glEnable(GL_DEPTH_TEST);

    // Пример шейдеров
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
            FragColor = vec4(1.0, 0.5, 0.2, 1.0);
        }
    )";

    // Создаем шейдерную программу
    GLuint shaderProgram = createShaderProgram(vertexShaderSource, fragmentShaderSource);

    if (shaderProgram == 0) {
        cerr << "Не удалось создать шейдерную программу!" << endl;
        return -1;
    }

    cout << "Шейдерная программа успешно создана! ID: " << shaderProgram << endl;

    // Вершины треугольника
    float vertices[] = {
        -0.5f, -0.5f, 0.0f,
         0.5f, -0.5f, 0.0f,
         0.0f,  0.5f, 0.0f
    };

    // Создаем VAO и VBO
    GLuint VAO, VBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Основной цикл
    bool running = true;
    while (running) {
        // Обработка событий
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                running = false;
            }
            else if (event.type == sf::Event::Resized) {
                // Устанавливаем новую область просмотра при изменении размера окна
                glViewport(0, 0, event.size.width, event.size.height);
            }
        }

        // Очистка экрана
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Используем нашу шейдерную программу
        glUseProgram(shaderProgram);

        // Рисуем треугольник
        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, 3);

        // Отображаем на экране
        window.display();
    }

    // Очистка ресурсов
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteProgram(shaderProgram);

    return 0;
}
