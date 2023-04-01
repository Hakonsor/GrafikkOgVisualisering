#include <iostream>
#include "shapes.h"
#include <array>

#ifndef M_PI
#define M_PI 3.14159265359f
#endif

Mesh cube(glm::vec3 scale, glm::vec2 textureScale, bool tilingTextures, bool inverted, glm::vec3 textureScale3d) {
    glm::vec3 points[8];
    int indices[36];

    for (int y = 0; y <= 1; y++)
    for (int z = 0; z <= 1; z++)
    for (int x = 0; x <= 1; x++) {
        points[x+y*4+z*2] = glm::vec3(x*2-1, y*2-1, z*2-1) * 0.5f * scale;
    }

    int faces[6][4] = {
        {2,3,0,1}, // Bottom 
        {4,5,6,7}, // Top 
        {7,5,3,1}, // Right 
        {4,6,0,2}, // Left 
        {5,4,1,0}, // Back 
        {6,7,2,3}, // Front 
    };

    scale = scale * textureScale3d;
    glm::vec2 faceScale[6] = {
        {-scale.x,-scale.z}, // Bottom
        {-scale.x,-scale.z}, // Top
        { scale.z, scale.y}, // Right
        { scale.z, scale.y}, // Left
        { scale.x, scale.y}, // Back
        { scale.x, scale.y}, // Front
    }; 

    glm::vec3 normals[6] = {
        { 0,-1, 0}, // Bottom 
        { 0, 1, 0}, // Top 
        { 1, 0, 0}, // Right 
        {-1, 0, 0}, // Left 
        { 0, 0,-1}, // Back 
        { 0, 0, 1}, // Front 
    };

    glm::vec2 UVs[4] = {
        {0, 0},
        {0, 1},
        {1, 0},
        {1, 1},
    };

    Mesh m;
    for (int face = 0; face < 6; face++) {
        int offset = face * 6;
        indices[offset + 0] = faces[face][0];
        indices[offset + 3] = faces[face][0];

        if (!inverted) {
            indices[offset + 1] = faces[face][3];
            indices[offset + 2] = faces[face][1];
            indices[offset + 4] = faces[face][2];
            indices[offset + 5] = faces[face][3];
        } else {
            indices[offset + 1] = faces[face][1];
            indices[offset + 2] = faces[face][3];
            indices[offset + 4] = faces[face][3];
            indices[offset + 5] = faces[face][2];
        }

        for (int i = 0; i < 6; i++) {
            m.vertices.push_back(points[indices[offset + i]]);
            m.indices.push_back(offset + i);
            m.normals.push_back(normals[face] * (inverted ? -1.f : 1.f));
        }

        glm::vec2 textureScaleFactor = tilingTextures ? (faceScale[face] / textureScale) : glm::vec2(1);

        if (!inverted) {
            for (int i : {1,2,3,1,0,2}) {
                m.textureCoordinates.push_back(UVs[i] * textureScaleFactor);
            }
        } else {
            for (int i : {3,1,0,3,0,2}) {
                m.textureCoordinates.push_back(UVs[i] * textureScaleFactor);
            }
        }
    }

    return m;
}

