#version 410

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in vec2 inUV;

uniform sampler2D s_Diffuse;
uniform sampler2D s_Diffuse2;

uniform vec3  u_AmbientCol;
uniform float u_AmbientStrength;

uniform vec3  u_LightPos;
uniform vec3  u_LightCol;
uniform float u_AmbientLightStrength;
uniform float u_SpecularLightStrength;
uniform float u_Shininess;
// NEW in week 7, see https://learnopengl.com/Lighting/Light-casters for a good reference on how this all works, or
// https://developer.valvesoftware.com/wiki/Constant-Linear-Quadratic_Falloff
uniform float u_LightAttenuationConstant;
uniform float u_LightAttenuationLinear;
uniform float u_LightAttenuationQuadratic;

uniform vec3  u_CamPos;

uniform int u_Condition;

//Toon-Shading Variables
const float lightIntensity = 10.0;
const int bands = 5;
const float scaleFactor = 1.0 / bands;

out vec4 frag_color;

// https://learnopengl.com/Advanced-Lighting/Advanced-Lighting
void main() {
	// Lecture 5
	vec3 ambient = u_AmbientLightStrength * u_LightCol;

	// Diffuse
	vec3 N = normalize(inNormal);
	vec3 lightDir = normalize(u_LightPos - inPos);

	float dif = max(dot(N, lightDir), 0.0);
	vec3 diffuse = dif * u_LightCol;// add diffuse intensity

	//Attenuation
	float dist = length(u_LightPos - inPos);
	float attenuation = 1.0f / (
		u_LightAttenuationConstant + 
		u_LightAttenuationLinear * dist +
		u_LightAttenuationQuadratic * dist * dist);

	// Specular
	vec3 viewDir  = normalize(u_CamPos - inPos);
	vec3 h        = normalize(lightDir + viewDir);

	// Get the specular power from the specular map
	float texSpec = 1.0f;
	float spec = pow(max(dot(N, h), 0.0), u_Shininess); // Shininess coefficient (can be a uniform)
	vec3 specular = u_SpecularLightStrength * texSpec * spec * u_LightCol; // Can also use a specular color

	// Get the albedo from the diffuse / albedo map
	vec4 textureColor = texture(s_Diffuse, inUV);

	//Toon-Shading
	vec3 diffuseOut = (dif * u_LightCol) / (dist * dist);
	diffuseOut = diffuseOut * lightIntensity;
	diffuseOut = floor(diffuseOut * bands) * scaleFactor;
	float edge = (dot(viewDir, inNormal) < 0.4) ? 0.0 : 1.0;

	vec3 result;

	switch (u_Condition)
	{
		//No lighting (flat shading)
		case 0: 
			result = inColor * textureColor.rgb;		
			break;

		//Ambient Lighting
		case 1:
			result = (
			(u_AmbientCol * u_AmbientStrength) + 
			(ambient) * attenuation
			) * inColor * textureColor.rgb;
			break; 
		//Diffuse Lighting
		case 2:
			result = (
			(diffuse) * attenuation 
			) * inColor * textureColor.rgb;
			break;
		//Specular Lighting
		case 3:
			result = (
			(specular) * attenuation
			) * inColor * textureColor.rgb;
			break;
		//Ambient + Specular + Diffuse
		case 4:
			result = (
			(u_AmbientCol * u_AmbientStrength) + 
			(ambient + diffuse + specular) * attenuation 
			) * inColor * textureColor.rgb; 
			break;	
		//Ambient + Specular + Diffuse + Toon-Shading
		case 5:
			result = (
			(u_AmbientCol * u_AmbientStrength) + 
			(ambient + (diffuseOut * edge) + specular) * attenuation 
			) * inColor * textureColor.rgb; 
			break;
	}

	frag_color = vec4(result, textureColor.a);
}