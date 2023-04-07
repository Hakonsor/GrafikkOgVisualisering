#version 430 core

in layout(location = 0) vec3 position;
in layout(location = 1) vec3 normal_in;
in layout(location = 2) vec2 textureCoordinates_in;
in layout(location = 3) vec3 tangent;
in layout(location = 4) vec3 bitangent;
//in layout(location = 5) float distanceFromCenter;

uniform layout(location = 3) mat4 M;
uniform layout(location = 4) mat4 P;
uniform layout(location = 5) mat4 V;
uniform layout(location = 13) mat3 normalMatrix;



out layout(location = 0) vec3 normal_out;
out layout(location = 1) vec2 textureCoordinates_out;
out layout(location = 2) vec3 vertPos;
out layout(location = 7) vec3 vPosition;
out layout(location = 3) mat3 TBN;



//ny




void main()
{
    // Task 1e)
  // fragDistanceFromCenter = distanceFromCenter;
   normal_out = normalize(normalMatrix * normal_in);
    vec3 tangent_out =  normalize( normalMatrix* tangent);
    vec3 bitangent_out =  normalize(normalMatrix* bitangent);
//     vPosition = vec3(position);
    vPosition = position;
    TBN = mat3(
        tangent_out,
        bitangent_out,
        normal_out
    );
    
    vec4 vertpos4 = (  V * M ) * vec4(position, 1.0f);
    vertPos = vec3(vertpos4) / vertpos4.w;
    textureCoordinates_out = textureCoordinates_in;
    gl_Position = ( P * V * M ) * vec4(position, 1.0f);
}
