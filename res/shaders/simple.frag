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
in layout(location = 13) vec2 v_noise;
in layout(location = 14) vec3 vetorEye;
//in layout(location = 16) float depth;

uniform Lights lights[1];
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
uniform layout(location = 17) vec3 atmosphereCenter;
uniform layout(location = 18) vec2 atmosphereMinMax;

//layout(binding = 4) uniform sampler2D depthTexture;
layout(binding = 3) uniform sampler2D elevationcolor;
layout(binding = 2) uniform sampler2D roughness_texture;
layout(binding = 1) uniform sampler2D normal_texture;
layout(binding = 0) uniform sampler2D diffusetexture;

out vec4 color;



float BiomePercentFromPoint(float heightPercent);
float getColorFromSun();
float diffuse(vec3 n, vec3 l);
float specular(vec3 n, vec3 l, vec3 campos );
float attenuation(vec3 lightL );
vec3 reject(vec3 from, vec3 onto);
float map(float value, float min1, float max1, float min2, float max2); 
float inverseLerp(float a, float b, float value);
float rand(vec2 co) { return fract(sin(dot(co.xy, vec2(12.9898,78.233))) * 43758.5453); }
float dither(vec2 uv) { return (rand(uv)*2.0-1.0) / 256.0; }
void initColor();

// sun
float diffuse_reflekton = 0.3;
float ambient = 0.1;
float shininessVal = 11;
vec3 lightSpecular;
float la = 0.000009;
float lb = 0.000009;
float lc = 0.000009;
float radius = 3;
float softradius = 4;


// asmophere
vec2 raySphere(vec3 sphereCenter, float sphereradius, vec3 rayOrigin, vec3 rayDir );

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
//    return (2.0 * near * far) / (far + near - (depth * 2.0 - 1.0) * (far - near));
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


    float calculateLight(vec3 rayOrigin, vec3 rayDir, float distanceThroughAmoshpere){
    vec3 dirToSun = lights[0].position;
    vec3 inScatterPoint = rayOrigin;
    float stepSize = distanceThroughAmoshpere / (numInScatteringPoints -1);
    float inScatteredLight = 0;
       
    for(int i = 0; i < numInScatteringPoints; i++){
        float sunRayLength = raySphere(atmosphereCenter, atmosphericRadius, inScatterPoint, dirToSun).y;
        float sunRayOpticalDepth = opticalDepth(inScatterPoint, dirToSun, sunRayLength);
        float viewRayOpticalDepth = opticalDepth(inScatterPoint, -rayDir, stepSize * i);
        float transmittance = (exp(-sunRayOpticalDepth)*exp(-viewRayOpticalDepth))/4; // add divied by 4
        float localDensity = densityAtPoint(inScatterPoint);
        inScatteredLight += localDensity * transmittance * stepSize;
        inScatterPoint += rayDir * stepSize; 
    }
    return inScatteredLight;
}
    
    vec4 asmophere(vec4 orignalcolor,  float sphereradius, float depth){
        float ocenradius = minmax.x;
        // depth of scene, less brigth if its closer to planet.
        vec3 rayorigin = eye;
        vec3 raydiraction = normalize(-vetorEye);
        float sceneDepth = LinearEyeDepth(depth) * length(vetorEye);
        float distancetoOcean = raySphere(atmosphereCenter, ocenradius, rayorigin, raydiraction).x;   
        float distancetosurface = min(sceneDepth, distancetoOcean);

        vec2 hitinfo = raySphere(atmosphereCenter, sphereradius, rayorigin, raydiraction );
        float distanceToAmoshpere = hitinfo.x;
        float distanceThroughAmoshpere = min(hitinfo.y, distancetosurface - distanceToAmoshpere);
        return (distanceThroughAmoshpere / (atmosphericRadius * 2) * vec4(raydiraction.xyz * 0.5 + 0.5, 1));
        if(distanceThroughAmoshpere > 0){
            const float epsilon = 0.0001;
            vec3 pointInAtmosphere = rayorigin + raydiraction + (distanceToAmoshpere+ epsilon);
            float light = calculateLight(pointInAtmosphere, raydiraction, distanceThroughAmoshpere- epsilon * 2);
            return min(orignalcolor * (1 - light) + light, 0.5); // added min 0.5
        }
        return orignalcolor;
    }
    
