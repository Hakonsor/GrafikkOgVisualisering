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

ShapeSettings selectSettings;

double padPositionX = 0;
double padPositionZ = 0;
bool change = true;
bool moonchange = true;
bool astroidchange = true;
unsigned int currentKeyFrame = 0;
unsigned int previousKeyFrame = 0;

glm::vec3 cameraPosition = glm::vec3(0, 2, 50);

float rotasjonupanddown = 0.0;
float rotasjonfloat = 0.0;
float rotasjonsomething = 0.0;

SceneNode* rootNode;
SceneNode* boxNode;
SceneNode* padNode;
SceneNode* moonNode;
SceneNode* astroidNode;

SceneNode* lightLeftNode;

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

float inverseLerp(float a, float b, float value) {
    if (a != b) {
        return (value - a) / (b - a);
    }
    else {
        return 0.0f;
    }
}

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

glm::vec4 ElevationColor(float elevation, std::vector<ElevationColorPoint> elevationColors) {
    //printf("elevation: %g", elevation);
    for (size_t i = 0; i < elevationColors.size() - 1; ++i) {
        if (elevation >= elevationColors[i].elevation && elevation <= elevationColors[i + 1].elevation) {
            float t = (elevation - elevationColors[i].elevation) / (elevationColors[i + 1].elevation - elevationColors[i].elevation);
            return glm::mix(elevationColors[i].color, elevationColors[i + 1].color, t) / 255.0f;
        }
    }
    return elevationColors.back().color / 255.0f;
}

unsigned int GetElevationColorMap(SceneNode* node) {
    int textureResolution = 50;
    int numBiomes = node->colorGenerators->numbiomes;
    //printf("enumbiomes: %d", numBiomes);
    std::vector<glm::vec4> colour(textureResolution * numBiomes);
    int colorindex = 0;
    for (int biome = 0; biome < numBiomes; ++biome ) {
        for (int i = 0; i < textureResolution; i++) {
            float t = float(i) / (textureResolution - 1.0f);
            glm::vec4 gradientcollum = ElevationColor(t , node->colorGenerators->biomes[biome].coloursettings.colour);// node->colorSettings.biome.coloursettings.colour
            glm::vec4 tintcol = node->colorGenerators->biomes[biome].tint;      
            colour[colorindex] = gradientcollum * (1 - node->colorGenerators->biomes[biome].tintpercent) + tintcol * node->colorGenerators->biomes[biome].tintpercent;
            colorindex++;   
        }
    }
    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, textureResolution, numBiomes, 0, GL_RGBA, GL_FLOAT, colour.data());

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    node->elevationTextureID = textureID;
    return textureID;
     
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

//Mesh pad;

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

