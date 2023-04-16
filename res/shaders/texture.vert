#version 430 core

in layout(location = 0) vec3 position;
in layout(location = 1) vec3 normal_in;
in layout(location = 2) vec2 textureCoordinates_in;

uniform layout(location = 3) mat4 M;
uniform layout(location = 4) mat4 P;
uniform layout(location = 5) mat4 V;



uniform layout(location = 13) mat3 normalMatrix;

out layout(location = 1) vec2 textureCoordinates_out;



void main()
{
    
    textureCoordinates_out = textureCoordinates_in;
    gl_Position = ( M) * vec4(position, 1.0f);
}