//vec3 calculateLight(vec3 rayOrigin, vec3 rayDir, float distanceThroughAmoshpere, vec3 orignialcolor){
//    vec3 dirToSun = lights[0].position;
//    vec3 inScatterPoint = rayOrigin;
//    float stepSize = distanceThroughAmoshpere / (numInScatteringPoints -1);
//    vec3 inScatteredLight = vec3(0);
//    float viewRayOpticalDepth = 0;
//       
//    for(int i = 0; i < numInScatteringPoints; i++){
//        float sunRayLength = raySphere(center, atmosphericRadius, inScatterPoint, dirToSun).y;
//        float sunRayOpticalDepth = opticalDepth(inScatterPoint, dirToSun, sunRayLength);
//        viewRayOpticalDepth = opticalDepth(inScatterPoint, -rayDir, stepSize * i);
//        vec3 transmittance = exp(-(sunRayOpticalDepth+viewRayOpticalDepth) * scatteringCoefficients);  
//        float localDensity = densityAtPoint(inScatterPoint);
//        inScatteredLight += localDensity * transmittance * scatteringCoefficients * stepSize;
//        inScatterPoint += rayDir * stepSize; 
//    }
//    float orignialcoltransmittance = exp(-viewRayOpticalDepth);
//    return orignialcolor * orignialcoltransmittance + inScatteredLight;
//}
//
//
//
//vec4 asmophere(vec4 orignalcolor,  float sphereradius, float depth){
//    float ocenradius = minmax.x;
//    
//    vec3 rayorigin = eye;
//    vec3 raydiraction = normalize(-vetorEye);
//    float sceneDepth = depth * length(vetorEye);
//    float distancetoOcean = raySphere(center, ocenradius, rayorigin, raydiraction).x;   
//    float distancetosurface = min(sceneDepth, distancetoOcean);
//
//    vec2 hitinfo = raySphere(center, sphereradius, rayorigin, raydiraction );
//    float distanceToAmoshpere = hitinfo.x;
//    float distanceThroughAmoshpere = min(hitinfo.y, distancetosurface - distanceToAmoshpere);
//
//    if(distanceThroughAmoshpere > 0){
//        const float epsilon = 0.0001;
//        vec3 pointInAtmosphere = rayorigin + raydiraction + (distanceToAmoshpere+ epsilon);
//        vec3 light = calculateLight(pointInAtmosphere, raydiraction, distanceThroughAmoshpere- epsilon * 2, orignalcolor.xyz);
//
//        return vec4(light, 1); 
//    }
//    return orignalcolor;
//    }
    



void main()
{

     
     if(normal_geo == 3){ // "sun"
        color = vec4(1);
        return;
     }
     initColor();

    float distanceFromCenter = distance(vpos, center);
    float elevation = inverseLerp(minmax.x, minmax.y, distanceFromCenter);
    float normalizedY = (vpos.y - (center.y - distanceFromCenter)) / (2.0 * distanceFromCenter);
    normalizedY += v_noise.x;
    vec4 pixelcolor = texture(elevationcolor, vec2(elevation, BiomePercentFromPoint(normalizedY)));


    float diffuseTotal = getColorFromSun();
    float dilter = dither(textureCoordinates);

    vec4 atmosphericEffect;
    vec4 updatedcolor;

    if (minmax.y < distanceFromCenter){
        vec4 diftexture = texture(diffusetexture, textureCoordinates);
        updatedcolor = vec4((diftexture.xyz*diffuseTotal)+dilter, diftexture.w);
        atmosphericEffect = asmophere(updatedcolor, atmosphericRadius, gl_FragCoord.z);
    } else{
        updatedcolor = vec4(((pixelcolor.xyz)*diffuseTotal)+dilter, pixelcolor.w) ;
        atmosphericEffect = asmophere(updatedcolor, atmosphericRadius, gl_FragCoord.z);
    }

//    if(atmosphericEffect.x > 0)
//            updatedcolor.xyz += atmosphericEffect.xyz;
        
      color = updatedcolor;
//      color = vec4(vetorEye, 1);
//color = vec4(vetorEye, 1);
    }




    
float getColorFromSun(){
    

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
    return diffuseTotal;
}


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
    vec3 scatteringCoefficients = vec3(scatterR, scatterG, scatterB);
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