void DefultPlanet(SceneNode* node) {

    ShapeSettings shapeSettings;
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

    node->shapesettings = shapeSettings;

    ColourGenerator* colors = new ColourGenerator(7);
    colors->filter->noiseSetting.strength = 1;
    //colors->noiseOffset = 0.1;
    //colors->noiseStrength = 0.1;

    BiomeSettings biome = colors->biomes[0];

    biome.tint = glm::vec4(0);
    biome.startheight = 0.0001f;
    biome.coloursettings.colour = {
        {0.00f, glm::vec4(255, 255, 255, 255)},
        {0.80f, glm::vec4(150, 150, 150, 255)}, // High rocky terrain
        {0.95f, glm::vec4(255, 255, 255, 255)}  // Mountain top
    };
    colors->biomes[0] = biome;

    biome = colors->biomes[1];
    biome.tint = glm::vec4(1);
    biome.coloursettings.colour = {
        {0.00f, glm::vec4(0, 0, 102, 255)}, // Deepsea
        {0.18f, glm::vec4(30, 144, 255, 255)}, // Icy water
        {0.20f, glm::vec4(70, 130, 180, 255)}, // Ice
        {0.21f, glm::vec4(255, 250, 250, 255)}, // Snowy
        {0.30f, glm::vec4(240, 248, 255, 255)}, // Light snow
        {0.40f, glm::vec4(205, 133, 63, 255)}, // Bare ground
        {0.50f, glm::vec4(139, 69, 19, 255)}, // Rocky ground
        {0.80f, glm::vec4(150, 150, 150, 255)}, // High rocky terrain
        {0.95f, glm::vec4(255, 255, 255, 255)}  // Mountain top
    };
    biome.startheight = 0.002f;
    colors->biomes[1] = biome;

    biome = colors->biomes[2];
    biome.tint = glm::vec4(1);
    biome.startheight = 0.030f;
    colors->biomes[2] = biome;

    biome = colors->biomes[3];
    biome.tint = glm::vec4(1);
    biome.startheight = 0.50f;
    biome.coloursettings.colour = {
        { 0.00f, glm::vec4(0, 0, 102, 255)}, // Deepsea
        { 0.18f, glm::vec4(25, 25, 112, 255) }, // Shallow water
        { 0.20f, glm::vec4(218, 165, 32, 255) }, // Beach
        { 0.30f, glm::vec4(244, 164, 96, 255) }, // Light sand
        { 0.40f, glm::vec4(210, 180, 140, 255) }, // Sand
        { 0.50f, glm::vec4(139, 69, 19, 255) }, // Rocky ground
        { 0.60f, glm::vec4(160, 82, 45, 255) }, // Red rock
        { 0.80f, glm::vec4(128, 128, 0, 255) }, // Sparse vegetation
        { 0.95f, glm::vec4(105, 105, 105, 255) }  // Mountain top
    };
    colors->biomes[3] = biome;

    biome = colors->biomes[4];
    biome.tint = glm::vec4(1);
    biome.startheight = 0.60f;
    colors->biomes[4] = biome;

    biome = colors->biomes[5];
    biome.tint = glm::vec4(1);
    biome.startheight = 0.99f;
    //biome.coloursettings.colour = {
    //       {0.00f, glm::vec4(0, 0, 102, 255)}, // Deepsea
    //{0.18f, glm::vec4(30, 144, 255, 255)}, // Icy water
    //{0.20f, glm::vec4(70, 130, 180, 255)}, // Ice
    //{0.21f, glm::vec4(255, 250, 250, 255)}, // Snowy
    //{0.30f, glm::vec4(240, 248, 255, 255)}, // Light snow
    //{0.40f, glm::vec4(205, 133, 63, 255)}, // Bare ground
    //{0.50f, glm::vec4(139, 69, 19, 255)}, // Rocky ground
    //{0.80f, glm::vec4(150, 150, 150, 255)}, // High rocky terrain
    //{0.95f, glm::vec4(255, 255, 255, 255)}  // Mountain top
    //};
    colors->biomes[5] = biome;

    biome = colors->biomes[6];
    biome.tint = glm::vec4(0);
    biome.startheight = 1.00f;
    biome.coloursettings.colour = {
        {0.00f, glm::vec4(255, 255, 255, 255)},
        {0.80f, glm::vec4(150, 150, 150, 255)}, // High rocky terrain
        {0.95f, glm::vec4(255, 255, 255, 255)}  // Mountain top
    };
    colors->biomes[6] = biome;
    node->colorGenerators = colors;
    node->change = true;
    GetElevationColorMap(node);
    printf("\nnumber of biomes: %d\n", node->colorGenerators->numbiomes );
}

