#version 330 core

uniform mat4 transform;
uniform mat3 normalMatrix;
uniform mat4 vpMatrix = mat4(1);
uniform vec4 color;
uniform vec4 ambientLight = vec4(0.2, 0.2, 0.2, 1);
uniform vec3 lightPosition;
uniform vec4 lightColor = vec4(1);
uniform int flatMode;

layout(location = 0) in vec4 position;
layout(location = 1) in vec3 normal;

out vec4 vertexColor;

void main()
{
  vec4 P = transform * position;
  vec3 L = normalize(lightPosition - vec3(P));
  vec3 N = normalize(normalMatrix * normal);
  vec4 A = ambientLight * float(1 - flatMode);

  gl_Position = vpMatrix * P;
  vertexColor = A + color * lightColor * max(dot(N, L), float(flatMode));
}
