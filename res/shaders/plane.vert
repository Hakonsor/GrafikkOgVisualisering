#version 430 core

in layout(location = 0) vec3 position;
in layout(location = 2) vec2 textureCoordinates_in;

uniform layout(location = 3) mat4 M;
uniform layout(location = 4) mat4 P;
uniform layout(location = 5) mat4 V;
uniform layout(location = 9) vec3 eye;

out layout(location = 1) vec2 textureCoordinates_out;
out layout(location = 2) vec3 vertPos;

void main()
{
    vec4 vertpos4 = (  V * M ) * vec4(position, 1.0f);
    vertPos = vec3(vertpos4) / vertpos4.w;
    textureCoordinates_out = textureCoordinates_in;
    gl_Position = vec4(position.x,position.y ,0, 1.0f);
}
