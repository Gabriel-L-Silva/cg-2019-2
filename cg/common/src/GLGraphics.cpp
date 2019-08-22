//[]---------------------------------------------------------------[]
//|                                                                 |
//| Copyright (C) 2014, 2018 Orthrus Group.                         |
//|                                                                 |
//| This software is provided 'as-is', without any express or       |
//| implied warranty. In no event will the authors be held liable   |
//| for any damages arising from the use of this software.          |
//|                                                                 |
//| Permission is granted to anyone to use this software for any    |
//| purpose, including commercial applications, and to alter it and |
//| redistribute it freely, subject to the following restrictions:  |
//|                                                                 |
//| 1. The origin of this software must not be misrepresented; you  |
//| must not claim that you wrote the original software. If you use |
//| this software in a product, an acknowledgment in the product    |
//| documentation would be appreciated but is not required.         |
//|                                                                 |
//| 2. Altered source versions must be plainly marked as such, and  |
//| must not be misrepresented as being the original software.      |
//|                                                                 |
//| 3. This notice may not be removed or altered from any source    |
//| distribution.                                                   |
//|                                                                 |
//[]---------------------------------------------------------------[]
//
// OVERVIEW: GLGraphics.cpp
// ========
// Source file for OpenGL graphics.
//
// Author: Paulo Pagliosa
// Last revision: 26/10/2018

#include "graphics/GLGraphics.h"

