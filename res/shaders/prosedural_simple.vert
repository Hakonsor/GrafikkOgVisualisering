#version 430 core

layout (location = 1) in vec3 position; // 1 fra index buffer
layout (location = 2) in vec4 colour;
layout (location = 2) out vec4 frag; 
layout (location = 3) in vec3 normalvert;
layout (location = 3) out vec3 normal; 

uniform layout (location = 0) mat4 matrix; 
uniform layout (location = 5) mat3 change; 

void main()
{
    gl_Position = matrix * vec4(position, 1.0);
    frag = colour; 
    normal =  change * normalvert;


}