#version 430 core

struct Lights {
    vec3 position;
    vec3 color;
};
struct Biomes {
    float startheight;
};
in layout(location = 0) vec3 in_normal;
in layout(location = 1) vec2 textureCoordinates;
in layout(location = 2) vec3 pos;

in layout(location = 3) mat3 TBN;
in layout(location = 7) vec3 vpos;
in layout(location = 8) vec2 v_noise;

uniform Lights lights[3];
uniform Biomes biomes[7];
//uniform float distanceThreshold;

uniform layout(location = 5) mat4 V;
uniform layout(location = 6) int normal_geo;
uniform layout(location = 8) vec2 minmax;

uniform layout(location = 9) vec3 eye;
uniform layout(location = 10) int numBiomes;
uniform layout(location = 11) vec3 center;
uniform layout(location = 12) float noiseheight;
uniform layout(location = 15) vec3 ballpos;

layout(binding = 3) uniform sampler2D elevationcolor;
layout(binding = 2) uniform sampler2D roughness_texture;
layout(binding = 1) uniform sampler2D normal_texture;
layout(binding = 0) uniform sampler2D diffusetexture;

out vec4 color;


float rand(vec2 co) { return fract(sin(dot(co.xy, vec2(12.9898,78.233))) * 43758.5453); }
float dither(vec2 uv) { return (rand(uv)*2.0-1.0) / 256.0; }

float diffuse_reflekton = 0.6;
// Task 1 f)
float ambient = 0.2;
float shininessVal = 32;
vec3 lightSpecular;

float la = 0.0009;
float lb = 0.0009;
float lc = 0.0009;

float radius = 3;
float softradius = 4;


float diffuse(vec3 n, vec3 l){

    float nl = dot(n, l);
    float kd = diffuse_reflekton * nl;
    return max(kd, 0);
}

float specular(vec3 n, vec3 l, vec3 campos ){

    vec3 R = reflect(-n, l);   
    vec3 V = normalize(campos); 
    float specAngle = max(dot(R, V), 0.0);
    if(normal_geo != 1){
        return pow(specAngle, shininessVal);
    }
    float roughnessFactor = 5.0 / pow(texture(roughness_texture, textureCoordinates).r, 2.0);
    float specular = pow(specAngle, roughnessFactor); 
     //specular = pow(specAngle, shininessVal);
    return specular;
}

float attenuation(vec3 lightL ){
    float dist = length(lightL);
    return 1/(la + dist * lb + dist * dist * lc);
}

vec3 reject(vec3 from, vec3 onto) {
    return from - onto*dot(from, onto)/dot(onto, onto);
}

float map(float value, float min1, float max1, float min2, float max2) {
  return min2 + (value - min1) * (max2 - min2) / (max1 - min1);
}

float inverseLerp(float a, float b, float value) {
    if (a != b) {
        return (value - a) / (b - a);
    }
    else {
        return 0.0f;
    }
}


float BiomePercentFromPoint(float heightPercent) {
    float biomeIndex = 0;
    float blent = 1.0f;//todo make uniform
    float blendRange = blent / 2.0f + .001f;
    for (int i = 0; i < numBiomes; i++) {
        float dist = heightPercent   - biomes[i].startheight;
        float weight = inverseLerp(-blendRange, blendRange, dist );
        biomeIndex *= (1-weight);
        biomeIndex += i*weight;
    }
    return biomeIndex / float(max(1, numBiomes - 1));
}


void main()
{
    float distanceFromCenter = distance(vpos, center);
    float elevation = inverseLerp(minmax.x, minmax.y, distanceFromCenter);
    float normalizedY = (vpos.y - (center.y - distanceFromCenter)) / (2.0 * distanceFromCenter);
    normalizedY += v_noise.x;
    vec4 pixelcolor = texture(elevationcolor, vec2(elevation, BiomePercentFromPoint(normalizedY)));

    vec3 normal; 
    if (normal_geo == 1 ) {
        normal =   normalize(TBN *(texture( normal_texture , textureCoordinates) * 2 -1).xyz);
    }  else {
        normal =  normalize(vec3(1));
    }

    vec4 ballVec4 =  ( V ) * vec4(ballpos, 1.0f);
    vec3 homo_ballPos = vec3(ballVec4) / ballVec4.w;

    vec4 eyeVec4 =  (  V  ) * vec4(eye, 1.0f);
    vec3 homo_eye = vec3(eyeVec4) / eyeVec4.w;

    vec3 campos = normalize(homo_eye-pos);

    vec3 diffuseTotal = vec3(0) + ambient;
    
    for (int i = 0; i < lights.length(); i++)
    {
        vec4 homo = (  V  ) * vec4(lights[i].position, 1.0f);
        vec3 lightpos =  vec3(homo) / homo.w;

        // Task 1 g) diffuse
        vec3 light = diffuse(normal, normalize(lightpos - pos)) * lights[i].color;
        // Task 2 a) attenuation
        light *= attenuation(lightpos-pos);
        // Task 1 h) Specular
        if(length(light) > 0.0) {
            
            lightSpecular = specular(normal, normalize(lightpos - pos), campos) * lights[i].color;
            
            // Task 2 a) attenuation
            lightSpecular *= attenuation(lightpos-pos);
            light += lightSpecular;
        }

        // Task 3 b) shadow
        vec3 L = (lightpos - pos); 
        vec3 B = (homo_ballPos -pos);
        if(length(L) > length(B) && dot(L,B) > 0){
            vec3 re = reject(L, B);
            if(length(re) < radius){
                light = vec3(0);
            }
            // Task 4 (OPTIONAL): Getting softer with age
            if(length(re) < softradius){
                light *= map(length(re), radius, softradius, 0 , 1);
            }
        }
        // Task 1 i)
        diffuseTotal += light;
    }

    
    float dilter = dither(textureCoordinates);

    if (minmax.y < distanceFromCenter){
        vec4 diftexture = texture(diffusetexture, textureCoordinates);
        color = vec4((diftexture.xyz*diffuseTotal)+dilter, diftexture.w) ;
    } else{
        color = vec4(((pixelcolor.xyz)*diffuseTotal)+dilter, pixelcolor.w);
    }

    }
