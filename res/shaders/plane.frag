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

// sun
float diffuse_reflekton = 0.3;
float ambient = 0.1;
float shininessVal = 11;
vec3 lightSpecular;
float la = 0.00009;
float lb = 0.00009;
float lc = 0.00009;
float radius = 3;
float softradius = 4;

// asmophere
float atmosphericRadius = ((atmosphereMinMax.y+atmosphereMinMax.x)/2) * 1.3;
float planetRadius = (atmosphereMinMax.y+atmosphereMinMax.x)/2; // max + 10?
float densityFalloff = 3;
float numInScatteringPoints = 10;
float numOpticalDepthPoints = 10;
vec3 wavelength = vec3(700, 530, 440);
float scatterR;
float scatterB;
float scatterG;
float scatteringStrength = 3;
vec3 scatteringCoefficients;

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


    
vec3 calculateLight(vec3 rayOrigin, vec3 rayDir, float distanceThroughAmoshpere, vec3 orignialcolor){
    vec3 dirToSun = lights[0].position;
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
    float orignialcoltransmittance = exp(-viewRayOpticalDepth);
    return orignialcolor * orignialcoltransmittance + inScatteredLight;
}



vec4 asmophere(vec4 orignalcolor,  float sphereradius, float depth, vec3 viewDirection){
    float ocenradius = minmax.x;
    
    vec3 rayorigin = eye;
    vec3 raydiraction = normalize(-viewDirection);
    float sceneDepth = depth * length(raydiraction);
    float distancetoOcean = raySphere(atmosphereCenter, ocenradius, rayorigin, raydiraction).x;   
    float distancetosurface = min(sceneDepth, distancetoOcean);

    vec2 hitinfo = raySphere(atmosphereCenter, sphereradius, rayorigin, raydiraction );
    float distanceToAmoshpere = hitinfo.x;
    float distanceThroughAmoshpere = min(hitinfo.y, distancetosurface - distanceToAmoshpere);

    if(distanceThroughAmoshpere > 0){
        const float epsilon = 0.0001;
        vec3 pointInAtmosphere = rayorigin + raydiraction + (distanceToAmoshpere+ epsilon);
        vec3 light = calculateLight(pointInAtmosphere, raydiraction, distanceThroughAmoshpere- epsilon * 2, orignalcolor.xyz);

        return vec4(light, 1); 
    }
    return orignalcolor;
    }
    



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

    vec4 atmosphericEffect = asmophere(updatedcolor, atmosphericRadius, depth, eyeSpacePos.xyz);

    if(atmosphericEffect.x > 0)
            updatedcolor.xyz += atmosphericEffect.xyz;
     color = updatedcolor;

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



//    float calculateLight(vec3 rayOrigin, vec3 rayDir, float distanceThroughAmoshpere){
//    vec3 dirToSun = lights[0].position;
//    vec3 inScatterPoint = rayOrigin;
//    float stepSize = distanceThroughAmoshpere / (numInScatteringPoints -1);
//    float inScatteredLight = 0;
//       
//    for(int i = 0; i < numInScatteringPoints; i++){
//        float sunRayLength = raySphere(atmosphereCenter, atmosphericRadius, inScatterPoint, dirToSun).y;
//        float sunRayOpticalDepth = opticalDepth(inScatterPoint, dirToSun, sunRayLength);
//        float viewRayOpticalDepth = opticalDepth(inScatterPoint, -rayDir, stepSize * i);
//        float transmittance = (exp(-sunRayOpticalDepth)*exp(-viewRayOpticalDepth))/4; // add divied by 4
//        float localDensity = densityAtPoint(inScatterPoint);
//        inScatteredLight += localDensity * transmittance * stepSize;
//        inScatterPoint += rayDir * stepSize; 
//    }
//    return inScatteredLight;
//}
//    
//    vec4 asmophere(vec4 orignalcolor,  float sphereradius, float depth, vec3 viewDirection){
//        float ocenradius = minmax.x;
//        // depth of scene, less brigth if its closer to planet.
//        vec3 rayorigin = eye;
//        vec3 raydiraction = normalize(viewDirection);
//        float sceneDepth = LinearEyeDepth(depth) * length(viewDirection * 5);
//        float distancetoOcean = raySphere(atmosphereCenter, ocenradius, rayorigin, raydiraction).x;   
//        float distancetosurface = min(sceneDepth, distancetoOcean);
//
//        vec2 hitinfo = raySphere(atmosphereCenter, sphereradius, rayorigin, raydiraction );
//        float distanceToAmoshpere = hitinfo.x;
//        float distanceThroughAmoshpere = min(hitinfo.y, sceneDepth - distanceToAmoshpere);
//        if(distanceThroughAmoshpere > 0){
//            const float epsilon = 0.0001;
//            vec3 pointInAtmosphere = rayorigin + raydiraction + (distanceToAmoshpere+ epsilon);
//            float light = calculateLight(pointInAtmosphere, raydiraction, distanceThroughAmoshpere- epsilon * 2);
//            return min(orignalcolor * (1 - light) + light, 0.5); // added min 0.5
//        }
//        return orignalcolor;
//    }