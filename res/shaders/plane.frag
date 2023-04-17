#version 430 core

struct Lights {
    vec3 position;
    vec3 color;
};

in layout(location = 1) vec2 textureCoordinates;
in layout(location = 2) vec3 pos;

uniform layout(location = 3) mat4 M;
uniform layout(location = 4) mat4 P;
uniform layout(location = 5) mat4 V;
uniform layout(location = 8) vec2 minmax;

uniform float densityFalloff;
uniform float scatteringStrength;
uniform float intensity;
uniform float numInScatteringPoints;
uniform float numOpticalDepthPoints;


uniform Lights lights[1];

uniform layout(location = 9) vec3 eye;
uniform layout(location = 15) vec3 ballpos;
uniform layout(location = 17) vec3 atmosphereCenter;
uniform layout(location = 18) vec2 atmosphereMinMax;

layout(binding = 5) uniform sampler2D depthTexture;
layout(binding = 4) uniform sampler2D colorTexture;
layout(binding = 3) uniform sampler2D elevationcolor;

out vec4 color;

float inverseLerp(float a, float b, float value);
vec2 raySphere(vec3 sphereCenter, float sphereradius, vec3 rayOrigin, vec3 rayDir );

// asmophere
 void initColor();
float atmosphericRadius = ((atmosphereMinMax.y+atmosphereMinMax.x)/2) * 1.1;
float planetRadius = (atmosphereMinMax.y+atmosphereMinMax.x)/2; // max + 10?


vec3 wavelength = vec3(700, 530, 440);
float scatterR;
float scatterB;
float scatterG;
vec3 scatteringCoefficients ;


float LinearEyeDepth(float depth) {
    float near = 0.1; // Camera's near plane
    float far = 1000.0; // Camera's far plane
    return (2.0 * near) / (far + near - depth * (far - near));
}

float densityAtPoint(vec3 densitySamplePoint){
    float heightAboveSurface = length(densitySamplePoint - atmosphereCenter) - planetRadius;
    float height01 = heightAboveSurface / (atmosphericRadius - planetRadius);
    float localDensity = exp(-height01*densityFalloff) * (1 - height01);

    return localDensity;
}

float opticalDepth(vec3 rayOrigin, vec3 rayDirection, float rayLength ){
    vec3 densitySamplePoint = rayOrigin;
    float stepSize = rayLength / (numOpticalDepthPoints -1);
    float opticalDepth = 0;

    for(int i = 0; i < numOpticalDepthPoints; i++){
        float localDensity = densityAtPoint(densitySamplePoint);
        opticalDepth += localDensity * stepSize;
        densitySamplePoint += rayDirection * stepSize;
    }
    return opticalDepth;
}

vec3 calculateLight(vec3 rayOrigin, vec3 rayDir, float distanceThroughAmoshpere, vec3 originalColor, float distanceToSurface){
    vec3 dirToSun = -lights[0].position;;
    vec3 inScatterPoint = rayOrigin;
    float stepSize = distanceThroughAmoshpere / (numInScatteringPoints -1);
    vec3 inScatteredLight = vec3(0);
    float viewRayOpticalDepth = 0;
       
    for(int i = 0; i < numInScatteringPoints; i++){
        float sunRayLength = raySphere(atmosphereCenter, atmosphericRadius, inScatterPoint, dirToSun).y;
        float sunRayOpticalDepth = opticalDepth(inScatterPoint, dirToSun, sunRayLength);
        viewRayOpticalDepth = opticalDepth(inScatterPoint, -rayDir, stepSize * i);
        vec3 transmittance = exp(-(sunRayOpticalDepth+viewRayOpticalDepth) * scatteringCoefficients);  
        float localDensity = densityAtPoint(inScatterPoint);
        inScatteredLight += localDensity * transmittance * scatteringCoefficients * stepSize;
        inScatterPoint += rayDir * stepSize; 
    }
    float originalColTransmittance = exp(-viewRayOpticalDepth);
    return originalColor * originalColTransmittance + inScatteredLight * intensity;
}

vec4 asmophere(vec4 originalColor,  float sphereRadius, float depth, vec3 viewDirection){
    float oceanRadius = minmax.x;

    vec3 rayOrigin = eye;
    vec3 rayDirection = normalize(viewDirection);
    float sceneDepth = depth * length(rayDirection);
    // sea not implemented
    float distanceToOcean = raySphere(atmosphereCenter, oceanRadius, rayOrigin, rayDirection).x;
    float distanceToSurface = min(sceneDepth, distanceToOcean);

    vec2 hitInfo = raySphere(atmosphereCenter, sphereRadius, rayOrigin, rayDirection);
    float distanceToAtmosphere = hitInfo.x;
    float distanceThroughAtmosphere =  min(hitInfo.y, distanceToSurface - distanceToAtmosphere);

    if (distanceThroughAtmosphere > 0) {
        const float epsilon = 0.0001;
        vec3 pointInAtmosphere = rayOrigin + rayDirection * (distanceToAtmosphere + epsilon);
        vec3 light = calculateLight(pointInAtmosphere, rayDirection, distanceThroughAtmosphere - epsilon * 2, originalColor.xyz, distanceToSurface);

        return vec4(light, 1);//vec4(originalColor.xyz * (1 - light) + light, originalColor.w);
    }
    return originalColor;
   }


 vec4 asmophere2(vec4 originalColor,  float sphereRadius, float depth, vec3 viewDirection){
//    float oceanRadius = minmax.x;

    vec3 rayOrigin = eye;
    vec3 rayDirection = normalize(viewDirection);
    float sceneDepth = depth * length(rayDirection);

//    float distanceToOcean = raySphere(atmosphereCenter, oceanRadius, rayOrigin, rayDirection).x;
    float raydistanceToSurface = raySphere(atmosphereCenter, planetRadius, rayOrigin, rayDirection).x;
    float distanceToSurface = sceneDepth;
    vec2 hitInfo = raySphere(atmosphereCenter, sphereRadius, rayOrigin, rayDirection);
    float distanceToAtmosphere = hitInfo.x;
    float raydistanceThroughAtmosphere =  min(hitInfo.y, raydistanceToSurface - distanceToAtmosphere);
    float distanceThroughAtmosphere =  min(hitInfo.y, distanceToSurface - distanceToAtmosphere);

    if ( raydistanceThroughAtmosphere > 0) {
        const float epsilon = 0.0001;
        vec3 pointInAtmosphere = rayOrigin + rayDirection * (distanceToAtmosphere + epsilon);
        vec3 light = calculateLight(pointInAtmosphere, rayDirection, distanceThroughAtmosphere - epsilon * 2, originalColor.xyz, distanceToSurface);

        return vec4(light, 1);
    }
    return originalColor;
 } 
