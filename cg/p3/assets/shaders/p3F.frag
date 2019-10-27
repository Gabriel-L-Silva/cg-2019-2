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
	float openingAngle; // spot
	vec3 lightPosition;
	vec4 lightColor;
};

// padrão pra todos os pontos
uniform vec3 camPos;
uniform matProps material;
uniform int numLights;
uniform lightProps lights[10];
uniform mat4 transform;
uniform mat3 normalMatrix;
uniform mat4 vpMatrix = mat4(1);
uniform vec4 ambientLight = vec4(0.2, 0.2, 0.2, 1); 
uniform int flatMode;

in vec4 P;
in vec3 N; 

out vec4 fragmentColor;

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
	vec3 Ldirection = vec3(0,-1,0);
	vec3 L; //está invertido já
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

	fragmentColor = OAIA;

	for (int i = 0; i < numLights; i++)
	{
		switch(lights[i].type)
		{
			case (0): //directional
				L = Ldirection;
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
				float angle = acos(dot(Ldirection, L)); 
				IL = (angle < radians(lights[i].openingAngle) ? lights[i].lightColor/(pow(temp, lights[i].fallof)) * pow(cos(angle), lights[i].decayExponent) : vec4(0);
				break;
		}

		R = normalize(reflect(L, N));

		ODIL = elementWise(material.diffuse, IL);
		OSIL = elementWise(material.spot, IL);

		firstTemp = ODIL * max(dot(N, L), float(flatMode));
		secTemp = OSIL * min(max(dot(R, V), 0), 1 - float(flatMode)); 
		//firstTemp = elementWise(ODIL, max(dot(N, L), float(flatMode)); 
		//firstTemp = elementWise(OSIL, max(dot(N, L), float(flatMode) - 1);
		fragmentColor += firstTemp + secTemp;

	}

}
