#include <glad/glad.h>
#include <program.hpp>
#include "glutils.h"
#include <vector>
#include <iostream>
#include <sceneGraph.hpp>

template <class T>
unsigned int generateAttribute(int id, int elementsPerEntry, std::vector<T> data, bool normalize) {
    unsigned int bufferID;
    glGenBuffers(1, &bufferID);
    glBindBuffer(GL_ARRAY_BUFFER, bufferID);
    glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(T), data.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(id, elementsPerEntry, GL_FLOAT, normalize ? GL_TRUE : GL_FALSE, sizeof(T), 0);
    glEnableVertexAttribArray(id);
    return bufferID;
}

void computeTangentBasis(
    // inputs
    std::vector<glm::vec3>& vertices,
    std::vector<glm::vec2>& uvs,
    std::vector<glm::vec3>& normals,
    // outputs
    std::vector<glm::vec3>& tangents,
    std::vector<glm::vec3>& bitangents
) {

    /*For each triangle, we compute the edge(deltaPos) and the deltaUV*/
    for (int i = 0; i < vertices.size(); i += 3) {

        // Shortcuts for vertices
        glm::vec3& v0 = vertices[i + 0];
        glm::vec3& v1 = vertices[i + 1];
        glm::vec3& v2 = vertices[i + 2];

        // Shortcuts for UVs
        glm::vec2& uv0 = uvs[i + 0];
        glm::vec2& uv1 = uvs[i + 1];
        glm::vec2& uv2 = uvs[i + 2];

        // Edges of the triangle : position delta
        glm::vec3 deltaPos1 = v1 - v0;
        glm::vec3 deltaPos2 = v2 - v0;

        // UV delta
        glm::vec2 deltaUV1 = uv1 - uv0;
        glm::vec2 deltaUV2 = uv2 - uv0;


        /*We can now use our formula to compute the tangentand the bitangent :*/


        float r = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV1.y * deltaUV2.x);
        glm::vec3 tangent = (deltaPos1 * deltaUV2.y - deltaPos2 * deltaUV1.y) * r;
        glm::vec3 bitangent = (deltaPos2 * deltaUV1.x - deltaPos1 * deltaUV2.x) * r;

        /*Finally, we fill the* tangents*and* bitangents* buffers.Remember, these buffers are not indexed yet, so each vertex has its own copy.*/

        // Set the same tangent for all three vertices of the triangle.
        // They will be merged later, in vboindexer.cpp
        tangents.push_back(tangent);
        tangents.push_back(tangent);
        tangents.push_back(tangent);

        // Same thing for bitangents
        bitangents.push_back(bitangent);
        bitangents.push_back(bitangent);
        bitangents.push_back(bitangent);
    }
}

void generateBufferWhitNode(Mesh &mesh, SceneNode &node) {

    unsigned int vaoID;
    glGenVertexArrays(1, &vaoID);
    glBindVertexArray(vaoID);
    node.vertexBufferID = generateAttribute(0, 3, mesh.vertices, false);

    generateAttribute(1, 3, mesh.normals, true);
    if (mesh.textureCoordinates.size() > 0) {
        generateAttribute(2, 2, mesh.textureCoordinates, false);
    }

    node.vertexArrayObjectID = vaoID;
    node.VAOIndexCount = mesh.indices.size();

    unsigned int indexBufferID;
    glGenBuffers(1, &indexBufferID);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBufferID);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh.indices.size() * sizeof(unsigned int), mesh.indices.data(), GL_STATIC_DRAW);
    node.indexBufferID = indexBufferID;
    
    node.noiseBufferID = generateAttribute(5, 1, std::vector<glm::vec1>(mesh.vertices.size()), false);

    
    if (mesh.normals.size() > 0 ) {
        computeTangentBasis(
            mesh.vertices, mesh.textureCoordinates, mesh.normals,
            mesh.tangent, mesh.bitangent);

        GLuint tangentbuffer;
        glGenBuffers(1, &tangentbuffer);
        glBindBuffer(GL_ARRAY_BUFFER, tangentbuffer);
        glBufferData(GL_ARRAY_BUFFER, mesh.tangent.size() * sizeof(glm::vec3), &mesh.tangent[0], GL_STATIC_DRAW);
        node.tangetBufferID = generateAttribute(3, 3, mesh.tangent, false);

        GLuint bitangentbuffer;
        glGenBuffers(1, &bitangentbuffer);
        glBindBuffer(GL_ARRAY_BUFFER, bitangentbuffer);
        glBufferData(GL_ARRAY_BUFFER, mesh.bitangent.size() * sizeof(glm::vec3), &mesh.bitangent[0], GL_STATIC_DRAW);
        node.bitangentBufferID = generateAttribute(4, 3, mesh.bitangent, false);
    }
}
unsigned int generateFrameBufferObject(unsigned int depthTexture) {

    unsigned int framebuffer;
    glGenFramebuffers(1, &framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthTexture, 0);


    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cout << "Error: Framebuffer is not complete!" << std::endl;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0); // Unbind the framebuffer

    return framebuffer;
}

unsigned int generateDepthTextureObject(int screenWidth, int screenHeight) {
    unsigned int depthTexture;
    glGenTextures(1, &depthTexture);
    glBindTexture(GL_TEXTURE_2D, depthTexture);

    // Set texture parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, screenWidth, screenHeight, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);

    return depthTexture;
}

unsigned int generateBuffer(Mesh& mesh) {

    unsigned int vaoID;
    glGenVertexArrays(1, &vaoID);
    glBindVertexArray(vaoID);
    generateAttribute(0, 3, mesh.vertices, false);
    generateAttribute(1, 3, mesh.normals, true);
    if (mesh.textureCoordinates.size() > 0) {
        generateAttribute(2, 2, mesh.textureCoordinates, false);
    }

    unsigned int indexBufferID;
    glGenBuffers(1, &indexBufferID);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBufferID);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh.indices.size() * sizeof(unsigned int), mesh.indices.data(), GL_STATIC_DRAW);


    if (mesh.normals.size() > 0) {
        computeTangentBasis(
            mesh.vertices, mesh.textureCoordinates, mesh.normals,
            mesh.tangent, mesh.bitangent);

        GLuint tangentbuffer;
        glGenBuffers(1, &tangentbuffer);
        glBindBuffer(GL_ARRAY_BUFFER, tangentbuffer);
        glBufferData(GL_ARRAY_BUFFER, mesh.tangent.size() * sizeof(glm::vec3), &mesh.tangent[0], GL_STATIC_DRAW);
        generateAttribute(3, 3, mesh.tangent, false);

        GLuint bitangentbuffer;
        glGenBuffers(1, &bitangentbuffer);
        glBindBuffer(GL_ARRAY_BUFFER, bitangentbuffer);
        glBufferData(GL_ARRAY_BUFFER, mesh.bitangent.size() * sizeof(glm::vec3), &mesh.bitangent[0], GL_STATIC_DRAW);
        generateAttribute(4, 3, mesh.bitangent, false);
    }

    return vaoID;
}


    