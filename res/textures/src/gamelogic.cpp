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
#include "SimplexNoise.h"
#include <algorithm>
enum KeyFrameAction {
    BOTTOM, TOP
};

#include <timestamps.h>

const int const_numberofnoiselayer = 3;
class MinMax {
public:
    float Min = 0;
    float Max = 0;

    MinMax() {
        Min = std::numeric_limits<float>::max();
        Max = std::numeric_limits<float>::lowest();
    }
    void AddValue(float  v) {
        if (v > Max) {
            Max = v;
        }
        if (v < Min) {
            Min = v;
        }
    }
};
struct NoiseSettings
{
    float strength = 1.0f;
    float baseroughness = 1.0f;
    float roughness = 2.0f;
    float persistence = 0.54f;
    int layers = 1;
    glm::vec3 centre = glm::vec3(1);
    float minValue = 0.0;
    float maxValue = 0.0;
    float weightmultiplier = 0.8f;
};
class Filter {
public:
    NoiseSettings noiseSetting;
    virtual float NoiseFilter(glm::vec3 point) = 0;
};
struct NoiseLayer {
    bool enable = true;
    bool useFisrtLayerAsMask = true;
    std::unique_ptr<Filter> filter;
};
struct ShapeSettings
{
    MinMax minmax = MinMax();
    float planetRaduis = 25.0f;
    NoiseLayer* noiselayer[const_numberofnoiselayer];
    int numberofnoiselayer = const_numberofnoiselayer;
};
struct ColourSettings {
    glm::vec4 planetcolour = glm::vec4(1);
    PNGImage planetmaterial;
};
class ColourGenerator{
public:
    ColourSettings  coloursetting;
    void updateElevation(MinMax elevation) {
        coloursetting.planetcolour.x = elevation.Min;
        coloursetting.planetcolour.y = elevation.Max;
        //glBindTextureUnit(0, node->textureID);
        //glBindTextureUnit(0, node->textureID);
    }

};
ShapeSettings shapeSettings;
NoiseSettings noiseSettings;
ColourGenerator colourGenerator;

double padPositionX = 0;
double padPositionZ = 0;
bool change = true;
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
Gloom::Shader* proseduralShader;
sf::Sound* sound;
SimplexNoise noise;

const glm::vec3 boxDimensions(350, 350, 350);
const glm::vec3 padDimensions(1, 1, 1);

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
PNGImage world = loadPNGFile("../res/textures/world.png");

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

float distance(glm::vec3 point1, glm::vec3 point2) {
    return glm::length(point2 - point1);
}

// Modify if you want the music to start further on in the track. Measured in seconds.
const float debug_startTime = 0;
double totalElapsedTime = debug_startTime;
double gameElapsedTime = debug_startTime;

double mouseSensitivity = 1.0;
double lastMouseX = windowWidth / 2;
double lastMouseY = windowHeight / 2;

std::vector<glm::vec3> originalVertices;
Mesh pad;

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

float evaluate(glm::vec3 point, NoiseSettings noiseSetting) {
    glm::vec3 evaluate = point * noiseSetting.roughness + noiseSetting.centre;
    float noiseValue = (SimplexNoise::noise(evaluate.x, evaluate.y, evaluate.z) + 1) * 0.5;
    return noiseValue * noiseSetting.strength;
}


class SimpleNoiseFilter : public Filter {
public:

    float NoiseFilter(glm::vec3 point) override {
        float noiseValue = 0;
        float frequency = noiseSetting.baseroughness;
        float amplitude = 1;
        for (int i = 0; i < noiseSetting.layers; i++) {
            float v = evaluate(point * frequency + noiseSetting.centre, noiseSetting);
            noiseValue += (v + 1) * 0.5f * amplitude;
            frequency *= noiseSetting.roughness;
            amplitude *= noiseSetting.persistence;
        }
        noiseValue = std::max(0.0f, noiseValue - noiseSetting.minValue);
        return noiseValue * noiseSetting.strength;
    }
};

class RidgidNoisefilter : public Filter {
public:

