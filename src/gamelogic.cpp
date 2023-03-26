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

// These are heap allocated, because they should not be initialised at the start of the program
sf::SoundBuffer* buffer;
Gloom::Shader* shader;
Gloom::Shader* texture2dShader;
Gloom::Shader* texture3dShader;
sf::Sound* sound;

const glm::vec3 boxDimensions(180, 90, 90);
const glm::vec3 padDimensions(30, 3, 40);

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
    Mesh pad = cube(padDimensions, glm::vec2(30, 40), true);
    Mesh box = cube(boxDimensions, glm::vec2(90), true, true);
    Mesh sphere = generateSphere(1.0, 40, 40);
    
    // Fill buffers
    unsigned int ballVAO = generateBuffer(sphere);
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

    ballNode->id = 10;

    lightRightNode->color = glm::vec3(0.9, 0.9, 0.0);
    lightPadNode->color = glm::vec3(1, 1.0, 1.0);
    lightLeftNode->color = glm::vec3(0.0, 0.9, 0.9);

    //lightRightNode->color = glm::vec3(1);
    //lightPadNode->color = glm::vec3(1);
    //lightLeftNode->color = glm::vec3(1);

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

    ballNode->vertexArrayObjectID = ballVAO;
    ballNode->VAOIndexCount       = sphere.indices.size();

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

