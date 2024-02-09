#version 450
out vec4 FragColor;
in vec2 UV;
uniform sampler2D _ColorBuffer;
uniform float blurScale;
vec3 boxBlur[3] = {
	vec3(1,1,1),
	vec3(1,1,1),
	vec3(1,1,1)
};

vec3 sharpen[3] = {
	vec3(0,-1,0),
	vec3(-1,5,-1),
	vec3(0,-1,0)
};

vec3 gaussianBlur[3] = {
	vec3(1,2,1),
	vec3(2,4,2),
	vec3(1,2,1)
};

vec3 edgeDetect[3] = {
	vec3(-1,-1,-1),
	vec3(-1,8,-1),
	vec3(-1,-1,-1)
};

uniform int kernalSet;

void main(){
    if(kernalSet == 0)
    {
        vec2 texelSize = blurScale / textureSize(_ColorBuffer,0).xy;
        vec3 totalColor = vec3(0);
        for(int y = 0; y <= 2; y++){
            for(int x = -1; x <= 1; x++){
                vec2 offset = vec2(x,y) * texelSize;
                totalColor += texture(_ColorBuffer,UV + offset).rgb * boxBlur[x][y];
            }
        }
        totalColor/=(9);
        FragColor = vec4(totalColor,1.0);
    }
    else if(kernalSet == 1)
    {
        vec2 texelSize = blurScale / textureSize(_ColorBuffer,0).xy;
        vec3 totalColor = vec3(0);
        for(int y = 0; y <= 2; y++){
            for(int x = -1; x <= 1; x++){
                vec2 offset = vec2(x,y) * texelSize;
                totalColor += texture(_ColorBuffer,UV + offset).rgb * sharpen[x][y];
            }
        }
        totalColor/=(1);
        FragColor = vec4(totalColor,1.0);
    }
    else if(kernalSet == 2)
    {
        vec2 texelSize = blurScale / textureSize(_ColorBuffer,0).xy;
        vec3 totalColor = vec3(0);
        for(int y = 0; y <= 2; y++){
            for(int x = -1; x <= 1; x++){
                vec2 offset = vec2(x,y) * texelSize;
                totalColor += texture(_ColorBuffer,UV + offset).rgb * gaussianBlur[x][y];
            }
        }
        totalColor/=(16);
        FragColor = vec4(totalColor,1.0);
    }
    else if(kernalSet == 3)
    {
        vec2 texelSize = blurScale / textureSize(_ColorBuffer,0).xy;
        vec3 totalColor = vec3(0);
        for(int y = 0; y <= 2; y++){
            for(int x = -1; x <= 1; x++){
                vec2 offset = vec2(x,y) * texelSize;
                totalColor += texture(_ColorBuffer,UV + offset).rgb * edgeDetect[x][y];
            }
        }
        totalColor/=(1);
        FragColor = vec4(totalColor,1.0);
    }
}
