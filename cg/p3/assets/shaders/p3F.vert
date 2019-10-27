#version 330 core

// padrão pra todos os pontos
uniform mat4 transform;
uniform mat3 normalMatrix;
uniform mat4 vpMatrix = mat4(1);
uniform vec4 color;
uniform vec4 ambientLight = vec4(0.2, 0.2, 0.2, 1);
uniform vec3 lightPosition;
uniform vec4 lightColor = vec4(1);
uniform int flatMode;

// pra cada um
layout(location = 0) in vec4 position;
layout(location = 1) in vec3 normal;

out vec4 P;
out vec3 N;

void main()
{
  P = transform * position;
  N = normalize(normalMatrix * normal);

  gl_Position = vpMatrix * P;

}