void DefultMoon(SceneNode* node) {
    ShapeSettings shapeSettings;
    shapeSettings.planetRaduis = 1;


    shapeSettings.noiselayer[0] = new NoiseLayer();
    shapeSettings.noiselayer[0]->filter = std::make_unique<SimpleNoiseFilter>();
    shapeSettings.noiselayer[1] = new NoiseLayer();
    shapeSettings.noiselayer[1]->filter = std::make_unique<SimpleNoiseFilter>();
    shapeSettings.noiselayer[2] = new NoiseLayer();
    shapeSettings.noiselayer[2]->filter = std::make_unique<RidgidNoisefilter>();
    node->shapesettings = shapeSettings;
    ColourGenerator* colors = new ColourGenerator(1);
    colors->filter->noiseSetting.strength = 1;
    BiomeSettings biome = colors->biomes[0];
    biome.tint = glm::vec4(1);
    biome.startheight = 0.0f;
    biome.coloursettings.colour = {
        {0.00f, glm::vec4(0, 0, 0, 255)}, // Light gray
        {0.50f, glm::vec4(180, 180, 180, 255)}, // Lighter gray
        {0.00f, glm::vec4(60, 40, 40, 255)} 
    };
    colors->biomes[0] = biome;

    node->colorGenerators = colors;
    node->change = true;
    GetElevationColorMap(node);
    printf("\nmoon: number of biomes: %d\n", node->colorGenerators->numbiomes);
}

void DefaultAsteroid(SceneNode* node) {
    ShapeSettings shapeSettings;
    shapeSettings.planetRaduis = 0.5; // Smaller radius for asteroid

    // Adjust the noise layers to create a more asteroid-like shape
    shapeSettings.noiselayer[0] = new NoiseLayer();
    shapeSettings.noiselayer[0]->filter = std::make_unique<RidgidNoisefilter>();
    shapeSettings.noiselayer[1] = new NoiseLayer();
    shapeSettings.noiselayer[1]->filter = std::make_unique<RidgidNoisefilter>();
    shapeSettings.noiselayer[2] = new NoiseLayer();
    shapeSettings.noiselayer[2]->filter = std::make_unique<SimpleNoiseFilter>();

    node->shapesettings = shapeSettings;

    ColourGenerator* colors = new ColourGenerator(2);

    BiomeSettings biome = colors->biomes[0];
    biome.tint = glm::vec4(1);
    biome.startheight = 0.0f;
    biome.coloursettings.colour = {
          {0.00f, glm::vec4(60, 40, 40, 255)},
        {0.00f, glm::vec4(60, 40, 40, 255)}, // Dark brown
        {0.50f, glm::vec4(100, 70, 70, 255)}, // Medium brown
        {1.00f, glm::vec4(140, 100, 100, 255)} // Lighter brown
    };
    colors->biomes[0] = biome;

    biome = colors->biomes[1];
    biome.tint = glm::vec4(1);
    biome.startheight = 0.5f;
    biome.coloursettings.colour = {
        {0.00f, glm::vec4(130, 100, 100, 255)}, // Light brown
        {0.50f, glm::vec4(160, 130, 130, 255)}, // Lighter brown
        {1.00f, glm::vec4(190, 160, 160, 255)} // Almost white-brown
    };
    colors->biomes[1] = biome;

    node->colorGenerators = colors;
    node->change = true;
    GetElevationColorMap(node);
    printf("\nastroid: number of biomes: %d\n", node->colorGenerators->numbiomes);
}

