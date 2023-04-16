#pragma once

#include <glm/glm.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <stack>
#include <vector>
#include <cstdio>
#include <stdbool.h>
#include <cstdlib> 
#include <ctime> 
#include <chrono>
#include <fstream>
#include "SimplexNoise.h"
enum SceneNodeType {
	GEOMETRY, POINT_LIGHT, SPOT_LIGHT, GEOMETRY2D, NORMAL_MAPPED_GEOMETRY, PLANE
};
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

struct ElevationColorPoint {
	float elevation;
	glm::vec4 color;
};
struct ColourSettings {
	glm::vec2 elevation = glm::vec2(1);
	std::vector<ElevationColorPoint> colour = {
		{0.00f, glm::vec4(0, 0, 102, 255)},
		{0.18f, glm::vec4(0, 0, 153, 255)},
		{0.20f, glm::vec4(0, 128, 255, 255)},
		{0.21f, glm::vec4(240, 240, 64, 255)},
		{0.30f, glm::vec4(25, 153, 0, 255)},
		{0.40f, glm::vec4(77, 204, 0, 255)},
		{0.50f, glm::vec4(128, 102, 0, 255)},
		{0.80f, glm::vec4(102, 102, 102, 255)},
		{0.95f, glm::vec4(255, 255, 255, 255)}
	};
};
struct BiomeSettings {
	ColourSettings coloursettings;
	glm::vec4 tint = glm::vec4(1);
	float startheight = 0.0;
	float tintpercent = 0.1;
	ColourSettings coloursetting;
	void updateElevation(MinMax elevation) {
		coloursetting.elevation.x = elevation.Min;
		coloursetting.elevation.y = elevation.Max;
	}
};

inline float evaluate(glm::vec3 point, NoiseSettings noiseSetting) {
	glm::vec3 eval = point * noiseSetting.roughness + noiseSetting.centre;
	float noiseValue = (SimplexNoise::noise(eval.x, eval.y, eval.z) + 1) * 0.5;
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
class ColourGenerator {
public:
	Filter* filter = new SimpleNoiseFilter;
	int numbiomes = 1;
	ColourGenerator(int numBiomes ) : biomes(numBiomes) {
		filter->noiseSetting.strength = 1.0;
		filter->noiseSetting.roughness = 1.0;
		filter->noiseSetting.baseroughness = 1.0;
		filter->noiseSetting.minValue = 0.0;
		filter->noiseSetting.persistence = 0.5;
		filter->noiseSetting.layers = 2;
		numbiomes = numBiomes;
	}
	std::vector<BiomeSettings> biomes;
	float noiseOffset = 0.73;
	float noiseStrength = 0.43;
	float blent = 0.1 ;
};



struct SceneNode {
	SceneNode() {
		position = glm::vec3(0, 0, 0);
		rotation = glm::vec3(0, 0, 0);
		scale = glm::vec3(1, 1, 1);
		id = -1;
		color = glm::vec3(0, 0, 0);
        referencePoint = glm::vec3(0, 0, 0);
        vertexArrayObjectID = -1;
        VAOIndexCount = 0;

        nodeType = GEOMETRY;
		colorGenerators;
	}

	// A list of all children that belong to this node.
	// For instance, in case of the scene graph of a human body shown in the assignment text, the "Upper Torso" node would contain the "Left Arm", "Right Arm", "Head" and "Lower Torso" nodes in its list of children.
	std::vector<SceneNode*> children;
	
	
	// The node's position and rotation relative to its parent
	glm::vec3 position;
	glm::vec3 rotation;
	glm::vec3 scale;

	// A transformation matrix representing the transformation of the node's location relative to its parent. This matrix is updated every frame.
	glm::mat4 currentTransformationMatrix;

	// The location of the node's reference point
	glm::vec3 referencePoint;

	// The ID of the VAO containing the "appearance" of this SceneNode.
	// TODO
	// While you’re here, also define an enum for normal mapped geometry, and a corresponding normal map texture ID in the struct. We’ll use this in the other major task
	// of this assignment.
	//
	//
	int vertexArrayObjectID;
	int vertexBufferID;
	int indexBufferID;
	int tangetBufferID;
	int	bitangentBufferID;
	int noiseBufferID; 
	int orignialvertexBufferID;
	
	unsigned int VAOIndexCount;
	unsigned int textureID;
	unsigned int elevationTextureID;
	unsigned int textureNormal;
	unsigned int textureRoughness;
	std::vector<glm::vec3> originalVertices; 
	std::vector<glm::vec3> vertices;
	int id;
	bool change;
	bool atsmophere;
	glm::vec3 color;
	ColourGenerator* colorGenerators;
	ShapeSettings shapesettings;
	// Node type is used to determine how to handle the contents of a node
	SceneNodeType nodeType;
};

SceneNode* createSceneNode();
void addChild(SceneNode* parent, SceneNode* child);
void printNode(SceneNode* node);
int totalChildren(SceneNode* parent);

// For more details, see SceneGraph.cpp.