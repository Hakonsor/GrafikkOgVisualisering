#pragma once

#include "mesh.h"
#include "sceneGraph.hpp"

void generateBufferWhitNode(Mesh &mesh, SceneNode &node);
unsigned int generateFrameBufferObject(unsigned int depthTexture);
unsigned int generateDepthTextureObject(int screenWidth, int screenHeight);
unsigned int generateBuffer(Mesh &mesh); 