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

float atmosphericplanetradius = minmax.y+10;
float planetradius = 100; // max + 10?
float densityFalloff = 100;
float numInScatteringPoints = 50;
float numOpticalDepthPoints = 50;
float localDenity = 10;
    
    float dilter = dither(textureCoordinates);
     vec4 atmosphericEffect;
    if (minmax.y < distanceFromCenter){
        vec4 diftexture = texture(diffusetexture, textureCoordinates);
        diffuseTotal += vec4(atmosphericEffect).xyz;
        atmosphericEffect = asmophere(atmosphericplanetradius, diftexture.w);
//        if(atmosphericEffect > 0)
            diftexture.xyz += atmosphericEffect.xyz;
        color = vec4((diftexture.xyz*diffuseTotal)+dilter, diftexture.w);
    } else{
        atmosphericEffect = asmophere(atmosphericplanetradius, pixelcolor.w);
//        pixelcolor.xyz *= 0;
        
//        if(atmosphericEffect > 0)
            pixelcolor.xyz += atmosphericEffect.xyz;
        color = vec4(((pixelcolor.xyz)*diffuseTotal)+dilter, pixelcolor.w) ;
        
    }
     color =  atmosphericEffect;

    }

float calculateLight(vec3 rayOrigin, vec3 rayDir, float rayLength){
    vec3 dirToSun = lights[0].position;
    vec3 inScatterPoint = rayOrigin;
    float stepSize = rayLength / (numInScatteringPoints -1);
    float inScatteredLight = 0;
    for(int i = 0; i < numInScatteringPoints; i++){
        float sunRayLength = raySphere(center, atmosphericplanetradius, inScatterPoint, dirToSun).y;
        float sunRayOpticalDepth = opticalDepth(inScatterPoint, dirToSun, sunRayLength);

        float viewRayOpticalDepth = opticalDepth(inScatterPoint, -rayDir, stepSize * i);

        float transmittance = exp(-(sunRayOpticalDepth+viewRayOpticalDepth));
        float localDensity = densityAtPoint(inScatterPoint);
        inScatteredLight += localDenity * transmittance * stepSize;
        inScatterPoint += rayDir * stepSize; 
    
    }
    return inScatteredLight;
}



vec4 asmophere(float sphereradius, float depth){
    float ocenradius = minmax.x;
    // depth of scene, less brigth if its closer to planet.
    float sceneDepth = LinearEyeDepth(depth) * length(vetorEye);
    float distancetoOcean = raySphere(center, ocenradius, eye, vetorEye).x;   
    float distancetosurface = min(sceneDepth, distancetoOcean);

    vec2 hitinfo = raySphere(center, sphereradius, eye, vetorEye );
    float distanceToAmoshpere = hitinfo.x;
    float distanceThroughAmoshpere = min(hitinfo.y, distancetosurface - distanceToAmoshpere);
    //float distanceThroughAmoshpere = min(hitinfo.y, sceneDepth - distanceToAmoshpere);
//    return  vec4(1)*(distanceThroughAmoshpere / (sphereradius*2));
    return  (distanceThroughAmoshpere / (sphereradius*2))* vec4(normalize(-vetorEye.xyz) * 0.5 +0.5,1);
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


vec3 calculateLight(vec3 rayOrigin, vec3 rayDir, float distanceThroughAmoshpere, vec3 orignialcolor,float distanceToSurface){
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
