#include <chrono>
#include <GLFW/glfw3.h>
#include <glad/glad.h>
#include <SFML/Audio/SoundBuffer.hpp>
#include <utilities/shader.hpp>
#include <glm/vec3.hpp>
#include <iostream>
#include <utilities/timeutils.h>
#include <utilities/mesh.h>
#include <utilities/shapes.h>
#include <utilities/glutils.h>
#include <SFML/Audio/Sound.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <fmt/format.h>
#include "gamelogic.h"
#include "sceneGraph.hpp"
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/transform.hpp>
#include <conio.h>
#include "utilities/imageLoader.hpp"
#include "utilities/glfont.h"

enum KeyFrameAction {
    BOTTOM, TOP
};

#include <timestamps.h>

double padPositionX = 0;
double padPositionZ = 0;

unsigned int currentKeyFrame = 0;
unsigned int previousKeyFrame = 0;

glm::vec3 cameraPosition = glm::vec3(0, 2, 50);

float rotasjonupanddown = 0.0;
float rotasjonfloat = 0.0;
float rotasjonsomething = 0.0;

SceneNode* rootNode;
SceneNode* boxNode;
SceneNode* ballNode;
SceneNode* padNode;

SceneNode* lightLeftNode;
SceneNode* lightRightNode;
SceneNode* lightPadNode;

SceneNode* ballNode2;

SceneNode* textureNode;
double ballRadius = 3.0f;

// These are heap allocated, because they should not be initialised at the start of the programSA
sf::SoundBuffer* buffer;
Gloom::Shader* shader;
Gloom::Shader* texture2dShader;
Gloom::Shader* texture3dShader;
sf::Sound* sound;

const glm::vec3 boxDimensions(180, 90, 90);
const glm::vec3 padDimensions(30, 30, 30);

glm::vec3 ballPosition(0, ballRadius + padDimensions.y, boxDimensions.z / 2);
glm::vec3 ballDirection(1, 1, 0.2f);

CommandLineOptions options;

bool hasStarted        = false;
bool hasLost           = false;
bool jumpedToNextFrame = false;
bool isPaused          = false;

bool mouseLeftPressed   = false;
bool mouseLeftReleased  = false;
bool mouseRightPressed  = false;
bool mouseRightReleased = false;

PNGImage ASCII = loadPNGFile("../res/textures/charmap.png");
PNGImage brick_normalmap = loadPNGFile("../res/textures/Brick03_nrm.png");
PNGImage brick_diffuse = loadPNGFile("../res/textures/Brick03_col.png");
PNGImage brick03_rgh = loadPNGFile("../res/textures/Brick03_rgh.png");

unsigned int GetLoadedImage(PNGImage texture) {
    int target = GL_TEXTURE_2D;
    unsigned int textureid;
    glGenTextures(1, &textureid);
    glBindTexture(target, textureid);
    int level = 0;
    int border = 0;
    glTexImage2D(target, level, GL_RGBA, texture.width, texture.height, border, GL_RGBA, GL_UNSIGNED_BYTE, texture.pixels.data());
    glGenerateMipmap(target);
    glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    return textureid;
}


// Modify if you want the music to start further on in the track. Measured in seconds.
const float debug_startTime = 0;
double totalElapsedTime = debug_startTime;
double gameElapsedTime = debug_startTime;

double mouseSensitivity = 1.0;
double lastMouseX = windowWidth / 2;
double lastMouseY = windowHeight / 2;

void mouseCallback(GLFWwindow* window, double x, double y) {
    int windowWidth, windowHeight;
    glfwGetWindowSize(window, &windowWidth, &windowHeight);
    glViewport(0, 0, windowWidth, windowHeight);

    double deltaX = x - lastMouseX;
    double deltaY = y - lastMouseY;

    padPositionX -= mouseSensitivity * deltaX / windowWidth;
    padPositionZ -= mouseSensitivity * deltaY / windowHeight;

    if (padPositionX > 1) padPositionX = 1;
    if (padPositionX < 0) padPositionX = 0;
    if (padPositionZ > 1) padPositionZ = 1;
    if (padPositionZ < 0) padPositionZ = 0;

    glfwSetCursorPos(window, windowWidth / 2, windowHeight / 2);
}