    float NoiseFilter(glm::vec3 point) override {
        float noiseValue = 0;
        float frequency = noiseSetting.baseroughness;
        float amplitude = 1;
        float weight = 1;
        for (int i = 0; i < noiseSetting.layers; i++) {
            float v = 1 - std::abs(evaluate(point * frequency + noiseSetting.centre, noiseSetting));
            v *= v;
            v *= weight;
            weight = std::min(std::max(v * noiseSetting.weightmultiplier, 0.0f), 1.0f);
            noiseValue += v * amplitude;
            frequency *= noiseSetting.roughness;
            amplitude *= noiseSetting.persistence;
        }
        noiseValue = std::max(0.0f, noiseValue - noiseSetting.minValue);
        return noiseValue * noiseSetting.strength;
    }
};






void initGame(GLFWwindow* window, CommandLineOptions gameOptions) {
    buffer = new sf::SoundBuffer();
    if (!buffer->loadFromFile("../res/Hall of the Mountain King.ogg")) {
        return;
    }
    ColourGenerator colourGenerator = ColourGenerator(); 
    shapeSettings.noiselayer[0] = new NoiseLayer();
    shapeSettings.noiselayer[0]->filter = std::make_unique<SimpleNoiseFilter>();
    shapeSettings.noiselayer[1] = new NoiseLayer();
    shapeSettings.noiselayer[1]->filter = std::make_unique<SimpleNoiseFilter>();
    shapeSettings.noiselayer[2] = new NoiseLayer();
    shapeSettings.noiselayer[2]->filter = std::make_unique<RidgidNoisefilter>();

    NoiseSettings noise = shapeSettings.noiselayer[0]->filter->noiseSetting;
    noise.strength = 0.51f;
    noise.baseroughness = 0.71f;
    noise.roughness = 1.83f;
    noise.persistence = 0.54f;
    noise.layers = 5;
    noise.centre = glm::vec3(1);
    noise.minValue = 1.35409;
    shapeSettings.noiselayer[0]->filter->noiseSetting = noise;
    shapeSettings.noiselayer[0]->enable = true;
    // Initialize the noise settings of the second NoiseLayer
    noise = shapeSettings.noiselayer[1]->filter->noiseSetting;
    noise.strength = 0.80f;
    noise.baseroughness = 1.08f;
    noise.roughness = 1.92717f;
    noise.persistence = 0.54f;
    noise.layers = 5;
    noise.centre = glm::vec3(1);
    noise.minValue = 1.62409;
    shapeSettings.noiselayer[1]->filter->noiseSetting = noise;
    shapeSettings.noiselayer[1]->enable = true;

    noise = shapeSettings.noiselayer[2]->filter->noiseSetting;
    noise.strength = 0.1f;
    noise.baseroughness = 1.2f;
    noise.roughness = 2.34;
    noise.persistence = 0.50f;
    noise.layers = 5;
    noise.centre = glm::vec3(1);
    noise.minValue = 0;//1.5;
    shapeSettings.noiselayer[2]->filter->noiseSetting = noise;
    shapeSettings.noiselayer[2]->useFisrtLayerAsMask = false;

    //printf(" settings:%g ", shapeSettings.noiseSettings.strength);
    options = gameOptions;

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
    glfwSetCursorPosCallback(window, mouseCallback);

    texture2dShader = new Gloom::Shader();
    texture2dShader->makeBasicShader("../res/shaders/texture.vert", "../res/shaders/texture.frag");
    
    shader = new Gloom::Shader();
    shader->makeBasicShader("../res/shaders/simple.vert", "../res/shaders/simple.frag");
    shader->activate();
    

    // Create meshes
    
    pad = sphereCube(padDimensions, glm::vec2(30, 30), true, false, glm::vec3(1), 10.0, cameraPosition);
    originalVertices = pad.vertices;
    Mesh box = cube(boxDimensions, glm::vec2(30), true, true);
    
    // Fill buffers

    int textureid = GetLoadedImage(ASCII);
    std::string text = "Start";
    Mesh texture = generateTextGeometryBuffer(text, 39.0 / 29.0, 0.5);
    unsigned int textureVAO = generateBuffer(texture);

    // Construct scene
    rootNode = createSceneNode();
    boxNode  = createSceneNode();
    padNode  = createSceneNode();
    textureNode = createSceneNode();
    generateBufferWhitNode(pad, *padNode);
    generateBufferWhitNode(box, *boxNode);
    generateBufferWhitNode(texture, *textureNode);
    
    
    lightLeftNode = createSceneNode();

    lightLeftNode->nodeType = POINT_LIGHT;

    /// oblig 2 texture
    textureNode->nodeType = GEOMETRY2D;
    boxNode->nodeType = NORMAL_MAPPED_GEOMETRY;

    lightLeftNode->id = 2;
    textureNode->id = 9;

    lightLeftNode->color = glm::vec3(255.0f/255, 241.0f / 255, 00.1);


    lightLeftNode->position = { 10, 20, 40 };

    textureNode->position = { float(windowWidth/2.0)-(29*3), float(windowHeight / 2.0), 0.0};
    textureNode->scale = glm::vec3(300.0);

    rootNode->children.push_back(boxNode);
    rootNode->children.push_back(padNode);
    rootNode->children.push_back(textureNode);

    boxNode->children.push_back(lightLeftNode);

    /// oblig 2
    textureNode->textureID = textureid;

    boxNode->textureID = GetLoadedImage(brick_diffuse);
    boxNode->textureNormal = GetLoadedImage(brick_normalmap);
    boxNode->textureRoughness = GetLoadedImage(brick03_rgh);

    padNode->textureID = GetLoadedImage(world);
    getTimeDeltaSeconds();

    std::cout << fmt::format("Initialized scene with {} SceneNodes.", totalChildren(rootNode)) << std::endl;

    std::cout << "Ready. Click to start!" << std::endl;
}
int celeplanet = 1;
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

    if (glfwGetKey(window, GLFW_KEY_T) == GLFW_PRESS) {
        shapeSettings.noiselayer[celeplanet]->filter->noiseSetting.minValue -= timeDelta;
        change = true;
        printf("\nminvalue: %g", shapeSettings.noiselayer[celeplanet]->filter->noiseSetting.minValue);
    }
    if (glfwGetKey(window, GLFW_KEY_Y) == GLFW_PRESS) {
        shapeSettings.noiselayer[celeplanet]->filter->noiseSetting.minValue += timeDelta;
        change = true;
        printf("\nminvalue: %g", shapeSettings.noiselayer[celeplanet]->filter->noiseSetting.minValue);
    }
    if (glfwGetKey(window, GLFW_KEY_G) == GLFW_PRESS) {
        shapeSettings.noiselayer[celeplanet]->filter->noiseSetting.roughness -= (timeDelta);
        change = true;
        printf("\nroughness: %g", shapeSettings.noiselayer[celeplanet]->filter->noiseSetting.roughness);
    }
    if (glfwGetKey(window, GLFW_KEY_H) == GLFW_PRESS) {
        shapeSettings.noiselayer[celeplanet]->filter->noiseSetting.roughness += (timeDelta);
        change = true;
        printf("\nroughness: %g", shapeSettings.noiselayer[celeplanet]->filter->noiseSetting.roughness);
    }
    if (glfwGetKey(window, GLFW_KEY_O) == GLFW_PRESS) {
        shapeSettings.noiselayer[celeplanet]->filter->noiseSetting.strength -= (timeDelta);
        change = true;
        printf("\nstrength: %g", shapeSettings.noiselayer[celeplanet]->filter->noiseSetting.strength);
    }
    if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS) {
        shapeSettings.noiselayer[celeplanet]->filter->noiseSetting.strength += (timeDelta);
        change = true;
        printf("\nstrength: %g", shapeSettings.noiselayer[celeplanet]->filter->noiseSetting.strength);
    }

    if (glfwGetKey(window, GLFW_KEY_U) == GLFW_PRESS) {
        shapeSettings.noiselayer[celeplanet]->filter->noiseSetting.persistence -= (timeDelta);
        change = true;
        printf("\npersistence: %g", shapeSettings.noiselayer[celeplanet]->filter->noiseSetting.persistence);
    }
    if (glfwGetKey(window, GLFW_KEY_I) == GLFW_PRESS) {
        shapeSettings.noiselayer[celeplanet]->filter->noiseSetting.persistence += (timeDelta);
        change = true;
        printf("\npersistence: %g", shapeSettings.noiselayer[celeplanet]->filter->noiseSetting.persistence);
    }

    if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS) {
        shapeSettings.noiselayer[0]->enable = !shapeSettings.noiselayer[0]->enable;
        change = true;
        printf("\n1 off: %B", shapeSettings.noiselayer[0]->enable);
    }

    if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS) {
        shapeSettings.noiselayer[1]->enable = !shapeSettings.noiselayer[1]->enable;
        change = true;
        printf("\n2 off: %B", shapeSettings.noiselayer[1]->enable);
    }

    if (glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS) {
        shapeSettings.noiselayer[2]->enable = !shapeSettings.noiselayer[2]->enable;
        change = true;
        printf("\n3 off: %B", shapeSettings.noiselayer[2]->enable);
    }

}


