#include <GL/glew.h>

#include <SFML/Graphics.hpp>
#include <SFML/OpenGL.hpp>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <optional>
#include <iostream>
#include <string>
#include <vector>

#include "shader_utils.h"
#include "model.h"
#include "camera.h"

#include <imgui.h>
#include <imgui-SFML.h>

constexpr int MAX_POINT_LIGHTS = 8;
constexpr int MAX_SPOT_LIGHTS = 8;

struct DirLightData {
    glm::vec3 direction;
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;
};

struct PointLightData {
    glm::vec3 position;
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;
    glm::vec3 attenuation;
};

struct SpotLightData {
    glm::vec3 position;
    glm::vec3 direction;
    float innerCutOff;
    float outerCutOff;
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;
    glm::vec3 attenuation;
};

struct SceneObject {
    Model model;
    glm::vec3 position;
    glm::vec3 rotation;
    glm::vec3 scale;
    std::string name;
};

void UploadPointLights(GLuint program, const PointLightData* pointLights, int count)
{
    for (int i = 0; i < count; ++i) {
        std::string idx = "pointLights[" + std::to_string(i) + "].";
        glUniform3fv(glGetUniformLocation(program, (idx + "position").c_str()),
            1, glm::value_ptr(pointLights[i].position));
        glUniform3fv(glGetUniformLocation(program, (idx + "ambient").c_str()),
            1, glm::value_ptr(pointLights[i].ambient));
        glUniform3fv(glGetUniformLocation(program, (idx + "diffuse").c_str()),
            1, glm::value_ptr(pointLights[i].diffuse));
        glUniform3fv(glGetUniformLocation(program, (idx + "specular").c_str()),
            1, glm::value_ptr(pointLights[i].specular));
        glUniform3fv(glGetUniformLocation(program, (idx + "attenuation").c_str()),
            1, glm::value_ptr(pointLights[i].attenuation));
    }
}

void UploadSpotLights(GLuint program, const SpotLightData* spotLights, int count)
{
    for (int i = 0; i < count; ++i) {
        std::string idx = "spotLights[" + std::to_string(i) + "].";
        glUniform3fv(glGetUniformLocation(program, (idx + "position").c_str()),
            1, glm::value_ptr(spotLights[i].position));
        glUniform3fv(glGetUniformLocation(program, (idx + "direction").c_str()),
            1, glm::value_ptr(spotLights[i].direction));
        glUniform1f(glGetUniformLocation(program, (idx + "innerCutOff").c_str()),
            spotLights[i].innerCutOff);
        glUniform1f(glGetUniformLocation(program, (idx + "outerCutOff").c_str()),
            spotLights[i].outerCutOff);
        glUniform3fv(glGetUniformLocation(program, (idx + "ambient").c_str()),
            1, glm::value_ptr(spotLights[i].ambient));
        glUniform3fv(glGetUniformLocation(program, (idx + "diffuse").c_str()),
            1, glm::value_ptr(spotLights[i].diffuse));
        glUniform3fv(glGetUniformLocation(program, (idx + "specular").c_str()),
            1, glm::value_ptr(spotLights[i].specular));
        glUniform3fv(glGetUniformLocation(program, (idx + "attenuation").c_str()),
            1, glm::value_ptr(spotLights[i].attenuation));
    }
}

