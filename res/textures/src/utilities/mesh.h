#pragma once

#include <vector>
#include <glm/glm.hpp>

struct Mesh {
    std::vector<glm::vec3> vertices;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec2> textureCoordinates;
    std::vector<glm::vec3> tangent;
    std::vector<glm::vec3> bitangent;
    std::vector<unsigned int> indices;
};