void initGame(GLFWwindow* window, CommandLineOptions gameOptions) {
    buffer = new sf::SoundBuffer();
    if (!buffer->loadFromFile("../res/Hall of the Mountain King.ogg")) {
        return;
    }
    
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
    
    Mesh pad = sphereCube(padDimensions, glm::vec2(30, 30), true, false, glm::vec3(1), 10.0, cameraPosition);
    Mesh box = cube(boxDimensions, glm::vec2(30), true, true);
    Mesh moon = sphereCube(padDimensions, glm::vec2(30, 30), true, false, glm::vec3(1), 10.0, cameraPosition);
    Mesh astroid = sphereCube(padDimensions, glm::vec2(30, 30), true, false, glm::vec3(1), 10.0, cameraPosition);
    Mesh sun = generateSphere(1.0, 40, 40);
    
    // Fill buffers
    int textureid = GetLoadedImage(ASCII);
    std::string text = "Start";
    Mesh texture = generateTextGeometryBuffer(text, 39.0 / 29.0, 0.5);
    unsigned int textureVAO = generateBuffer(texture);

    // Construct scene
    rootNode = createSceneNode();
    boxNode  = createSceneNode();
    SceneNode* sunNode = createSceneNode();
    

    padNode  = createSceneNode();
    padNode->vertices = pad.vertices;
    padNode->originalVertices = pad.vertices;
    DefultPlanet(padNode);
    /*padNode->position.x += 10;*/

    moonNode = createSceneNode();
    moonNode->originalVertices = moon.vertices;
    moonNode->vertices = moon.vertices;
    DefultMoon(moonNode);
    moonNode->position = { 20, 10, 30 };

    astroidNode = createSceneNode();
    astroidNode->originalVertices = astroid.vertices;
    astroidNode->vertices = astroid.vertices;
    DefaultAsteroid(astroidNode);
    astroidNode->position = { 10, 10, 30 };

    textureNode = createSceneNode();
    generateBufferWhitNode(pad, *padNode);
    generateBufferWhitNode(box, *boxNode);
    generateBufferWhitNode(sun, *sunNode);
    generateBufferWhitNode(texture, *textureNode);
    generateBufferWhitNode(moon, *moonNode);
    generateBufferWhitNode(moon, *astroidNode);
    
    lightLeftNode = createSceneNode();

    lightLeftNode->nodeType = POINT_LIGHT;

    /// oblig 2 texture
    textureNode->nodeType = GEOMETRY2D;
    boxNode->nodeType = NORMAL_MAPPED_GEOMETRY;
    sunNode->nodeType = SPOT_LIGHT;

    lightLeftNode->id = 2;
    textureNode->id = 9;

    lightLeftNode->color = glm::vec3(255.0f/255, 255.0f / 255, 255/255);
    lightLeftNode->position = { 10, 10, 50 };
    sunNode->position = lightLeftNode->position;

    textureNode->position = { float(windowWidth/2.0)-(29*3), float(windowHeight / 2.0), 0.0};
    textureNode->scale = glm::vec3(300.0);

    rootNode->children.push_back(boxNode);
    rootNode->children.push_back(padNode);
    
    rootNode->children.push_back(astroidNode);
    rootNode->children.push_back(moonNode); // Moon Gets the shadow. only support for one shawdow so far
    boxNode->children.push_back(lightLeftNode);
    
    /*rootNode->children.push_back(astroidNode);*/
   
    
    
    
    /*lightLeftNode->children.push_back(sunNode);*/
    rootNode->children.push_back(textureNode);

    /// oblig 2
    textureNode->textureID = textureid;

    boxNode->textureID = GetLoadedImage(brick_diffuse);
    boxNode->textureNormal = GetLoadedImage(brick_normalmap);
    boxNode->textureRoughness = GetLoadedImage(brick03_rgh);
   

    /*padNode->textureID = GetLoadedImage(world);*/
    getTimeDeltaSeconds();

    std::cout << fmt::format("Initialized scene with {} SceneNodes.", totalChildren(rootNode)) << std::endl;

    std::cout << "Ready. Click to start!" << std::endl;
}
int celeplanet = 0;
void processInput(GLFWwindow* window,float timeDelta, SceneNode* node) {
    ShapeSettings shapeSettings = node->shapesettings;
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
        shapeSettings.noiselayer[celeplanet]->filter->noiseSetting.minValue -= 0.10;
        node->change = true;
        printf("\nminvalue: %g", shapeSettings.noiselayer[celeplanet]->filter->noiseSetting.minValue);
    }
    if (glfwGetKey(window, GLFW_KEY_Y) == GLFW_PRESS) {
        shapeSettings.noiselayer[celeplanet]->filter->noiseSetting.minValue += 0.10;
        node->change = true;
        printf("\nminvalue: %g", shapeSettings.noiselayer[celeplanet]->filter->noiseSetting.minValue);
    }
    if (glfwGetKey(window, GLFW_KEY_G) == GLFW_PRESS) {
        shapeSettings.noiselayer[celeplanet]->filter->noiseSetting.roughness -= 0.10;
        node->change = true;
        printf("\nroughness: %g", shapeSettings.noiselayer[celeplanet]->filter->noiseSetting.roughness);
    }
    if (glfwGetKey(window, GLFW_KEY_H) == GLFW_PRESS) {
        shapeSettings.noiselayer[celeplanet]->filter->noiseSetting.roughness += 0.10;
        node->change = true;
        printf("\nroughness: %g", shapeSettings.noiselayer[celeplanet]->filter->noiseSetting.roughness);
    }
    if (glfwGetKey(window, GLFW_KEY_O) == GLFW_PRESS) {
        shapeSettings.noiselayer[celeplanet]->filter->noiseSetting.strength -= 0.10;
        node->change = true;
        printf("\nstrength: %g", shapeSettings.noiselayer[celeplanet]->filter->noiseSetting.strength);
    }
    if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS) {
        shapeSettings.noiselayer[celeplanet]->filter->noiseSetting.strength += 0.10;
        node->change = true;
        printf("\nstrength: %g", shapeSettings.noiselayer[celeplanet]->filter->noiseSetting.strength);
    }

    if (glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS) {
        shapeSettings.noiselayer[celeplanet]->filter->noiseSetting.centre.x += 0.10;
        node->change = true;
    }

    if (glfwGetKey(window, GLFW_KEY_U) == GLFW_PRESS) {
        shapeSettings.noiselayer[celeplanet]->filter->noiseSetting.persistence -= 0.10;
        node->change = true;
        printf("\npersistence: %g", shapeSettings.noiselayer[celeplanet]->filter->noiseSetting.persistence);
    }
    if (glfwGetKey(window, GLFW_KEY_I) == GLFW_PRESS) {
        shapeSettings.noiselayer[celeplanet]->filter->noiseSetting.persistence += 0.10;
        node->change = true;
        printf("\npersistence: %g", shapeSettings.noiselayer[celeplanet]->filter->noiseSetting.persistence);
    }

    if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS) {
        shapeSettings.noiselayer[0]->enable = !shapeSettings.noiselayer[0]->enable;
        node->change = true;
        printf("\n1 off: %B", shapeSettings.noiselayer[0]->enable);
    }

    if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS) {
        shapeSettings.noiselayer[1]->enable = !shapeSettings.noiselayer[1]->enable;
        node->change = true;
        printf("\n2 off: %B", shapeSettings.noiselayer[1]->enable);
    }

    if (glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS) {
        shapeSettings.noiselayer[2]->enable = !shapeSettings.noiselayer[2]->enable;
        node->change = true;
        printf("\n3 off: %B", shapeSettings.noiselayer[2]->enable);
    }


    if (glfwGetKey(window, GLFW_KEY_0) == GLFW_PRESS) {
        lightLeftNode->position.x -= timeDelta * 100.0;
    }
    if (glfwGetKey(window, GLFW_KEY_9) == GLFW_PRESS) {
        lightLeftNode->position.x += timeDelta * 100.0;
    }
    if (glfwGetKey(window, GLFW_KEY_8) == GLFW_PRESS) {
        lightLeftNode->position.z -= timeDelta * 100.0;
    }
    if (glfwGetKey(window, GLFW_KEY_7) == GLFW_PRESS) {
        lightLeftNode->position.z += timeDelta * 100.0;
    }

}


