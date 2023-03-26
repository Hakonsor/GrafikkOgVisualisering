#version 430 core

out vec4 color;
layout(binding = 0) uniform sampler2D diffusetexture;

in layout(location = 1) vec2 textureCoordinates;




void main()
{
    vec4 col = texture( diffusetexture , textureCoordinates);
    color = vec4(col);

}
