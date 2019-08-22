#version 330 core
uniform mat4 transform = mat4(1);
uniform vec4 v[] = vec4[3](
  vec4(-0.5f, -0.5f, 0, 1),
  vec4(+0.5f, -0.5f, 0, 1),
  vec4(0, +0.5f, 0, 1));
uniform vec4 color;
out vec4 vertexColor;
void main()
{
  gl_Position = transform * v[gl_VertexID];
  vertexColor = color;
}