Mesh sphereCube(glm::vec3 scale, glm::vec2 textureScale, bool tilingTextures, bool inverted, glm::vec3 textureScale3d, float radius, glm::vec3 cameraPosition) {
    glm::vec3 points[8];
    int indices[36];
    //std::array<int, 6> lodPerFace = {20, 20, 20, 20, 20, 20};
    std::array<int, 6> lodPerFace;
    for (int y = 0; y <= 1; y++)
        for (int z = 0; z <= 1; z++)
            for (int x = 0; x <= 1; x++) {
                points[x + y * 4 + z * 2] = glm::vec3(x * 2 - 1, y * 2 - 1, z * 2 - 1) * 0.5f * scale;
            }

    int faces[6][4] = {
        {2,3,0,1}, // Bottom 
        {4,5,6,7}, // Top 
        {7,5,3,1}, // Right 
        {4,6,0,2}, // Left 
        {5,4,1,0}, // Back 
        {6,7,2,3}, // Front 
    };

    scale = scale * textureScale3d;
    glm::vec2 faceScale[6] = {
        {-scale.x,-scale.z}, // Bottom
        {-scale.x,-scale.z}, // Top
        { scale.z, scale.y}, // Right
        { scale.z, scale.y}, // Left
        { scale.x, scale.y}, // Back
        { scale.x, scale.y}, // Front
    };

    glm::vec3 normals[6] = {
        { 0,-1, 0}, // Bottom 
        { 0, 1, 0}, // Top 
        { 1, 0, 0}, // Right 
        {-1, 0, 0}, // Left 
        { 0, 0,-1}, // Back 
        { 0, 0, 1}, // Front 
    };

    glm::vec2 UVs[4] = {
        {0, 0},
        {0, 1},
        {1, 0},
        {1, 1},
    };

    float distanceThreshold = 30.0f;

    Mesh m;
    for (int face = 0; face < 6; face++) {
        // Calculate the face center
        glm::vec3 faceCenter = (points[faces[face][0]] + points[faces[face][1]] + points[faces[face][2]] + points[faces[face][3]]) * 0.25f;
        glm::vec3 sphericalCenter = glm::normalize(faceCenter) * radius;

        // Calculate the distance from the camera to the face center
        float distance = glm::distance(cameraPosition, sphericalCenter);

        // Update the LOD value based on the distance
        if (distance < distanceThreshold) {
            lodPerFace[face] = 4; // Higher LOD for closer faces
        }
        else {
            lodPerFace[face] = 1; // Lower LOD for farther faces
        }


        int lod = lodPerFace[face];
        float step = 1.0f / lod;
        printf("\n Side:%d, lod:%f  ", face, lod);
        for (float row = 0; row < lod; ++row) {
            for (float col = 0; col < lod; ++col) {
                glm::vec3 quadPoints[4];
                printf("\n rad:%d, col:%d ", row, col);
                for (int y = 0; y <= 1; y++)
                    for (int x = 0; x <= 1; x++) {
                        int idx = x + y * 2;
                        glm::vec3 topLeft = points[faces[face][0]];
                        glm::vec3 topRight = points[faces[face][1]];
                        glm::vec3 bottomLeft = points[faces[face][2]];
                        glm::vec3 bottomRight = points[faces[face][3]];

                        float u = x * step + col * step;
                        float v = y * step + row * step;

                        glm::vec3 top = glm::mix(topLeft, topRight, u);
                        glm::vec3 bottom = glm::mix(bottomLeft, bottomRight, u);
                        glm::vec3 point = glm::mix(top, bottom, v);

                        glm::vec3 sphericalPoint = glm::normalize(point) * radius; // Normalize and scale by radius
                        quadPoints[idx] = sphericalPoint; // Directly use the sphericalPoint
                        printf(" (%g, %g) ", point.x, point.y);
                    }
                //for (int y = 0; y <= 1; y++)
                //    for (int x = 0; x <= 1; x++) {
                //        int idx = x + y * 2;
                //        glm::vec3 point = points[faces[face][idx]];
                //        // 15 = 15 + (0.5 * 1 * 1) = 15.5

                //        point.x += step * col;
                //        point.y += step * row;
                //        glm::vec3 sphericalPoint = glm::normalize(point) * radius; // Normalize and scale by radius
                //        quadPoints[idx] = sphericalPoint; // Directly use the sphericalPoint
                //        printf(" (%g, %g) ", point.x, point.y);
                //    }



                // Calculate the offset for each quad
                int offset = m.vertices.size();

                // Add vertices and indices
                for (int i = 0; i < 4; ++i) {
                    glm::vec3 vertex = quadPoints[i];
                    m.vertices.push_back(vertex);
                    m.normals.push_back(normals[face] * (inverted ? -1.f : 1.f));
                }

                if (!inverted) {
                    m.indices.push_back(offset + 0);
                    m.indices.push_back(offset + 2);
                    m.indices.push_back(offset + 1);
                    m.indices.push_back(offset + 2);
                    m.indices.push_back(offset + 3);
                    m.indices.push_back(offset + 1);
                }
                else {
                    m.indices.push_back(offset + 0);
                    m.indices.push_back(offset + 1);
                    m.indices.push_back(offset + 2);
                    m.indices.push_back(offset + 2);
                    m.indices.push_back(offset + 1);
                    m.indices.push_back(offset + 3);
                }

                glm::vec2 textureScaleFactor = tilingTextures ? (faceScale[face] / textureScale) : glm::vec2(1);
                for (int i : {0, 2, 1, 2, 3, 1}) {
                    m.textureCoordinates.push_back(UVs[i] * textureScaleFactor);
                }
            }
        }
    }


    return m;
}