//// A few lines to help you if you've never used c++ structs
// struct LightSource {
//     bool a_placeholder_value;
// };
// LightSource lightSources[/*Put number of light sources you want here*/];

void initGame(GLFWwindow* window, CommandLineOptions gameOptions) {
    buffer = new sf::SoundBuffer();
    if (!buffer->loadFromFile("../res/Hall of the Mountain King.ogg")) {
        return;
    }

 
    options = gameOptions;

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
    glfwSetCursorPosCallback(window, mouseCallback);

    texture2dShader = new Gloom::Shader();
    texture2dShader->makeBasicShader("../res/shaders/texture.vert", "../res/shaders/texture.frag");
    
    shader = new Gloom::Shader();
    shader->makeBasicShader("../res/shaders/simple.vert", "../res/shaders/simple.frag");
    shader->activate();
    

    // Create meshes
    //Mesh pad = cube(padDimensions, glm::vec2(30, 30), true);
    Mesh pad = sphereCube(padDimensions, glm::vec2(30, 30), true, false, glm::vec3(1), 10.0);

    Mesh box = cube(boxDimensions, glm::vec2(900), true, true);
    Mesh sphere = generateSphere(1.0, 40, 40);
    
    // Fill buffers
    //unsigned int ballVAO = generateBuffer(sphere);

    unsigned int boxVAO  = generateBuffer(box);
    unsigned int padVAO  = generateBuffer(pad);

    int textureid = GetLoadedImage(ASCII);
    int normal = GetLoadedImage(ASCII);
    std::string text = "Start";
    Mesh texture = generateTextGeometryBuffer(text, 39.0 / 29.0, 0.5);
    unsigned int textureVAO = generateBuffer(texture);




    // Construct scene
    rootNode = createSceneNode();
    boxNode  = createSceneNode();
    padNode  = createSceneNode();
    ballNode = createSceneNode();

    textureNode = createSceneNode();
    
    // Construct scene_light
    // Task a)
    lightLeftNode = createSceneNode();
    lightRightNode = createSceneNode();
    lightPadNode = createSceneNode();

    lightLeftNode->nodeType = POINT_LIGHT;
    lightRightNode->nodeType = POINT_LIGHT;
    lightPadNode->nodeType = POINT_LIGHT;
    /// oblig 2 texture
    textureNode->nodeType = GEOMETRY2D;
    boxNode->nodeType = NORMAL_MAPPED_GEOMETRY;

    lightRightNode->id = 0;
    lightPadNode->id = 1;
    lightLeftNode->id = 2;

    textureNode->id = 9;

    //ballNode->id = 10;

    lightRightNode->color = glm::vec3(0.9, 0.9, 0.0);
    lightPadNode->color = glm::vec3(1, 1.0, 1.0);
    lightLeftNode->color = glm::vec3(0.0, 0.9, 0.9);


    lightLeftNode->position = { -30, -20, 0 };
    lightRightNode->position = { 20, -20, 0 };
    lightPadNode->position = { 0, 20, 0 };

    textureNode->position = { float(windowWidth/2.0)-(29*3), float(windowHeight / 2.0), 0.0};
    textureNode->scale = glm::vec3(300.0);

    rootNode->children.push_back(boxNode);
    rootNode->children.push_back(padNode);
    rootNode->children.push_back(ballNode);
    rootNode->children.push_back(textureNode);

    boxNode->children.push_back(lightLeftNode);
    boxNode->children.push_back(lightRightNode);
    padNode->children.push_back(lightPadNode);

    boxNode->vertexArrayObjectID  = boxVAO;
    boxNode->VAOIndexCount        = box.indices.size();

    padNode->vertexArrayObjectID  = padVAO;
    padNode->VAOIndexCount        = pad.indices.size();

    //ballNode->vertexArrayObjectID = ballVAO;
    //ballNode->VAOIndexCount       = sphere.indices.size();

    /// oblig 2
    textureNode->vertexArrayObjectID = textureVAO;
    textureNode->VAOIndexCount = texture.indices.size();
    textureNode->textureID = textureid;

    boxNode->textureID = GetLoadedImage(brick_diffuse);
    boxNode->textureNormal = GetLoadedImage(brick_normalmap);
    boxNode->textureRoughness = GetLoadedImage(brick03_rgh);
    
    
    getTimeDeltaSeconds();

    std::cout << fmt::format("Initialized scene with {} SceneNodes.", totalChildren(rootNode)) << std::endl;

    std::cout << "Ready. Click to start!" << std::endl;
}