namespace cg
{ // begin namespace cg

#define STRINGIFY(A) "#version 400\n"#A

static const char* vertexShader = STRINGIFY(
  uniform mat4 transform;
  uniform mat3 normalMatrix;
  uniform mat4 vpMatrix;
  uniform vec3 lightPosition;
  uniform vec4 lightColor = vec4(1, 1, 1, 1);
  uniform vec4 color;
  uniform int flatMode;

  layout(location = 0) in vec4 position;
  layout(location = 1) in vec3 normal;
  out vec4 vertexColor;

  void main()
  {
    vec4 P = transform * position;
    vec3 L = normalize(lightPosition - vec3(P));
    vec3 N = normalize(normalMatrix * normal);

    gl_Position = vpMatrix * P;
    vertexColor = color * lightColor * max(dot(N, L), float(flatMode));
  }
);

static const char* fragmentShader = STRINGIFY(
  in vec4 vertexColor;
  out vec4 fragmentColor;

  void main()
  {
    fragmentColor = vertexColor;
  }
);

static TriangleMesh*
makeCircle(const int np = 20)
{
  const int nt = np;
  const int nv = nt + 1;
  TriangleMesh::Data data;

  data.numberOfVertices = nv;
  data.vertices = new vec3f[nv];
  data.vertexNormals = new vec3f[nv];
  data.numberOfTriangles = nt;
  data.triangles = new TriangleMesh::Triangle[nt];
  data.vertices[0].set(0, 0, 0);
  data.vertexNormals[0].set(0, 0, 1);
  if (true)
  {
    const auto a = float(2 * M_PI) / np;
    const auto c = cos(a);
    const auto s = sin(a);
    auto x = 0.0f;
    auto y = 1.0f;

    for (int i = 1; i < nv; i++)
    {
      data.vertices[i].set(x, y, 0);
      data.vertexNormals[i].set(0, 0, 1);

      const auto tx = x;
      const auto ty = y;

      x = c * tx - s * ty;
      y = s * tx + c * ty;
    }
  }

  auto t = data.triangles;

  for (int i = 1; i < nv; i++, t++)
    t->setVertices(0, i, i % nt + 1);
  return new TriangleMesh(data);
}

inline TriangleMesh*
GLGraphics::circle()
{
  static Reference<TriangleMesh> _circle;

  if (_circle == nullptr)
    _circle = makeCircle();
  return _circle;
}

inline TriangleMesh*
makeCone(const int ns = 16)
{
  const int nt = ns * 2;
  const int nv = nt + 2;
  TriangleMesh::Data data;

  data.numberOfVertices = nv;
  data.vertices = new vec3f[nv];
  data.vertexNormals = new vec3f[nv];
  data.numberOfTriangles = nt;
  data.triangles = new TriangleMesh::Triangle[nt];
  if (true)
  {
    const auto a = float(2 * M_PI) / ns;
    const auto c = cos(a);
    const auto s = sin(a);
    auto x = 1.0f;
    auto z = 0.0f;
    const auto h = vec3f::up();
    const auto N = -h;
    int i{0};
    int j{ns + 1};

    for (; i < ns; i++, j++)
    {
      const auto p = vec3f{x, 0, z};

      data.vertices[i] = data.vertices[j] = p;
      data.vertexNormals[i] = p;
      data.vertexNormals[j] = N;

      const auto tx = x;
      const auto tz = z;

      x = c * tx - s * tz;
      z = s * tx + c * tz;
    }
    data.vertices[i] = h;
    data.vertices[j].set(0, 0, 0);
    data.vertexNormals[i] = h;
    data.vertexNormals[j] = N;

  }

  auto triangle = data.triangles;

  for (int t = ns + 1, i = 0; i < ns; i++, triangle++)
  {
    int j{(i + 1) % ns};

    triangle->setVertices(ns, i, j);
    triangle[ns].setVertices(nv - 1, j + t, i + t);
  }
  return new TriangleMesh(data);
}

inline TriangleMesh*
GLGraphics::cone()
{
  static Reference<TriangleMesh> _cone;

  if (_cone == nullptr)
    _cone = makeCone();
  return _cone;
}

GLGraphics::GLGraphics():
  _meshDrawer{"Mesh Drawer"},
  _vpMatrix{mat4f::identity()},
  _lightPosition{0, 0, 10},
  _flatMode(0)
{
  _meshDrawer.setShaders(vertexShader, fragmentShader).use();
  _transformLoc = _meshDrawer.uniformLocation("transform");
  _normalMatrixLoc = _meshDrawer.uniformLocation("normalMatrix");
  _vpMatrixLoc = _meshDrawer.uniformLocation("vpMatrix");
  _lightPositionLoc = _meshDrawer.uniformLocation("lightPosition");
  _colorLoc = _meshDrawer.uniformLocation("color");
  _flatModeLoc = _meshDrawer.uniformLocation("flatMode");
  _meshColor = _gridColor = Color{0.5f, 0.5f, 0.5f};
}

void
GLGraphics::drawMesh(TriangleMesh& mesh, const mat4f& t, const mat3f& n)
{
  auto cp = GLSL::Program::current();

  _meshDrawer.use();
  _meshDrawer.setUniformMat4(_transformLoc, t);
  _meshDrawer.setUniformMat3(_normalMatrixLoc, n);
  _meshDrawer.setUniformMat4(_vpMatrixLoc, _vpMatrix);
  _meshDrawer.setUniformVec3(_lightPositionLoc, _lightPosition);
  _meshDrawer.setUniformVec4(_colorLoc, _meshColor);
  _meshDrawer.setUniform(_flatModeLoc, _flatMode);

  auto m = glMesh(&mesh);

  m->bind();
  glDrawElements(GL_TRIANGLES, m->vertexCount(), GL_UNSIGNED_INT, 0);
  GLSL::Program::setCurrent(cp);
}

inline auto
TRS(const vec3f& p, const mat3f& r, const vec3f& s)
{
  mat4f m{r, p};

  m[0] *= s.x;
  m[1] *= s.y;
  m[2] *= s.z;
  return m;
}

void
GLGraphics::drawMesh(TriangleMesh& mesh,
  const vec3f& p,
  const mat3f& r,
  const vec3f& s)
{
  auto n = r;

  n[0] *= math::inverse(s.x);
  n[1] *= math::inverse(s.y);
  n[2] *= math::inverse(s.z);
  drawMesh(mesh, TRS(p, r, s), n);
}

void
GLGraphics::drawPoint(const vec3f& p)
{
  vec4f point{_vpMatrix.transform(vec4f{p, 1})};
  Base::drawPoint(&point);
}

void
GLGraphics::drawLine(const vec3f& p1, const vec3f& p2)
{
  vec4f points[2];

  points[0] = _vpMatrix.transform(vec4f{p1, 1});
  points[1] = _vpMatrix.transform(vec4f{p2, 1});
  Base::drawLine(points);
}

void
GLGraphics::drawTriangle(const vec3f& p1, const vec3f& p2, const vec3f& p3)
{
  vec4f points[3];

  points[0] = _vpMatrix.transform(vec4f{p1, 1});
  points[1] = _vpMatrix.transform(vec4f{p2, 1});
  points[2] = _vpMatrix.transform(vec4f{p3, 1});
  Base::drawTriangle(points);
}

inline void
GLGraphics::drawPolyline(const vec3f* v, int n, const mat4f& m, bool close)
{
  auto f = m.transform3x4(v[0]);
  auto p = f;

  for (int i = 1; i < n; i++)
  {
    auto q = m.transform3x4(v[i]);

    drawLine(p, q);
    p = q;
  }
  if (close)
    drawLine(p, f);
}

void
GLGraphics::drawCircle(const vec3f& center, float radius, const vec3f& normal)
{
  auto n = normal.versor();
  auto u = vec3f::up().cross(n);

  u = (u.isNull() ? vec3f(1, 0, 0).cross(n) : u).versor();

  mat3f r;

  r[0].set(u);
  r[1].set(n.cross(u));
  r[2].set(n);
  if (_polygonMode == FILL)
    drawMesh(*circle(), center, r, vec3f{radius});
  else
  {
    auto m = TRS(center, r, vec3f{radius});
    const auto& c = circle()->data();

    drawPolyline(c.vertices + 1, c.numberOfVertices - 1, m, true);
  }
}

void
GLGraphics::drawBounds(const Bounds3f& box)
{
  const auto& p1 = box.min();
  const auto& p7 = box.max();
  vec3f p2{p7.x, p1.y, p1.z};
  vec3f p3{p7.x, p7.y, p1.z};
  vec3f p4{p1.x, p7.y, p1.z};
  vec3f p5{p1.x, p1.y, p7.z};
  vec3f p6{p7.x, p1.y, p7.z};
  vec3f p8{p1.x, p7.y, p7.z};

  drawLine(p1, p2);
  drawLine(p2, p3);
  drawLine(p3, p4);
  drawLine(p1, p4);
  drawLine(p5, p6);
  drawLine(p6, p7);
  drawLine(p7, p8);
  drawLine(p5, p8);
  drawLine(p3, p7);
  drawLine(p2, p6);
  drawLine(p4, p8);
  drawLine(p1, p5);
}

void
GLGraphics::drawNormals(TriangleMesh& mesh, const mat4f& t, const mat3f& n)
{
  const auto& data = mesh.data();
  auto& glyph = *cone();

  if (data.vertexNormals == nullptr)
    return;
  _flatMode = 1;
  for (int i = 0; i < data.numberOfVertices; i++)
  {
    const auto p = t.transform3x4(data.vertices[i]);
    const auto N = n.transform(data.vertexNormals[i]).versor();

    drawVector(p, N, 0.5f, glyph);
  }
  _flatMode = 0;
}

void
GLGraphics::drawVector(const vec3f& p,
  const vec3f& d,
  float s,
  TriangleMesh& glyph)
{
  vec3f a;

  if (math::isZero(d.x) && math::isZero(d.z))
    a = d.y < 0 ? vec3f{0, 0, 1} : vec3f::up();
  else
    a.set(d.x, d.y + 1, d.z);

  const auto q = p + d * s;

  drawLine(p, q);
  drawMesh(glyph, q, mat3f{quatf{180, a}}, vec3f{0.1f, 0.4f, 0.1f});
}

void
GLGraphics::drawAxes(const vec3f& p, const mat3f& r, float s)
{
  auto dt = glIsEnabled(GL_DEPTH_TEST);
  auto pm = _polygonMode;
  auto& glyph = *cone();

  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  glDisable(GL_DEPTH_TEST);
  _flatMode = 1;
  setVectorColor(Color::red);
  drawVector(p, r[0], s, glyph);
  setVectorColor(Color::green);
  drawVector(p, r[1], s, glyph);
  setVectorColor(Color::blue);
  drawVector(p, r[2], s, glyph);
  _flatMode = 0;
  glPolygonMode(GL_FRONT_AND_BACK, pm);
  if (dt)
    glEnable(GL_DEPTH_TEST);
}

void
GLGraphics::drawGround(float size, float step)
{
  setLineColor(_gridColor);
  for (float s = step; s <= size; s += step)
  {
    drawLine(vec3f{-size, 0, +s}, vec3f{size, 0, +s});
    drawLine(vec3f{-size, 0, -s}, vec3f{size, 0, -s});
    drawLine(vec3f{+s, 0, -size}, vec3f{+s, 0, size});
    drawLine(vec3f{-s, 0, -size}, vec3f{-s, 0, size});
  }
  setLineColor(Color::red);
  drawLine(vec3f{-size, 0, 0}, vec3f{size, 0, 0});
  setLineColor(Color::blue);
  drawLine(vec3f{0, 0, -size}, vec3f{0, 0, size});
}

} // end namespace cg