void updateFrame(GLFWwindow* window) {
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    double timeDelta = getTimeDeltaSeconds();

    const float ballBottomY = boxNode->position.y - (boxDimensions.y / 2) + ballRadius + padDimensions.y;
    const float ballTopY = boxNode->position.y + (boxDimensions.y / 2) - ballRadius;
    const float BallVerticalTravelDistance = ballTopY - ballBottomY;

    const float cameraWallOffset = 30; // Arbitrary addition to prevent ball from going too much into camera

    const float ballMinX = boxNode->position.x - (boxDimensions.x / 2) + ballRadius;
    const float ballMaxX = boxNode->position.x + (boxDimensions.x / 2) - ballRadius;
    const float ballMinZ = boxNode->position.z - (boxDimensions.z / 2) + ballRadius;
    const float ballMaxZ = boxNode->position.z + (boxDimensions.z / 2) - ballRadius - cameraWallOffset;

    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_1)) {
        mouseLeftPressed = true;
        mouseLeftReleased = false;
    }
    else {
        mouseLeftReleased = mouseLeftPressed;
        mouseLeftPressed = false;
    }
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_2)) {
        mouseRightPressed = true;
        mouseRightReleased = false;
    }
    else {
        mouseRightReleased = mouseRightPressed;
        mouseRightPressed = false;
    }

    if (!hasStarted) {
        textureNode->position = { float(windowWidth / 2.0) - (29 * 3), float(windowHeight / 2.0), 0.0 };
        if (mouseLeftPressed) {
            if (options.enableMusic) {
                sound = new sf::Sound();
                sound->setBuffer(*buffer);
                sf::Time startTime = sf::seconds(debug_startTime);
                sound->setPlayingOffset(startTime);
                sound->play();
            }
            totalElapsedTime = debug_startTime;
            gameElapsedTime = debug_startTime;
            hasStarted = true;
            
        }

        ballPosition.x = ballMinX + (1 - padPositionX) * (ballMaxX - ballMinX);
        ballPosition.y = ballBottomY;
        ballPosition.z = ballMinZ + (1 - padPositionZ) * ((ballMaxZ + cameraWallOffset) - ballMinZ);
    }
    else {
        totalElapsedTime += timeDelta;
        textureNode->position = glm::vec3(textureNode->position.x, textureNode->position.y - timeDelta * 1000, textureNode->position.z) ;
        if (hasLost) {
            if (mouseLeftReleased) {
                hasLost = false;
                hasStarted = false;
                currentKeyFrame = 0;
                previousKeyFrame = 0;
            }
        }
        else if (isPaused) {
            if (mouseRightReleased) {
                isPaused = false;
                if (options.enableMusic) {
                    sound->play();
                }
            }
        }
        else {
            gameElapsedTime += timeDelta;
            if (mouseRightReleased) {
                isPaused = true;
                if (options.enableMusic) {
                    sound->pause();
                }
            }
            // Get the timing for the beat of the song
            for (unsigned int i = currentKeyFrame; i < keyFrameTimeStamps.size(); i++) {
                if (gameElapsedTime < keyFrameTimeStamps.at(i)) {
                    continue;
                }
                currentKeyFrame = i;
            }

            jumpedToNextFrame = currentKeyFrame != previousKeyFrame;
            previousKeyFrame = currentKeyFrame;

            double frameStart = keyFrameTimeStamps.at(currentKeyFrame);
            double frameEnd = keyFrameTimeStamps.at(currentKeyFrame + 1); // Assumes last keyframe at infinity

            double elapsedTimeInFrame = gameElapsedTime - frameStart;
            double frameDuration = frameEnd - frameStart;
            double fractionFrameComplete = elapsedTimeInFrame / frameDuration;

            double ballYCoord;

            KeyFrameAction currentOrigin = keyFrameDirections.at(currentKeyFrame);
            KeyFrameAction currentDestination = keyFrameDirections.at(currentKeyFrame + 1);

            // Synchronize ball with music
            if (currentOrigin == BOTTOM && currentDestination == BOTTOM) {
                ballYCoord = ballBottomY;
            }
            else if (currentOrigin == TOP && currentDestination == TOP) {
                ballYCoord = ballBottomY + BallVerticalTravelDistance;
            }
            else if (currentDestination == BOTTOM) {
                ballYCoord = ballBottomY + BallVerticalTravelDistance * (1 - fractionFrameComplete);
            }
            else if (currentDestination == TOP) {
                ballYCoord = ballBottomY + BallVerticalTravelDistance * fractionFrameComplete;
            }

            // Make ball move
            const float ballSpeed = 60.0f;
            ballPosition.x += timeDelta * ballSpeed * ballDirection.x;
            ballPosition.y = ballYCoord;
            ballPosition.z += timeDelta * ballSpeed * ballDirection.z;

            // Make ball bounce
            if (ballPosition.x < ballMinX) {
                ballPosition.x = ballMinX;
                ballDirection.x *= -1;
            }
            else if (ballPosition.x > ballMaxX) {
                ballPosition.x = ballMaxX;
                ballDirection.x *= -1;
            }
            if (ballPosition.z < ballMinZ) {
                ballPosition.z = ballMinZ;
                ballDirection.z *= -1;
            }
            else if (ballPosition.z > ballMaxZ) {
                ballPosition.z = ballMaxZ;
                ballDirection.z *= -1;
            }

            if (options.enableAutoplay) {
                padPositionX = 1 - (ballPosition.x - ballMinX) / (ballMaxX - ballMinX);
                padPositionZ = 1 - (ballPosition.z - ballMinZ) / ((ballMaxZ + cameraWallOffset) - ballMinZ);
            }

            // Check if the ball is hitting the pad when the ball is at the bottom.
            // If not, you just lost the game! (hehe)
            if (jumpedToNextFrame && currentOrigin == BOTTOM && currentDestination == TOP) {
                double padLeftX = boxNode->position.x - (boxDimensions.x / 2) + (1 - padPositionX) * (boxDimensions.x - padDimensions.x);
                double padRightX = padLeftX + padDimensions.x;
                double padFrontZ = boxNode->position.z - (boxDimensions.z / 2) + (1 - padPositionZ) * (boxDimensions.z - padDimensions.z);
                double padBackZ = padFrontZ + padDimensions.z;

                if (ballPosition.x < padLeftX
                    || ballPosition.x > padRightX
                    || ballPosition.z < padFrontZ
                    || ballPosition.z > padBackZ
                    ) {
                    hasLost = true;
                    if (options.enableMusic) {
                        sound->stop();
                        delete sound;
                    }
                }
            }

        }
    }

    glm::mat4 projection = glm::perspective(glm::radians(80.0f), float(windowWidth) / float(windowHeight), 0.1f, 350.f);

    glm::vec3 cameraPosition = glm::vec3(0, 2, -20);

    // Some math to make the camera move in a nice way
    float lookRotation = -0.6 / (1 + exp(-5 * (padPositionX - 0.5))) + 0.3;
    glm::mat4 cameraTransform =
        glm::rotate(0.3f + 0.2f * float(-padPositionZ * padPositionZ), glm::vec3(1, 0, 0)) *
        glm::rotate(lookRotation, glm::vec3(0, 1, 0)) *
        glm::translate(-cameraPosition);

    glm::mat4 P = projection ;
    glm::mat4 V = cameraTransform;
    // Move and rotate various SceneNodes
    boxNode->position = { 0, -10, -80 };
    ballNode->position = ballPosition;
    ballNode->scale = glm::vec3(ballRadius);
    ballNode->rotation = { 0, totalElapsedTime * 2, 0 };
    padNode->position = {
        boxNode->position.x - (boxDimensions.x / 2) + (padDimensions.x / 2) + (1 - padPositionX) * (boxDimensions.x - padDimensions.x),
        boxNode->position.y - (boxDimensions.y / 2) + (padDimensions.y / 2),
        boxNode->position.z - (boxDimensions.z / 2) + (padDimensions.z / 2) + (1 - padPositionZ) * (boxDimensions.z - padDimensions.z)
    };
    
    glm::mat4 identity = glm::mat4(1);
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