void processInput(GLFWwindow* window,float timeDelta) {

    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        cameraPosition.x -= timeDelta * 100.0;
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        cameraPosition.x += timeDelta * 100.0;
    }
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        cameraPosition.z -= timeDelta * 100.0;
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        cameraPosition.z += timeDelta * 100.0;
    }
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
        cameraPosition.y += timeDelta * 100.0;
    }
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
        cameraPosition.y -= timeDelta * 100.0;
    }
    if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) {
        rotasjonsomething -= timeDelta * 100.0;
    }
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) {
        rotasjonsomething -= timeDelta * 100.0;
    }
    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) {
        rotasjonupanddown += timeDelta ;
    }
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
        rotasjonupanddown -= timeDelta ;
    }
    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) {
        rotasjonfloat += timeDelta ;
    }
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
        rotasjonfloat -= timeDelta;
    }
}



void updateFrame(GLFWwindow* window) {


    double timeDelta = getTimeDeltaSeconds();

    processInput(window, timeDelta);

    // Set up camera
    glm::vec3 camera_position = cameraPosition;
    glm::vec3 camera_target(0.0f, 0.0f, -1.0f);
    glm::vec3 camera_up(0.0f, 1.0f, 0.0f);
    glm::mat4 view_matrix = glm::lookAt(camera_position, camera_position + camera_target, camera_up);

    // Set up projection
    float field_of_view = 80.0f;
    float aspect_ratio = float(windowWidth) / float(windowHeight);
    float near_clip_distance = 0.1f;
    float far_clip_distance = 350.0f;
    glm::mat4 projection_matrix = glm::perspective(glm::radians(field_of_view), aspect_ratio, near_clip_distance, far_clip_distance);
    // Set up model
    glm::mat4 model_matrix = glm::mat4(1.0f);
    model_matrix = glm::rotate(model_matrix, rotasjonfloat, glm::vec3(0.0f, 1.0f, 0.0f));
    model_matrix = glm::rotate(model_matrix, rotasjonupanddown, glm::vec3(1.0f, 0.0f, 0.0f));

    // Set up matrices for rendering
    glm::mat4 P = projection_matrix;
    glm::mat4 V = view_matrix;
    glm::mat4 M = model_matrix;

    glm::mat4 identity = glm::mat4(model_matrix);
    updateNodeTransformations(rootNode, identity);
    glUniformMatrix4fv(4, 1, GL_FALSE, glm::value_ptr(P));
    glUniformMatrix4fv(5, 1, GL_FALSE, glm::value_ptr(V));
    glUniform3f(9, cameraPosition.x, cameraPosition.y, cameraPosition.z);

}