bool change1 = false;

void updateFrame(GLFWwindow* window) {


    double timeDelta = getTimeDeltaSeconds();

    processInput(window, timeDelta, moonNode);
   
    // Set up camera
    glm::vec3 camera_position = cameraPosition;
    glm::vec3 camera_target(0.0f, 0.0f, -1.0f);
    glm::vec3 camera_up(0.0f, 1.0f, 0.0f);
    glm::mat4 view_matrix = glm::lookAt(camera_position, camera_position + camera_target, camera_up);

    // Set up projection
    float field_of_view = 80.0f;
    float aspect_ratio = float(windowWidth) / float(windowHeight);
    float near_clip_distance = 0.1f;
    float far_clip_distance = 3500.0f;
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
    case GEOMETRY:{
            // Send position of ball
            glm::vec3 shadow = node->currentTransformationMatrix * glm::vec4(0, 0, 0, 1);
            glUniform3f(15, shadow.x, shadow.y, shadow.z);
        break;
    }
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
        if (node->vertexArrayObjectID != -1) {
            double timeDelta = getTimeDeltaSeconds();
            
            //noiseSettings.centre.x += (sin(timeDelta * frequency) * amplitude);
            if(node->change){
                std::vector<glm::vec1> uv(node->originalVertices.size());
                node->shapesettings.minmax.Min = std::numeric_limits<float>::max();
                node->shapesettings.minmax.Max = std::numeric_limits<float>::lowest();
                for (size_t i = 0; i < node->originalVertices.size(); ++i) {
                    float elevation = 0;
                    float firstLayerValue = 0;
                    if (node->shapesettings.numberofnoiselayer > 0) {
                        firstLayerValue = node->shapesettings.noiselayer[0]->filter->NoiseFilter(node->originalVertices[i]);
                        if (node->shapesettings.noiselayer[0]->enable) {
                            elevation = firstLayerValue;
                        }
                    }
                    for (int j = 1; j < node->shapesettings.numberofnoiselayer; j++) {
                        if (node->shapesettings.noiselayer[j]->enable) {
                            float mask = (node->shapesettings.noiselayer[j]->useFisrtLayerAsMask) ? firstLayerValue : 1;
                            elevation += node->shapesettings.noiselayer[j]->filter->NoiseFilter(node->originalVertices[i]) * mask;
                        }
                    }
                    elevation = node->shapesettings.planetRaduis * (1 + elevation);
                    node->shapesettings.minmax.AddValue(elevation);
                    node->vertices[i] = node->originalVertices[i] * elevation;

                    uv[i] = glm::vec1((node->colorGenerators->filter->NoiseFilter(node->originalVertices[i])-node->colorGenerators->noiseOffset) * node->colorGenerators->noiseStrength);
                }
                glBindBuffer(GL_ARRAY_BUFFER, node->noiseBufferID); 
                glBufferData(GL_ARRAY_BUFFER, uv.size() * sizeof(glm::vec1), uv.data(), GL_STATIC_DRAW);
                glBindBuffer(GL_ARRAY_BUFFER, node->vertexBufferID);
                glBufferData(GL_ARRAY_BUFFER, node->vertices.size() * sizeof(glm::vec3), node->vertices.data(), GL_STATIC_DRAW);
                node->change = false;
            }
        }
        if (node->vertexArrayObjectID != -1) {

            for (size_t i = 0; i < node->colorGenerators->numbiomes; i++) {
                std::string uniformName = "biomes[" + std::to_string(i) + "].startheight";
                GLint startheightLocation = shader->getUniformFromName(uniformName.c_str());
                glUniform1f(startheightLocation, node->colorGenerators->biomes[i].startheight);
            }
            glUniform1i(10, static_cast<GLint>(node->colorGenerators->numbiomes));
            glUniform3f(11, node->position.x, node->position.y, node->position.z);
            glUniform2f(8, node->shapesettings.minmax.Min, node->shapesettings.minmax.Max);
            glUniform1i(6, 2);
            glBindVertexArray(node->vertexArrayObjectID);
            glBindTextureUnit(0, node->textureID);
            glBindTextureUnit(1, node->textureNormal);
            glBindTextureUnit(2, node->textureRoughness);
            glBindTextureUnit(3, node->elevationTextureID);
            glDrawElements(GL_TRIANGLES, node->VAOIndexCount, GL_UNSIGNED_INT, nullptr);
        }
        break;
        case POINT_LIGHT: 
            break;
        case SPOT_LIGHT: 
            glBindVertexArray(node->vertexArrayObjectID);
            glDrawElements(GL_TRIANGLES, node->VAOIndexCount, GL_UNSIGNED_INT, nullptr);
            break;
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
