#version 430 core

layout (location = 2) in vec4 frag;
layout (location = 3) in vec3 normal;
out vec4 color;


void main()
{
    vec3 lightDirection = normalize(vec3(0.8, -0.5, 0.6));
    color =  vec4( frag.rgb * max(0, -dot( normalize(normal.xyz), lightDirection)), frag.a  );
}





    /*
    vec2 Frag = gl_FragCoord.xy;
    Frag.x/=800;
    //Frag.x += 0.5;
    Frag.y/=600;
    //Frag.y += 0.5;

    Frag.x = -1.0 + (Frag.x - 0) * (1 - -1) / (1 - 0);
    Frag.y = -1.0 + (Frag.y - 0) * (1 - -1) / (1 - 0);
    // low2 + (value - low1) * (high2 - low2) / (high1 - low1)
    float len = sqrt(Frag.x*Frag.x+Frag.y*Frag.y);
    
    if(len > 0.4 && len < 0.6)
        discard;  
    color = vec4(gl_FragCoord.x/800, gl_FragCoord.y/600, 0.0f, 1.0f);
   
    color = vec4(1.0f, 1.0f, 1.0f, 1.0f); 
    */