#include <GL/glew.h>
#include <SFML/OpenGL.hpp>
#include <SFML/Window.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <optional>
#include <iostream>

#include "shader_utils.h"
#include "model.h"
#include "camera.h"

int main()
{
    sf::Window window(sf::VideoMode({ 800u, 600u }), "bananaCat test", sf::Style::Default);
    window.setFramerateLimit(60);

    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        std::cerr << "Failed to initialize GLEW\n";
        return -1;
    }

    glEnable(GL_DEPTH_TEST);

    Camera camera(glm::vec3(0.0f, 4.0f, 20.0f));

    GLuint shaderProgram = CreateShaderProgramFromFiles("basic.vert", "basic.frag");
    if (!shaderProgram) {
        std::cerr << "Failed to create shader program\n";
        return -1;
    }

    Model bananaCat;
    bananaCat.name = "bananaCat";
    if (!LoadOBJModel("models/bananaCat.obj", bananaCat) || !InitializeModelGL(bananaCat))
    {
        std::cerr << "Failed to load or init bananaCat\n";
        return -1;
    }

    glUseProgram(shaderProgram);

    GLint modelLoc = glGetUniformLocation(shaderProgram, "model");
    GLint viewLoc = glGetUniformLocation(shaderProgram, "view");
    GLint projLoc = glGetUniformLocation(shaderProgram, "projection");
    GLint viewPosLoc = glGetUniformLocation(shaderProgram, "viewPos");

    GLint matDiffLoc = glGetUniformLocation(shaderProgram, "material.diffuse");
    GLint matSpecLoc = glGetUniformLocation(shaderProgram, "material.specular");
    GLint matShineLoc = glGetUniformLocation(shaderProgram, "material.shininess");

    GLint dirDirLoc = glGetUniformLocation(shaderProgram, "dirLight.direction");
    GLint dirAmbLoc = glGetUniformLocation(shaderProgram, "dirLight.ambient");
    GLint dirDiffLoc = glGetUniformLocation(shaderProgram, "dirLight.diffuse");
    GLint dirSpecLoc = glGetUniformLocation(shaderProgram, "dirLight.specular");

    glUniform1i(matDiffLoc, 0);

    glUniform3f(matSpecLoc, 1.0f, 1.0f, 1.0f);
    glUniform1f(matShineLoc, 32.0f);

    glUniform3f(dirDirLoc, -0.2f, -1.0f, -0.3f);
    glUniform3f(dirAmbLoc, 0.2f, 0.2f, 0.2f);
    glUniform3f(dirDiffLoc, 0.7f, 0.7f, 0.7f);
    glUniform3f(dirSpecLoc, 1.0f, 1.0f, 1.0f);

    glUseProgram(0);

    sf::Clock clock;
    float lastTime = 0.0f;
    bool running = true;

    while (running) {
        float currentTime = clock.getElapsedTime().asSeconds();
        float deltaTime = currentTime - lastTime;
        lastTime = currentTime;

        while (const std::optional<sf::Event> event = window.pollEvent()) {
            if (!event) continue;

            if (event->is<sf::Event::Closed>()) {
                running = false;
            }
            else if (const auto* keyEvent = event->getIf<sf::Event::KeyPressed>()) {
                if (keyEvent->scancode == sf::Keyboard::Scancode::Escape) {
                    running = false;
                }
            }
            else if (const auto* mouseWheel = event->getIf<sf::Event::MouseWheelScrolled>()) {
                if (mouseWheel->wheel == sf::Mouse::Wheel::Vertical) {
                    camera.ProcessMouseScroll(mouseWheel->delta);
                }
            }
        }

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W))
            camera.ProcessKeyboard(FORWARD, deltaTime);
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S))
            camera.ProcessKeyboard(BACKWARD, deltaTime);
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A))
            camera.ProcessKeyboard(LEFT, deltaTime);
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D))
            camera.ProcessKeyboard(RIGHT, deltaTime);
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Space))
            camera.ProcessKeyboard(UP, deltaTime);
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LShift))
            camera.ProcessKeyboard(DOWN, deltaTime);

        const float lookSpeed = 100.0f;
        float yawOffset = 0.0f;
        float pitchOffset = 0.0f;

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left))
            yawOffset -= lookSpeed * deltaTime;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right))
            yawOffset += lookSpeed * deltaTime;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Up))
            pitchOffset += lookSpeed * deltaTime;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Down))
            pitchOffset -= lookSpeed * deltaTime;

        if (yawOffset != 0.0f || pitchOffset != 0.0f) {
            camera.ProcessMouseMovement(yawOffset, pitchOffset);
        }

        glClearColor(0.0f, 0.0f, 0.05f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(shaderProgram);

        glm::mat4 view = camera.GetViewMatrix();
        glm::mat4 projection = glm::perspective(
            glm::radians(camera.Zoom),
            800.0f / 600.0f,
            0.1f,
            200.0f
        );

        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

        glUniform3fv(viewPosLoc, 1, glm::value_ptr(camera.Position));

        if (bananaCat.vao != 0) {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, bananaCat.texture);

            glm::mat4 modelMat(1.0f);
            modelMat = glm::translate(modelMat, glm::vec3(0.0f, -1.0f, 0.0f));
            modelMat = glm::scale(modelMat, glm::vec3(2.0f));

            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelMat));

            glBindVertexArray(bananaCat.vao);
            glDrawElements(GL_TRIANGLES, bananaCat.indexCount, GL_UNSIGNED_INT, nullptr);
            glBindVertexArray(0);
        }

        glUseProgram(0);
        window.display();
    }

    DestroyModelGL(bananaCat);
    glDeleteProgram(shaderProgram);

    return 0;
}
