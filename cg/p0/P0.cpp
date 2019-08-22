#include "P0.h"

void
P0::initialize()
{
  cg::Application::loadShaders(_program, "p0.vs", "p0.fs");
  glGenVertexArrays(1, &_vao);
  glBindVertexArray(_vao);
  _program.use();
}

void
P0::gui()
{
  static auto position = cg::vec3f::null();
  static auto rotation = cg::vec3f::null();
  static auto scale = cg::vec3f{1.0f};

  ImGui::Begin("Welcome to Computer Graphics 2019!");
  ImGui::Text("GLSL program '%s' in use.", _program.name());
  ImGui::Text("Authors: ");
  ImGui::SameLine();
  ImGui::TextColored({0, 1, 0, 1}, "Gabriel Lucas & Lucas Santana");
  ImGui::Separator();
  ImGui::ColorEdit3("Background Color", (float*)&backgroundColor);
  ImGui::ColorEdit3("Triangle Color", (float*)&_triangleColor);
  ImGui::Separator();
  ImGui::DragFloat3("Rotation", (float*)&rotation);
  ImGui::Separator();
  ImGui::DragFloat2("Position", (float*)&position);
  ImGui::Separator();
  ImGui::DragFloat2("Scale", (float*)&scale);
  // This is the transformation to be applied to the triangle.
  // TODO: add GUI elements to set position and scale.
  _transform.setTRS(position, rotation, scale);
  ImGui::Separator();
  ImGui::Text("Application average %.3f ms/frame (%.1f FPS)",
    deltaTime(),
    ImGui::GetIO().Framerate);
  ImGui::End();
}

void
P0::render()
{
  Base::render();
  _program.setUniformVec4("color", _triangleColor);
  _program.setUniformMat4("transform", _transform);
  glDrawArrays(GL_TRIANGLES, 0, 3);
}
