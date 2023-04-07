#include <iostream>
#include "glfont.h"

/*
    text: The text to be rendered.
    characterHeightOverWidth: The ratio of an individual characters height over its width.
    totalTextWidth: The total width of the entire text-mesh.
*/
Mesh generateTextGeometryBuffer(std::string text, float characterHeightOverWidth, float totalTextWidth) {
    float characterWidth = totalTextWidth / float(text.length());
    float characterHeight = characterHeightOverWidth * characterWidth;

    unsigned int vertexCount = 4 * text.length();
    unsigned int indexCount = 6 * text.length();

    Mesh mesh;
    mesh.vertices.resize(vertexCount);
    mesh.indices.resize(indexCount);
    mesh.textureCoordinates.resize(vertexCount);

    for(unsigned int i = 0; i < text.length(); i++)
    {
        float baseXCoordinate = float(i) * characterWidth;
       
        mesh.vertices.at(4 * i + 0) = {baseXCoordinate, 0, 0};
        mesh.vertices.at(4 * i + 1) = {baseXCoordinate + characterWidth, 0, 0};
        mesh.vertices.at(4 * i + 2) = {baseXCoordinate + characterWidth, characterHeight, 0};

        float ASKIIchar = text[i];
        float ASKIIcharwhith = 29;
        float right = (ASKIIchar * ASKIIcharwhith) + ASKIIcharwhith; 
        float left = (ASKIIchar * ASKIIcharwhith);
        int totalTextureWidth = 3712;
        right = right / totalTextureWidth;
        left = left / totalTextureWidth;
        float top = 1.0f;
        float botton = 0.0f;
        float noe = text[i] / 128;

        mesh.textureCoordinates.at(4 * i + 0) = { ASKIIchar / 128.0, botton };
        mesh.textureCoordinates.at(4 * i + 1) = { (ASKIIchar + 1.0) / 128.0, botton };
        mesh.textureCoordinates.at(4 * i + 2) = { (ASKIIchar + 1.0) / 128.0, top };
        mesh.textureCoordinates.at(4 * i + 3) = { ASKIIchar / 128.0, top };

        mesh.vertices.at(4 * i + 0) = {baseXCoordinate, 0, 0};
        mesh.vertices.at(4 * i + 2) = {baseXCoordinate + characterWidth, characterHeight, 0};
        mesh.vertices.at(4 * i + 3) = {baseXCoordinate, characterHeight, 0};
        
        mesh.indices.at(6 * i + 0) = 4 * i + 0;
        mesh.indices.at(6 * i + 1) = 4 * i + 1;
        mesh.indices.at(6 * i + 2) = 4 * i + 2;
        mesh.indices.at(6 * i + 3) = 4 * i + 0;
        mesh.indices.at(6 * i + 4) = 4 * i + 2;
        mesh.indices.at(6 * i + 5) = 4 * i + 3;
       
        
    }

    return mesh;
}