bool change1 = false;

void updateFrame(GLFWwindow* window) {


    double timeDelta = getTimeDeltaSeconds();

    processInput(window, timeDelta);

    //float cameraPadDistance = distance(cameraPosition, padNode->position);
  
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
        * glm::rotate(node->rotation.y, glm::vec3(0, 1, 0))
        * glm::rotate(node->rotation.x, glm::vec3(1, 0, 0))
        * glm::rotate(node->rotation.z, glm::vec3(0, 0, 1))
        * glm::scale(node->scale)
        * glm::translate(-node->referencePoint);

    node->currentTransformationMatrix = transformationThusFar * transformationMatrix;


    switch (node->nodeType) {
    case GEOMETRY:


        if (node->id == padNode->id) {
            // Send position of ball
            glm::vec3 shadow = node->currentTransformationMatrix * glm::vec4(0, 0, 0, 1);
            glUniform3f(7, shadow.x, shadow.y, shadow.z);
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
    case GEOMETRY2D: break;
    

    }

    for (SceneNode* child : node->children) {
        updateNodeTransformations(child, node->currentTransformationMatrix);
    }
}

void renderNode(SceneNode* node) {


    glUniformMatrix4fv(3, 1, GL_FALSE, glm::value_ptr(node->currentTransformationMatrix));
    // Task 1 c)
    glm::mat3 normalmatrix = glm::transpose(glm::inverse(node->currentTransformationMatrix));
    // Task 1 b)
    glUniformMatrix3fv(13, 1, GL_FALSE, glm::value_ptr(normalmatrix));


    switch (node->nodeType) {

    case GEOMETRY:
        if (node->vertexArrayObjectID == padNode->vertexArrayObjectID) {
            double timeDelta = getTimeDeltaSeconds();
            //noiseSettings.centre.x += (sin(timeDelta * frequency) * amplitude);
            if(change){
                for (size_t i = 0; i < originalVertices.size(); ++i) {
                    float elevation = 0;
                    float firstLayerValue = 0;
                    if (shapeSettings.numberofnoiselayer > 0) {
                        firstLayerValue = shapeSettings.noiselayer[0]->filter->NoiseFilter(originalVertices[i]);
                        if (shapeSettings.noiselayer[0]->enable) {
                            elevation = firstLayerValue;
                        }
                    }
                    for (int j = 1; j < shapeSettings.numberofnoiselayer; j++) {
                        if (shapeSettings.noiselayer[j]->enable) {
                            float mask = (shapeSettings.noiselayer[j]->useFisrtLayerAsMask) ? firstLayerValue : 1;
                            elevation += shapeSettings.noiselayer[j]->filter->NoiseFilter(originalVertices[i]) * mask;
                        }
                    }
                    elevation = shapeSettings.planetRaduis * (1 + elevation);
                    shapeSettings.minmax.AddValue(elevation);
                    pad.vertices[i] = originalVertices[i] * elevation;
                }
                glUniform3f(11, node->position.x,node->position.y, node->position.z);
                glUniform2f(8, shapeSettings.minmax.Min, shapeSettings.minmax.Max);
                glm::vec3 position = node->currentTransformationMatrix * glm::vec4(0, 0, 0, 1);
                glUniform3f(7, position.x, position.y, position.z);
                glBindBuffer(GL_ARRAY_BUFFER, node->vertexBufferID);
                glBufferData(GL_ARRAY_BUFFER, pad.vertices.size() * sizeof(glm::vec3), pad.vertices.data(), GL_STATIC_DRAW);
                change = false;
            }
        }
        if (node->vertexArrayObjectID != -1) {
            // 1 for bruk texture
            glUniform1i(6, 2);
            glUniformMatrix3fv(7, 1, GL_FALSE, glm::value_ptr(node->position));
            glUniformMatrix3fv(8, 1, GL_FALSE, glm::value_ptr(node->position));
            glBindVertexArray(node->vertexArrayObjectID);
            glBindTextureUnit(0, node->textureID);
            glBindTextureUnit(1, node->textureNormal);
            glBindTextureUnit(2, node->textureRoughness);
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