void updateNodeTransformations(SceneNode* node, glm::mat4 transformationThusFar) {
    glm::mat4 transformationMatrix =
              glm::translate(node->position)
            * glm::translate(node->referencePoint)
            * glm::rotate(node->rotation.y, glm::vec3(0,1,0))
            * glm::rotate(node->rotation.x, glm::vec3(1,0,0))
            * glm::rotate(node->rotation.z, glm::vec3(0,0,1))
            * glm::scale(node->scale)
            * glm::translate(-node->referencePoint);

    node->currentTransformationMatrix = transformationThusFar * transformationMatrix;

   

    switch (node->nodeType) {
    case GEOMETRY:
        if (node->id == 10) {
            // Send position of ball
            glm::vec3 shadow = node->currentTransformationMatrix * glm::vec4(0, 0, 0, 1);
            glUniform3f(node->id, shadow.x, shadow.y, shadow.z);
        }
        break;
    case POINT_LIGHT: {
        // Send position of and color of lights
        GLint location = shader->getUniformFromName("lights[" + std::to_string(node->id) + "].position");
        GLint colorlocation = shader->getUniformFromName("lights[" + std::to_string(node->id) + "].color");
        glm::vec3 light = node->currentTransformationMatrix * glm::vec4(0, 0, 0, 1);

        glUniform3f(location, light.x, light.y, light.z);
        glUniform3f(colorlocation, node->color.x, node->color.y, node->color.z);
        break;
        }
    case SPOT_LIGHT: break;
    case GEOMETRY2D: {

       
        
        break;
        
    }

    }

    for(SceneNode* child : node->children) {
        updateNodeTransformations(child, node->currentTransformationMatrix);
    }
}

void renderNode(SceneNode* node) {
    

    glUniformMatrix4fv(3, 1, GL_FALSE, glm::value_ptr(node->currentTransformationMatrix));
    // Task 1 c)
    glm::mat3 normalmatrix = glm::transpose(glm::inverse(node->currentTransformationMatrix));
    // Task 1 b)
    glUniformMatrix3fv(13, 1, GL_FALSE, glm::value_ptr(normalmatrix));

    switch(node->nodeType) {
        case GEOMETRY:
            if(node->vertexArrayObjectID != -1) {
                glUniform1i(6, 0);
                glBindVertexArray(node->vertexArrayObjectID);
                glDrawElements(GL_TRIANGLES, node->VAOIndexCount, GL_UNSIGNED_INT, nullptr);
            }
            break;
        case POINT_LIGHT: 

            
            break;
        case SPOT_LIGHT: break;

        case NORMAL_MAPPED_GEOMETRY:

            if (node->vertexArrayObjectID != -1) {
                glUniform1i(6, 1);
                glBindVertexArray(node->vertexArrayObjectID);
                glBindTextureUnit(0, node->textureID);
                glBindTextureUnit(1, node->textureNormal);
                glBindTextureUnit(2, node->textureRoughness);
                glDrawElements(GL_TRIANGLES, node->VAOIndexCount, GL_UNSIGNED_INT, nullptr);
            }
            break;
        case GEOMETRY2D:
            texture2dShader->activate();

            /* glm::vec3 texture = node->currentTransformationMatrix * glm::vec4(0, 0, 0, 1);
            glUniform3f(node->id, texture.x, texture.y, texture.z);*/
            glm::mat4 P = glm::ortho(0.0f, float(windowWidth), 0.0f, float(windowHeight), -1.0f, 350.f);
            glm::mat4 V = glm::mat4(1.0f); //cameraTransform;
            glUniformMatrix4fv(3, 1, GL_FALSE, glm::value_ptr(node->currentTransformationMatrix));
            glUniformMatrix4fv(4, 1, GL_FALSE, glm::value_ptr(P));
            glUniformMatrix4fv(5, 1, GL_FALSE, glm::value_ptr(V));

            glUniform1i(6, 0);
            glBindVertexArray(node->vertexArrayObjectID);
            glBindTextureUnit(0, node->textureID);
            glDrawElements(GL_TRIANGLES, node->VAOIndexCount, GL_UNSIGNED_INT, nullptr);


            shader->activate();
            break;
    }

    for(SceneNode* child : node->children) {
        renderNode(child);
    }
}

void renderFrame(GLFWwindow* window) {
    int windowWidth, windowHeight;
    glfwGetWindowSize(window, &windowWidth, &windowHeight);
    glViewport(0, 0, windowWidth, windowHeight);
    renderNode(rootNode);

}