int main()
{
    sf::VideoMode desktop = sf::VideoMode::getDesktopMode();

    sf::RenderWindow window(desktop, "bananaCat test with Toon Shading", sf::Style::None);

    window.setFramerateLimit(60);
    window.setActive(true);

    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        std::cerr << "Failed to initialize GLEW\n";
        return -1;
    }

    glEnable(GL_DEPTH_TEST);

    if (!ImGui::SFML::Init(window)) {
        std::cerr << "Failed to initialize ImGui-SFML\n";
        return -1;
    }

    Camera camera(glm::vec3(0.0f, 4.0f, 20.0f));

    DirLightData dirLightData{
        glm::vec3(-0.2f, -1.0f, -0.3f),
        glm::vec3(0.2f),
        glm::vec3(0.7f),
        glm::vec3(1.0f)
    };

    int pointLightCount = 1;
    PointLightData pointLights[MAX_POINT_LIGHTS]{};

    pointLights[0] = {
        glm::vec3(3.0f, 5.0f, 3.0f),      // position
        glm::vec3(0.1f),                  // ambient
        glm::vec3(0.8f),                  // diffuse
        glm::vec3(1.0f),                  // specular
        glm::vec3(1.0f, 0.09f, 0.032f)    // attenuation
    };

    int spotLightCount = 1;
    SpotLightData spotLights[MAX_SPOT_LIGHTS]{};

    spotLights[0] = {
        camera.Position,                  // position
        camera.Front,                     // direction
        glm::cos(glm::radians(12.5f)),    // innerCutOff
        glm::cos(glm::radians(17.5f)),    // outerCutOff
        glm::vec3(0.0f),                  // ambient
        glm::vec3(1.0f),                  // diffuse
        glm::vec3(1.0f),                  // specular
        glm::vec3(1.0f, 0.09f, 0.032f)    // attenuation
    };

    GLuint shaderProgram = CreateShaderProgramFromFiles("basic.vert", "basic.frag");
    if (!shaderProgram) {
        std::cerr << "Failed to create shader program\n";
        return -1;
    }

    glUseProgram(shaderProgram);

    GLint modelLoc = glGetUniformLocation(shaderProgram, "model");
    GLint viewLoc = glGetUniformLocation(shaderProgram, "view");
    GLint projLoc = glGetUniformLocation(shaderProgram, "projection");
    GLint viewPosLoc = glGetUniformLocation(shaderProgram, "viewPos");
    GLint normalMatrixLoc = glGetUniformLocation(shaderProgram, "normalMatrix");

    GLint numPointLightsLoc = glGetUniformLocation(shaderProgram, "numPointLights");
    GLint numSpotLightsLoc = glGetUniformLocation(shaderProgram, "numSpotLights");

    GLint matDiffLoc = glGetUniformLocation(shaderProgram, "material.diffuse");
    GLint matSpecLoc = glGetUniformLocation(shaderProgram, "material.specular");
    GLint matShineLoc = glGetUniformLocation(shaderProgram, "material.shininess");

    GLint dirDirLoc = glGetUniformLocation(shaderProgram, "dirLight.direction");
    GLint dirAmbLoc = glGetUniformLocation(shaderProgram, "dirLight.ambient");
    GLint dirDiffLoc = glGetUniformLocation(shaderProgram, "dirLight.diffuse");
    GLint dirSpecLoc = glGetUniformLocation(shaderProgram, "dirLight.specular");

    // Toon shading uniforms
    GLint useToonShadingLoc = glGetUniformLocation(shaderProgram, "u_useToonShading");
    GLint toonLevelsLoc = glGetUniformLocation(shaderProgram, "u_toonLevels");
    GLint toonSpecularSizeLoc = glGetUniformLocation(shaderProgram, "u_toonSpecularSize");
    GLint toonEdgeThresholdLoc = glGetUniformLocation(shaderProgram, "u_toonEdgeThreshold");
    GLint outlineColorLoc = glGetUniformLocation(shaderProgram, "u_outlineColor");

    glUniform1i(matDiffLoc, 0);
    glUniform3f(matSpecLoc, 1.0f, 1.0f, 1.0f);
    glUniform1f(matShineLoc, 32.0f);

    glUniform3fv(dirDirLoc, 1, glm::value_ptr(dirLightData.direction));
    glUniform3fv(dirAmbLoc, 1, glm::value_ptr(dirLightData.ambient));
    glUniform3fv(dirDiffLoc, 1, glm::value_ptr(dirLightData.diffuse));
    glUniform3fv(dirSpecLoc, 1, glm::value_ptr(dirLightData.specular));

    glUniform1i(numPointLightsLoc, pointLightCount);
    glUniform1i(numSpotLightsLoc, spotLightCount);

    // Initialize toon shading uniforms
    bool useToonShading = false;
    int toonLevels = 4;
    float toonSpecularSize = 0.1f;
    float toonEdgeThreshold = 0.2f;
    glm::vec3 outlineColor = glm::vec3(0.0f, 0.0f, 0.0f);

    glUniform1i(useToonShadingLoc, useToonShading ? 1 : 0);
    glUniform1i(toonLevelsLoc, toonLevels);
    glUniform1f(toonSpecularSizeLoc, toonSpecularSize);
    glUniform1f(toonEdgeThresholdLoc, toonEdgeThreshold);
    glUniform3fv(outlineColorLoc, 1, glm::value_ptr(outlineColor));

    glUseProgram(0);

    std::vector<SceneObject> sceneObjects;

    auto AddObjToScene = [&](const char* path,
        const char* name,
        glm::vec3 pos,
        glm::vec3 rotDeg,
        glm::vec3 scale) -> bool
        {
            SceneObject obj;
            obj.name = name;
            obj.position = pos;
            obj.rotation = rotDeg;
            obj.scale = scale;

            if (!LoadOBJModel(path, obj.model) || !InitializeModelGL(obj.model)) {
                std::cerr << "Failed to load/init: " << path << "\n";
                return false;
            }

            sceneObjects.emplace_back(std::move(obj));
            return true;
        };

    AddObjToScene("models/bananaCat.obj", "bananaCat", { -0.3f,-1,7 }, { 0,-1,0 }, { 1.5f,1.5f,1.5f });
    AddObjToScene("models/tree.obj", "tree", { 5, 0,7.8f }, { 0,-40,0 }, { 2,2,2 });
    AddObjToScene("models/witness.obj", "witness", { -4.5f,-1.1f, 7 }, { 0,76,0 }, { 0.15f,0.15f,0.15f });
    AddObjToScene("models/cow.obj", "cow", { -2.5f,3.5f, -7.6f }, { -9,-26,17 }, { 0.1f,0.1f,0.1f });
    AddObjToScene("models/UFO.obj", "ufo", { -3.3f,7, -7 }, { 10,-65,-2 }, { 1,1,1 });
    AddObjToScene("models/pepe.obj", "pepe", { 14,16, -65 }, { 8.5f,-15,0 }, { 10,10,10 });

    int selectedObject = 0;

    char modelPathBuffer[256] = "models/bananaCat.obj";
    char modelNameBuffer[64] = "newModel";

    sf::Clock deltaClock;
    bool running = true;

    while (running) {
        sf::Time dt = deltaClock.restart();
        float deltaTime = dt.asSeconds();

        while (const std::optional<sf::Event> event = window.pollEvent()) {
            if (!event) continue;

            ImGui::SFML::ProcessEvent(window, *event);

            if (event->is<sf::Event::Closed>()) {
                running = false;
            }
            else if (const auto* keyEvent = event->getIf<sf::Event::KeyPressed>()) {
                if (keyEvent->scancode == sf::Keyboard::Scancode::Escape) {
                    running = false;
                }
                // Toggle toon shading with T key
                else if (keyEvent->scancode == sf::Keyboard::Scancode::T) {
                    useToonShading = !useToonShading;
                    std::cout << "Toon shading: " << (useToonShading ? "ON" : "OFF") << std::endl;
                }
            }
            else if (const auto* mouseWheel = event->getIf<sf::Event::MouseWheelScrolled>()) {
                if (mouseWheel->wheel == sf::Mouse::Wheel::Vertical) {
                    camera.ProcessMouseScroll(mouseWheel->delta);
                }
            }
        }

        ImGui::SFML::Update(window, dt);

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

        if (yawOffset != 0.0f || pitchOffset != 0.0f)
            camera.ProcessMouseMovement(yawOffset, pitchOffset);

        ImGui::Begin("Lights");

        if (ImGui::CollapsingHeader("Directional light", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::DragFloat3("Direction", glm::value_ptr(dirLightData.direction), 0.01f);
            ImGui::ColorEdit3("Ambient", glm::value_ptr(dirLightData.ambient));
            ImGui::ColorEdit3("Diffuse", glm::value_ptr(dirLightData.diffuse));
            ImGui::ColorEdit3("Specular", glm::value_ptr(dirLightData.specular));
        }

        if (ImGui::CollapsingHeader("Point lights", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Text("Count: %d / %d", pointLightCount, MAX_POINT_LIGHTS);
            if (ImGui::Button("Add point light") && pointLightCount < MAX_POINT_LIGHTS) {
                pointLights[pointLightCount] = pointLights[0];
                pointLights[pointLightCount].position += glm::vec3(2.0f * pointLightCount, 0.0f, 0.0f);
                ++pointLightCount;
            }

            for (int i = 0; i < pointLightCount; ++i) {
                ImGui::PushID(i);
                if (ImGui::TreeNode("Point light", "Point %d", i)) {
                    ImGui::DragFloat3("Position", glm::value_ptr(pointLights[i].position), 0.1f);
                    ImGui::ColorEdit3("Ambient", glm::value_ptr(pointLights[i].ambient));
                    ImGui::ColorEdit3("Diffuse", glm::value_ptr(pointLights[i].diffuse));
                    ImGui::ColorEdit3("Specular", glm::value_ptr(pointLights[i].specular));
                    ImGui::DragFloat3("Attenuation",
                        glm::value_ptr(pointLights[i].attenuation), 0.001f, 0.0f, 10.0f);
                    ImGui::TreePop();
                }
                ImGui::PopID();
            }
        }

        if (ImGui::CollapsingHeader("Spot lights", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Text("Count: %d / %d", spotLightCount, MAX_SPOT_LIGHTS);
            if (ImGui::Button("Add spot light") && spotLightCount < MAX_SPOT_LIGHTS) {
                spotLights[spotLightCount] = spotLights[0];
                ++spotLightCount;
            }

            for (int i = 0; i < spotLightCount; ++i) {
                ImGui::PushID(100 + i);
                if (ImGui::TreeNode("Spot light", "Spot %d", i)) {
                    ImGui::DragFloat3("Position", glm::value_ptr(spotLights[i].position), 0.1f);
                    ImGui::DragFloat3("Direction", glm::value_ptr(spotLights[i].direction), 0.01f);

                    float innerDeg = glm::degrees(std::acos(glm::clamp(spotLights[i].innerCutOff, -1.0f, 1.0f)));
                    float outerDeg = glm::degrees(std::acos(glm::clamp(spotLights[i].outerCutOff, -1.0f, 1.0f)));

                    if (ImGui::DragFloat("Inner angle", &innerDeg, 0.5f, 0.0f, 90.0f) ||
                        ImGui::DragFloat("Outer angle", &outerDeg, 0.5f, 0.0f, 90.0f)) {

                        innerDeg = glm::clamp(innerDeg, 0.0f, 90.0f);
                        outerDeg = glm::clamp(outerDeg, 0.0f, innerDeg);

                        spotLights[i].innerCutOff = glm::cos(glm::radians(innerDeg));
                        spotLights[i].outerCutOff = glm::cos(glm::radians(outerDeg));
                    }

                    ImGui::ColorEdit3("Ambient", glm::value_ptr(spotLights[i].ambient));
                    ImGui::ColorEdit3("Diffuse", glm::value_ptr(spotLights[i].diffuse));
                    ImGui::ColorEdit3("Specular", glm::value_ptr(spotLights[i].specular));
                    ImGui::DragFloat3("Attenuation",
                        glm::value_ptr(spotLights[i].attenuation), 0.001f, 0.0f, 10.0f);

                    ImGui::TreePop();
                }
                ImGui::PopID();
            }
        }

        ImGui::End();

        ImGui::Begin("Toon Shading");

        if (ImGui::CollapsingHeader("Toon Shading Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
            if (ImGui::Checkbox("Enable Toon Shading", &useToonShading)) {
            }

            ImGui::Text("Press 'T' key to toggle Toon Shading");
            ImGui::Separator();

            if (useToonShading) {
                ImGui::Separator();
                ImGui::Text("Toon shading creates a cartoon/cel-shaded look");
                ImGui::Text("by quantizing colors and adding dark outlines.");
                ImGui::Text("Color Levels: controls color quantization (lower = more cartoonish)");
                ImGui::Text("Specular Size: controls size of highlights");
                ImGui::Text("Edge Threshold: controls outline thickness");
            }
        }

        ImGui::End();

        ImGui::Begin("Scene");

        ImGui::Text("Objects: %d", (int)sceneObjects.size());
        ImGui::Separator();

        for (int i = 0; i < (int)sceneObjects.size(); ++i) {
            ImGui::PushID(i);
            bool isSelected = (i == selectedObject);
            if (ImGui::Selectable(sceneObjects[i].name.c_str(), isSelected)) {
                selectedObject = i;
            }
            ImGui::PopID();
        }

        ImGui::Separator();

        if (!sceneObjects.empty() &&
            selectedObject >= 0 &&
            selectedObject < (int)sceneObjects.size()) {

            SceneObject& obj = sceneObjects[selectedObject];

            ImGui::Text("Selected: %s", obj.name.c_str());
            ImGui::DragFloat3("Position", glm::value_ptr(obj.position), 0.1f);
            ImGui::DragFloat3("Rotation", glm::value_ptr(obj.rotation), 0.5f);
            ImGui::DragFloat3("Scale", glm::value_ptr(obj.scale), 0.01f, 0.01f, 100.0f);
        }

        ImGui::Separator();
        ImGui::Text("Load new model");

        ImGui::InputText("Path", modelPathBuffer, IM_ARRAYSIZE(modelPathBuffer));
        ImGui::InputText("Name", modelNameBuffer, IM_ARRAYSIZE(modelNameBuffer));

        if (ImGui::Button("Load OBJ")) {
            SceneObject newObj;
            newObj.name = modelNameBuffer[0] ? modelNameBuffer : "Object";
            newObj.position = glm::vec3(0.0f);
            newObj.rotation = glm::vec3(0.0f);
            newObj.scale = glm::vec3(1.0f);

            if (!LoadOBJModel(modelPathBuffer, newObj.model) ||
                !InitializeModelGL(newObj.model)) {
                std::cerr << "Failed to load model: " << modelPathBuffer << "\n";
            }
            else {
                sceneObjects.push_back(newObj);
                selectedObject = (int)sceneObjects.size() - 1;
            }
        }

        ImGui::End();

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, window.getSize().x, window.getSize().y);

        glEnable(GL_DEPTH_TEST);
        glDepthMask(GL_TRUE);
        glDisable(GL_SCISSOR_TEST);
        glDisable(GL_BLEND);

        glClearColor(0.0f, 0.0f, 0.05f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(shaderProgram);

        // Update toon shading uniforms
        glUniform1i(useToonShadingLoc, useToonShading ? 1 : 0);
        glUniform1i(toonLevelsLoc, toonLevels);
        glUniform1f(toonSpecularSizeLoc, toonSpecularSize);
        glUniform1f(toonEdgeThresholdLoc, toonEdgeThreshold);
        glUniform3fv(outlineColorLoc, 1, glm::value_ptr(outlineColor));

        glm::mat4 view = camera.GetViewMatrix();

        sf::Vector2u size = window.getSize();
        float aspect = static_cast<float>(size.x) / static_cast<float>(size.y);

        glm::mat4 projection = glm::perspective(
            glm::radians(camera.Zoom),
            aspect,
            0.1f,
            200.0f
        );

        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

        glUniform3fv(viewPosLoc, 1, glm::value_ptr(camera.Position));

        spotLights[0].position = camera.Position;
        spotLights[0].direction = camera.Front;

        glUniform3fv(dirDirLoc, 1, glm::value_ptr(dirLightData.direction));
        glUniform3fv(dirAmbLoc, 1, glm::value_ptr(dirLightData.ambient));
        glUniform3fv(dirDiffLoc, 1, glm::value_ptr(dirLightData.diffuse));
        glUniform3fv(dirSpecLoc, 1, glm::value_ptr(dirLightData.specular));

        glUniform1i(numPointLightsLoc, pointLightCount);
        glUniform1i(numSpotLightsLoc, spotLightCount);

        UploadPointLights(shaderProgram, pointLights, pointLightCount);
        UploadSpotLights(shaderProgram, spotLights, spotLightCount);

        for (SceneObject& obj : sceneObjects) {
            if (obj.model.vao == 0) continue;

            glm::mat4 modelMat(1.0f);
            modelMat = glm::translate(modelMat, obj.position);
            modelMat = glm::rotate(modelMat, glm::radians(obj.rotation.y), glm::vec3(0, 1, 0));
            modelMat = glm::rotate(modelMat, glm::radians(obj.rotation.x), glm::vec3(1, 0, 0));
            modelMat = glm::rotate(modelMat, glm::radians(obj.rotation.z), glm::vec3(0, 0, 1));
            modelMat = glm::scale(modelMat, obj.scale);

            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelMat));

            glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(modelMat)));
            glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));

            DrawModel(obj.model);
        }

        glUseProgram(0);
        ImGui::SFML::Render(window);
        window.display();
    }

    for (SceneObject& obj : sceneObjects) {
        DestroyModelGL(obj.model);
    }
    glDeleteProgram(shaderProgram);
    ImGui::SFML::Shutdown();

    return 0;
}
