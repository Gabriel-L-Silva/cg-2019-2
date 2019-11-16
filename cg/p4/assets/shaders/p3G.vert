#version 330 core

// eu adicionei
struct matProps
{
	vec4 ambient; // ambient color (Oa da fórmula)
	vec4 diffuse; // diffuse color
	vec4 spot; // specular spot color
	float shine; // specular spot exponent
};

struct lightProps
{
	int type;
	int fallof; // spot  and point
	int decayExponent; // spot
	float openningAngle; // spot
	vec3 lightPosition;
	vec4 lightColor;
	vec3 Ldirection;
};

// padrão pra todos os pontos

//object
uniform matProps material;
uniform mat4 transform;
uniform mat3 normalMatrix;
uniform mat4 vpMatrix = mat4(1);

//light
uniform int numLights;
uniform lightProps lights[10];
uniform vec4 ambientLight = vec4(0.2, 0.2, 0.2, 1); 
uniform int flatMode;


uniform vec3 camPos;
// pra cada um
layout(location = 0) in vec4 position;
layout(location = 1) in vec3 normal;

out vec4 vertexColor;

// elementwise multiplication
vec4 elementWise(vec4 firstVec, vec4 secVec)
{
	vec4 result;
	for (int i = 0;i < 4; i++)
	{
		result[i] = firstVec[i] * secVec[i];
	}		

	return result;
}


void main()
{

	vec4 P = transform * position;
	vec3 L; //está invertido já
	vec3 N = normalize(normalMatrix * normal);
	vec4 A = ambientLight * float(1 - flatMode); //Ia
	vec4 OAIA;
	vec4 ODIL;  
	vec4 OSIL;
	vec4 IL;
	vec3 R;
	vec3 V = normalize(vec3(P) - camPos);
	float temp; 
	vec4 firstTemp;
	vec4 secTemp;

	// obtenção do cálculo de OAIA

	OAIA = elementWise(material.ambient, A);

	vertexColor = OAIA;

	for (int i = 0; i < numLights; i++)
	{
		switch(lights[i].type)
		{
			case (0): //directional
				L = lights[i].Ldirection;
				IL = lights[i].lightColor;
				break;

			case (1): //point 
				L = normalize(lights[i].lightPosition - vec3(P)); 
				temp = distance(lights[i].lightPosition, vec3(P));
				IL = lights[i].lightColor/(pow(temp, lights[i].fallof));
				break;

			case (2): //spot
				L = normalize(lights[i].lightPosition - vec3(P));
				temp = distance(lights[i].lightPosition, vec3(P));
				float angle = acos(dot(lights[i].Ldirection, L)); 
				IL = (angle < radians(lights[i].openningAngle)) ? lights[i].lightColor/(pow(temp, lights[i].fallof)) * pow(cos(angle), lights[i].decayExponent) : vec4(0);
				break;
		}

		R = normalize(reflect(L, N));

		ODIL = elementWise(material.diffuse, IL);
		OSIL = elementWise(material.spot, IL);

		firstTemp = ODIL * max(dot(N, L), float(flatMode));
		secTemp = OSIL * pow(min(max(dot(R, V), 0), 1 - float(flatMode)), material.shine);
		//firstTemp = elementWise(ODIL, max(dot(N, L), float(flatMode)); 
		//firstTemp = elementWise(OSIL, max(dot(N, L), float(flatMode) - 1);
		vertexColor += firstTemp + secTemp;

	}

	gl_Position = vpMatrix * P;
	

}
