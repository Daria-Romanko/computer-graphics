#include <GL/glew.h>
#include <SFML/OpenGL.hpp>
#include <SFML/Window.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <cstring>
#include <cstdint>
#include <random>
#include "shader_utils.h"
#include "model.h"
#include "camera.h"

int main() {
	sf::Window window(sf::VideoMode({ 800, 600 }), "Solar System with Interactive Camera", sf::Style::Default);
	window.setFramerateLimit(60);

	window.setMouseCursorVisible(false);
	window.setMouseCursorGrabbed(true);

	glewExperimental = GL_TRUE;
	if (glewInit() != GLEW_OK) {
		std::cerr << "Failed to initialize GLEW" << std::endl;
		return -1;
	}

	glEnable(GL_DEPTH_TEST);

	Camera camera(glm::vec3(0.0f, 5.0f, 20.0f));

	bool firstMouse = true;
	sf::Vector2i lastMousePos(400, 300);
	sf::Mouse::setPosition({ 400, 300 }, window);

	GLuint shaderProgram = CreateShaderProgramFromFiles("basic.vert", "basic.frag");
	if (!shaderProgram) {
		std::cerr << "Failed to create shader program" << std::endl;
		return -1;
	}

	Model centralOilDrum;
	centralOilDrum.name = "Central Oil Drum";

	if (!LoadOBJModel("oil-drum_col.obj", centralOilDrum)) {
		std::cerr << "ERROR: Failed to load oil-drum_col.obj\n";
		return -1;
	}
	if (!InitializeModelGL(centralOilDrum, "oil-drum_col_texture.jpg")) {
		std::cerr << "ERROR: Failed to initialize central oil drum model\n";
		return -1;
	}

	Model fireExtinguisherModel;
	fireExtinguisherModel.name = "Fire Extinguisher";

	if (!LoadOBJModel("fire_extinguisher.obj", fireExtinguisherModel)) {
		std::cerr << "ERROR: Failed to load fire_extinguisher.obj\n";
		return -1;
	}
	if (!InitializeModelGL(fireExtinguisherModel, "fire_extinguisher_texture.jpg")) {
		std::cerr << "ERROR: Failed to initialize fire extinguisher model\n";
		return -1;
	}

	const int NUM_PLANETS = 100;
	std::vector<glm::vec3> planetPositions;
	std::vector<float> planetRotations;

	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_real_distribution<float> angleDist(0.0f, 2.0f * 3.14159265f);
	std::uniform_real_distribution<float> heightDist(-2.0f, 2.0f);

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
	}

	GLuint modelLoc = glGetUniformLocation(shaderProgram, "model");
	GLuint viewLoc = glGetUniformLocation(shaderProgram, "view");
	GLuint projectionLoc = glGetUniformLocation(shaderProgram, "projection");

	sf::Clock clock;
	float lastTime = 0.0f;
	float time = 0.0f;
	bool running = true;

	while (running) {
		float currentTime = clock.getElapsedTime().asSeconds();
		float deltaTime = currentTime - lastTime;
		lastTime = currentTime;
		time += deltaTime;

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
				else if (const auto* mouseMove = event->getIf<sf::Event::MouseMoved>()) {
					if (firstMouse) {
						lastMousePos = sf::Vector2i(mouseMove->position.x, mouseMove->position.y);
						firstMouse = false;
						continue;
					}

					float xoffset = mouseMove->position.x - lastMousePos.x;
					float yoffset = lastMousePos.y - mouseMove->position.y;

					lastMousePos = sf::Vector2i(mouseMove->position.x, mouseMove->position.y);

					camera.ProcessMouseMovement(xoffset, yoffset);

					sf::Mouse::setPosition({ 400, 300 }, window);
					lastMousePos = sf::Vector2i(400, 300);
				}
				else if (const auto* mouseWheel = event->getIf<sf::Event::MouseWheelScrolled>()) {
					if (mouseWheel->wheel == sf::Mouse::Wheel::Vertical) {
						camera.ProcessMouseScroll(mouseWheel->delta);
					}
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
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Q))
			camera.ProcessKeyboard(ROTATE_LEFT, deltaTime);
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::E))
			camera.ProcessKeyboard(ROTATE_RIGHT, deltaTime);

		glClearColor(0.0f, 0.0f, 0.05f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glUseProgram(shaderProgram);

		glm::mat4 view = camera.GetViewMatrix();
		glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), 800.0f / 600.0f, 0.1f, 200.0f);

		glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
		glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));

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

		if (fireExtinguisherModel.vao != 0) {
			glBindVertexArray(fireExtinguisherModel.vao);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, fireExtinguisherModel.texture);

			const int NUM_NEAR_EXT = 5;
			float radius = 5.0f;

			for (int instance = 0; instance < NUM_NEAR_EXT; instance++) {
				glm::mat4 modelMat = glm::mat4(1.0f);

				float angle = (instance * 72.0f) * glm::pi<float>() / 180.0f;

				glm::vec3 position(
					radius * std::cos(angle + time * 0.5f),
					0.0f,
					radius * std::sin(angle + time * 0.5f)
				);

				modelMat = glm::translate(modelMat, position);
				modelMat = glm::rotate(modelMat, time * 1.0f + instance * 0.3f, glm::vec3(0, 1, 0));
				modelMat = glm::scale(modelMat, glm::vec3(0.8f, 0.8f, 0.8f));

				glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelMat));
				glDrawElements(GL_TRIANGLES, fireExtinguisherModel.indexCount, GL_UNSIGNED_INT, 0);
			}

			glBindVertexArray(0);
		}

		if (fireExtinguisherModel.vao != 0) {
			glBindVertexArray(fireExtinguisherModel.vao);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, fireExtinguisherModel.texture);

			for (int i = 0; i < NUM_PLANETS; i++) {
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
				glDrawElements(GL_TRIANGLES, fireExtinguisherModel.indexCount, GL_UNSIGNED_INT, 0);
			}

			glBindVertexArray(0);
		}

		window.display();
	}

	if (centralOilDrum.vao != 0) {
		glDeleteVertexArrays(1, &centralOilDrum.vao);
		glDeleteBuffers(1, &centralOilDrum.vbo);
		glDeleteBuffers(1, &centralOilDrum.ebo);
		glDeleteTextures(1, &centralOilDrum.texture);
	}

	if (fireExtinguisherModel.vao != 0) {
		glDeleteVertexArrays(1, &fireExtinguisherModel.vao);
		glDeleteBuffers(1, &fireExtinguisherModel.vbo);
		glDeleteBuffers(1, &fireExtinguisherModel.ebo);
		glDeleteTextures(1, &fireExtinguisherModel.texture);
	}

	glDeleteProgram(shaderProgram);
	return 0;
}