//vec4 asmophere(vec4 originalColor,  float sphereRadius, float depth, vec3 viewDirection){
// 
//     float oceanRadius = minmax.x;
//
//    vec3 rayOrigin = eye;
//    vec3 rayDirection = normalize(viewDirection);
//    float sceneDepth = depth * length(rayDirection);
//    // sea not implemented
//    // float distanceToOcean = raySphere(atmosphereCenter, oceanRadius, rayOrigin, rayDirection).x;
//    float distanceToSurface = sceneDepth; // min(sceneDepth, distanceToOcean);
//
//    vec2 hitInfo = raySphere(atmosphereCenter, sphereRadius, rayOrigin, rayDirection);
//    float distanceToAtmosphere = hitInfo.x;
//    float distanceThroughAtmosphere = distanceToSurface - distanceToAtmosphere; // min(hitInfo.y, distanceToSurface - distanceToAtmosphere);
//
//    if (distanceThroughAtmosphere > 0) {
//        const float epsilon = 0.0001;
//        vec3 pointInAtmosphere = rayOrigin + rayDirection * (distanceToAtmosphere + epsilon);
//        vec3 light = calculateLight(pointInAtmosphere, rayDirection, distanceThroughAtmosphere - epsilon * 2, originalColor.xyz, distanceToSurface);
//
//        return vec4(originalColor.xyz * (1 - light) + light, originalColor.w);
//}
//return originalColor;
// 
// }

void main()
{
    vec4 updatedcolor =  texture(colorTexture, textureCoordinates);
    float depth = texture(depthTexture, textureCoordinates).r;

    vec2 screenSize = vec2(1366, 768);
    vec2 screenPos = gl_FragCoord.xy / screenSize;
    vec4 clipSpacePos = vec4((screenPos * 2.0 - 1.0), depth * 2.0 - 1.0, 1.0);

    mat4 invProjection = inverse(P);
    mat4 invView = inverse(V);

    vec4 eyeSpacePos = invProjection * clipSpacePos;
    eyeSpacePos /= eyeSpacePos.w;

    vec3 viewDirection = normalize(eyeSpacePos.xyz);
    
    initColor();

    vec4 atmosphericEffect = updatedcolor;
//    atmosphericEffect = asmophere2(updatedcolor, atmosphericRadius, depth, eyeSpacePos.xyz);
//    atmosphericEffect = asmophere(updatedcolor, atmosphericRadius, depth, eyeSpacePos.xyz);
//    if(distance(eye, atmosphereCenter) < atmosphericRadius){
//        atmosphericEffect = asmophere(updatedcolor, atmosphericRadius, depth, eyeSpacePos.xyz);
//    }else {
        atmosphericEffect = asmophere2(updatedcolor, atmosphericRadius, depth, eyeSpacePos.xyz);
////        
//    }


    color = atmosphericEffect;

    }


float inverseLerp(float a, float b, float value) {
    if (a != b) {
        return (value - a) / (b - a);
    }
    else {
        return 0.0f;
    }
}
vec2 raySphere(vec3 sphereCenter, float sphereradius, vec3 rayOrigin, vec3 rayDir ){
    
    vec3 offset = rayOrigin - sphereCenter;
    float a = 1.0;
    float b = 2.0 * dot(offset, rayDir);
    float c = dot(offset, offset) - sphereradius * sphereradius;
    float discriminant = b * b - 4.0 * a * c;

    if (discriminant > 0.0) {
        float s = sqrt(discriminant);
        float distanceToSphereNear = max(0.0, (-b - s) / (2.0 * a));
        float distanceToSphereFar = (-b + s) / (2.0 * a);
        if (distanceToSphereFar >= 0.0) {
            return vec2(distanceToSphereNear, distanceToSphereFar - distanceToSphereNear);
        }
    }
    float fMaxFloat = intBitsToFloat(2139095039);
    return vec2(fMaxFloat, 0.0);
}

void initColor(){

    float scatterR = pow(400 / wavelength.x, 4) * scatteringStrength;
    float scatterB = pow(400 / wavelength.z, 4) * scatteringStrength;
    float scatterG = pow(400 / wavelength.y, 4) * scatteringStrength;
    scatteringCoefficients = vec3(scatterR, scatterG, scatterB);
}