Mesh generateSphere(float sphereRadius, int slices, int layers) {
    const unsigned int triangleCount = slices * layers * 2;

    std::vector<glm::vec3> vertices;
    std::vector<glm::vec3> normals;
    std::vector<unsigned int> indices;
    std::vector<glm::vec2> uvs;

    vertices.reserve(3 * triangleCount);
    normals.reserve(3 * triangleCount);
    indices.reserve(3 * triangleCount);

    // Slices require us to define a full revolution worth of triangles.
    // Layers only requires angle varying between the bottom and the top (a layer only covers half a circle worth of angles)
    const float degreesPerLayer = 180.0 / (float) layers;
    const float degreesPerSlice = 360.0 / (float) slices;

    unsigned int i = 0;

    // Constructing the sphere one layer at a time
    for (int layer = 0; layer < layers; layer++) {
        int nextLayer = layer + 1;

        // Angles between the vector pointing to any point on a particular layer and the negative z-axis
        float currentAngleZDegrees = degreesPerLayer * layer;
        float nextAngleZDegrees = degreesPerLayer * nextLayer;

        // All coordinates within a single layer share z-coordinates.
        // So we can calculate those of the current and subsequent layer here.
        float currentZ = -cos(glm::radians(currentAngleZDegrees));
        float nextZ = -cos(glm::radians(nextAngleZDegrees));

        // The row of vertices forms a circle around the vertical diagonal (z-axis) of the sphere.
        // These radii are also constant for an entire layer, so we can precalculate them.
        float radius = sin(glm::radians(currentAngleZDegrees));
        float nextRadius = sin(glm::radians(nextAngleZDegrees));

        // Now we can move on to constructing individual slices within a layer
        for (int slice = 0; slice < slices; slice++) {

            // The direction of the start and the end of the slice in the xy-plane
            float currentSliceAngleDegrees = slice * degreesPerSlice;
            float nextSliceAngleDegrees = (slice + 1) * degreesPerSlice;

            // Determining the direction vector for both the start and end of the slice
            float currentDirectionX = cos(glm::radians(currentSliceAngleDegrees));
            float currentDirectionY = sin(glm::radians(currentSliceAngleDegrees));

            float nextDirectionX = cos(glm::radians(nextSliceAngleDegrees));
            float nextDirectionY = sin(glm::radians(nextSliceAngleDegrees));

            vertices.emplace_back(sphereRadius * radius * currentDirectionX,
                                  sphereRadius * radius * currentDirectionY,
                                  sphereRadius * currentZ);
            vertices.emplace_back(sphereRadius * radius * nextDirectionX,
                                  sphereRadius * radius * nextDirectionY,
                                  sphereRadius * currentZ);
            vertices.emplace_back(sphereRadius * nextRadius * nextDirectionX,
                                  sphereRadius * nextRadius * nextDirectionY,
                                  sphereRadius * nextZ);
            vertices.emplace_back(sphereRadius * radius * currentDirectionX,
                                  sphereRadius * radius * currentDirectionY,
                                  sphereRadius * currentZ);
            vertices.emplace_back(sphereRadius * nextRadius * nextDirectionX,
                                  sphereRadius * nextRadius * nextDirectionY,
                                  sphereRadius * nextZ);
            vertices.emplace_back(sphereRadius * nextRadius * currentDirectionX,
                                  sphereRadius * nextRadius * currentDirectionY,
                                  sphereRadius * nextZ);

            normals.emplace_back(radius * currentDirectionX,
                                 radius * currentDirectionY,
                                 currentZ);
            normals.emplace_back(radius * nextDirectionX,
                                 radius * nextDirectionY,
                                 currentZ);
            normals.emplace_back(nextRadius * nextDirectionX,
                                 nextRadius * nextDirectionY,
                                 nextZ);
            normals.emplace_back(radius * currentDirectionX,
                                 radius * currentDirectionY,
                                 currentZ);
            normals.emplace_back(nextRadius * nextDirectionX,
                                 nextRadius * nextDirectionY,
                                 nextZ);
            normals.emplace_back(nextRadius * currentDirectionX,
                                 nextRadius * currentDirectionY,
                                 nextZ);

            indices.emplace_back(i + 0);
            indices.emplace_back(i + 1);
            indices.emplace_back(i + 2);
            indices.emplace_back(i + 3);
            indices.emplace_back(i + 4);
            indices.emplace_back(i + 5);

            for (int j = 0; j < 6; j++) {
                glm::vec3 vertex = vertices.at(i+j);
                uvs.emplace_back(
                    0.5 + (glm::atan(vertex.z, vertex.y)/(2.0*M_PI)),
                    0.5 - (glm::asin(vertex.y)/M_PI)
                );
            }

            i += 6;
        }
    }

    Mesh mesh;
    mesh.vertices = vertices;
    mesh.normals = normals;
    mesh.indices = indices;
    mesh.textureCoordinates = uvs;
    return mesh;
}
