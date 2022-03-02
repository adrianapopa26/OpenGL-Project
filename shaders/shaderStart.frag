#version 410 core

in vec3 fNormal;
in vec4 fPosEye;
in vec2 fTexCoords;
in vec4 fPosEyeLightSpace;
uniform sampler2D shadowMap;
out vec4 fColor;

//lighting
uniform	vec3 lightDir;
uniform	vec3 lightColor;

//texture
uniform sampler2D diffuseTexture;
uniform sampler2D specularTexture;

vec3 ambient;
float ambientStrength = 0.5f;
vec3 diffuse;
vec3 specular;
float specularStrength = 0.5f;
float shininess = 32.0f;

// fog
uniform int initFog;
uniform float initFogDensity;

// spot light
uniform int initSpotLight;
uniform float spotLight;
uniform float spotLight1;

uniform vec3 spotLightDirection;
uniform vec3 spotLightPosition;

vec3 spotLightColor = vec3(15,0,0);

float computeShadow()
{
	// perform perspective divide
	vec3 normalizedCoords = fPosEyeLightSpace.xyz / fPosEyeLightSpace.w;
	if (normalizedCoords.z > 1.0f)
		return 0.0f;

	// Transform to [0,1] range
	normalizedCoords = normalizedCoords * 0.5 + 0.5;
	
	// Get closest depth value from light's perspective
	float closestDepth = texture(shadowMap, normalizedCoords.xy).r;
	
	// Get depth of current fragment from light's perspective
	float currentDepth = normalizedCoords.z;
	
	// Check whether current frag pos is in shadow
	float bias = 0.005f;
	float shadow = currentDepth -bias > closestDepth ? 1.0f:0.0f;

	return shadow;
}

vec3 computeLightComponents()
{		
	vec3 cameraPosEye = vec3(0.0f);//in eye coordinates, the viewer is situated at the origin
	
	//transform normal
	vec3 normalEye = normalize(fNormal);	
	
	//compute light direction
	vec3 lightDirN = normalize(lightDir);
	
	//compute view direction 
	vec3 viewDirN = normalize(cameraPosEye - fPosEye.xyz);
		
	//compute ambient light
	ambient = ambientStrength * lightColor;
	
	//compute diffuse light
	diffuse = max(dot(normalEye, lightDirN), 0.0f) * lightColor;
	
	//compute specular light
	vec3 reflection = reflect(-lightDirN, normalEye);
	float specCoeff = pow(max(dot(viewDirN, reflection), 0.0f), shininess);
	specular = specularStrength * specCoeff * lightColor;
	
	return (ambient + diffuse + specular);
}

float computeFog()
{
	float fragmentDistance = length(fPosEye);
	float fogFactor = exp(-pow(fragmentDistance * initFogDensity, 2));
	return clamp(fogFactor, 0.0f, 1.0f);
}

vec3 computeLightSpotComponents() {
	vec3 cameraPosEye = vec3(0.0f);//in eye coordinates, the viewer is situated at the origin
	
	//transform normal
	vec3 lightDir = normalize(spotLightPosition-fPosEye.xyz);
	vec3 normalEye = normalize(fNormal);
	
	//compute light direction
	vec3 lightDirN = normalize(lightDir);
	
	//compute view direction 
	vec3 viewDirN = normalize(cameraPosEye - fPosEye.xyz);
	vec3 halfVector = normalize(lightDirN + viewDirN);

	float spotLightAttenuation = 1.0f / (1.0f + 0.09f * length(spotLightPosition - fPosEye.xyz) + 0.02f * length(spotLightPosition - fPosEye.xyz) * length(spotLightPosition - fPosEye.xyz));

	float spotLightIntensity = clamp((dot(lightDir, normalize(-spotLightDirection)) - spotLight1)/(spotLight - spotLight1), 0.0, 5.0);
	
	//compute ambient light
	vec3 ambient = spotLightColor * vec3(0.2f, 0.2f, 0.2f) * vec3(texture(diffuseTexture, fTexCoords));
	
	//compute difuse light
	vec3 diffuse = spotLightColor * vec3(50.0f, 50.0f, 50.0f) * max(dot(normalEye, lightDir), 0.0f) * vec3(texture(diffuseTexture, fTexCoords));
	
	//compute specular
	vec3 specular = spotLightColor * vec3(50.0f, 50.0f, 50.0f) * pow(max(dot(normalEye, halfVector), 0.0f), shininess) * vec3(texture(specularTexture, fTexCoords));
	
	ambient *= spotLightAttenuation * spotLightIntensity;
	diffuse *= spotLightAttenuation * spotLightIntensity;
	specular *= spotLightAttenuation * spotLightIntensity;
	
	return ambient + diffuse + specular;
}

void main() 
{
	vec3 light = computeLightComponents();
	vec3 baseColor = vec3(0.9f, 0.35f, 0.0f);//orange
	
	ambient *= texture(diffuseTexture, fTexCoords).rgb;
	diffuse *= texture(diffuseTexture, fTexCoords).rgb;
	specular *= texture(specularTexture, fTexCoords).rgb;
	
	// spotlight
	if (initSpotLight == 1){
		light += computeLightSpotComponents();
	}
	
	float shadow = computeShadow();
	vec3 color = min((ambient + (1.0f - shadow)*diffuse) + (1.0f - shadow)*specular, 1.0f);
	
    float fogFactor = computeFog();
	vec4 fogColor = vec4(0.5f,0.5f,0.5f,1.0f);
	vec4 drawShadow = vec4(color,1.0f);
	if (initFog == 0) {
		fColor = min(drawShadow * vec4(light, 1.0f), 1.0f);
	}
	else {
		fColor = (fogColor * (1 - fogFactor)) + (drawShadow * fogFactor);
	}
}
