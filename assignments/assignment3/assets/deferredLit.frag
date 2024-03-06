#version 450 core
out vec4 FragColor; 
in vec2 UV; //From fsTriangle.vert

//All your material and lighting uniforms go here!
uniform vec3 _EyePos;
uniform vec3 _LightColor = vec3(1.0);
uniform vec3 _AmbientColor = vec3(0.3,0.4,0.46);
struct Material{
	float Ka; //Ambient coefficient (0-1)
	float Kd; //Diffuse coefficient (0-1)
	float Ks; //Specular coefficient (0-1)
	float Shininess; //Affects size of specular highlight
};
uniform Material _Material;

struct PointLight{
	vec3 position;
	float radius;
	vec3 color;
};
const int MAX_POINT_LIGHTS = 64;
PointLight pointLights[MAX_POINT_LIGHTS];


struct DirectionalLight{
	vec3 direction;
	vec3 color;
};
uniform vec3 mainDirection;
uniform vec3 mainColor;
DirectionalLight _MainLight;

//layout(binding = i) can be used as an alternative to shader.setInt()
//Each sampler will always be bound to a specific texture unit
uniform layout(binding = 0) sampler2D _gPositions;
uniform layout(binding = 1) sampler2D _gNormals;
uniform layout(binding = 2) sampler2D _gAlbedo;

vec3 calculateLighting(vec3 normal,vec3 worldPos,vec3 albedo);
float attenuateLinear(float distance, float radius);
float attenuateExponential(float distance, float radius);

vec3 calcPointLight(PointLight light,vec3 normal){
	vec3 worldPos = texture(_gPositions,UV).xyz;
	vec3 diff = light.position - worldPos;
	//Direction toward light position
	vec3 toLight = normalize(diff);

	float diffuseFactor = max(dot(normal,toLight),0.0);
	vec3 toEye = normalize(_EyePos - worldPos);
	vec3 h = normalize(toLight + toEye);
	float specularFactor = pow(max(dot(normal,h),0.0),_Material.Shininess);
	//TODO: Usual blinn-phong calculations for diffuse + specular
	vec3 lightColor = (diffuseFactor + specularFactor) * light.color;
	//Attenuation
	float d = length(diff); //Distance to light
	lightColor *= attenuateExponential(d,light.radius); //See below for attenuation options
	return lightColor;
}

vec3 calcDirectionalLight(DirectionalLight light, vec3 normal)
{
	vec3 worldPos = texture(_gPositions,UV).xyz;
	//Light pointing straight down
	vec3 toLight = -light.direction;
	float diffuseFactor = max(dot(normal,toLight),0.0);

	vec3 toEye = normalize(_EyePos - worldPos);
	vec3 h = normalize(toLight + toEye);
	float specularFactor = pow(max(dot(normal,h),0.0),_Material.Shininess);
	//Combination of specular and diffuse reflection
	vec3 lightColor = (_Material.Kd * diffuseFactor + _Material.Ks * specularFactor) * _LightColor;

	lightColor+=_AmbientColor * _Material.Ka;


	return lightColor;
}

void main()
{
	_MainLight.direction = mainDirection;
	_MainLight.color = mainColor;
	
	for(int i = 0; i <= 10; i++)
	{
		pointLights[i].position = vec3(-5 + i,3, -5 + i);
		pointLights[i].radius = 5f;
		pointLights[i].color = vec3(i+1/11,1,1);
	}
	
	vec3 normal = texture(_gNormals,UV).xyz;

	vec3 totalLight = vec3(0);
	totalLight += calcDirectionalLight(_MainLight, normal);
	//totalLight+=calcDirectionalLight(_MainLight,normal);
	for(int i=0;i<MAX_POINT_LIGHTS;i++){
		totalLight+=calcPointLight(pointLights[i],normal);
	}
	vec3 albedo = texture(_gAlbedo,UV).xyz;
	FragColor = vec4(albedo * totalLight,0);
}

/*
void main()
{
	for(int i = 0; i <= 10; i++)
	{
		pointLights[i].position = vec3(0,3, i);
		pointLights[i].radius = 5f;
		pointLights[i].color = vec4(i+1/11,1,1,1)
	}
	
	//Sample surface properties for this screen pixel
	vec3 normal = texture(_gNormals,UV).xyz;
	vec3 worldPos = texture(_gPositions,UV).xyz;
	vec3 albedo = texture(_gAlbedo,UV).xyz;

	//Worldspace lighting calculations, same as in forward shading
	vec3 lightColor = calculateLighting(normal,worldPos,albedo);
	FragColor = vec4(albedo * lightColor,1.0);
}


	//Make sure fragment normal is still length 1 after interpolation.
	normal = normalize(fs_in.WorldNormal);
	//Light pointing straight down
	vec3 toLight = -_LightDirection;
	float diffuseFactor = max(dot(normal,toLight),0.0);
	//Calculate specularly reflected light
	vec3 toEye = normalize(_EyePos - fs_in.WorldPos);
	//Blinn-phong uses half angle
	vec3 h = normalize(toLight + toEye);
	float specularFactor = pow(max(dot(normal,h),0.0),_Material.Shininess);
	//Combination of specular and diffuse reflection
	vec3 lightColor = (_Material.Kd * diffuseFactor + _Material.Ks * specularFactor) * _LightColor;
	float shadow = ShadowCalculation(fs_in.FragPosLightSpace);
	shadow /= shadScale;

	lightColor+=_AmbientColor * _Material.Ka * (1.0 - shadow);
	vec3 objectColor = texture(_MainTex,fs_in.TexCoord).rgb;
*/
vec3 calculateLighting(vec3 normal,vec3 worldPos,vec3 albedo)
{
	//Light pointing straight down
	vec3 toLight = worldPos - _EyePos;
	float diffuseFactor = max(dot(normal,toLight),0.0);
	//Calculate specularly reflected light
	vec3 toEye = normalize(_EyePos - worldPos);
	//Blinn-phong uses half angle
	vec3 h = normalize(toLight + toEye);
	float specularFactor = pow(max(dot(normal,h),0.0),_Material.Shininess);
	//Combination of specular and diffuse reflection
	vec3 lightColor = (_Material.Kd * diffuseFactor + _Material.Ks * specularFactor) * _LightColor;

	lightColor+=_AmbientColor * _Material.Ka;
	return lightColor;
}

//Linear falloff
float attenuateLinear(float distance, float radius){
	return clamp((radius-distance)/radius,0f,1f);
}
//Exponential falloff
float attenuateExponential(float distance, float radius)
{
	float i = clamp(1.0 - pow(distance/radius,4.0),0.0,1.0);
	return i * i;
	
}
