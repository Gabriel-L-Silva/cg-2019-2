//[]---------------------------------------------------------------[]
//|                                                                 |
//| Copyright (C) 2018, 2019 Orthrus Group.                         |
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
// OVERVIEW: MeshSweeper.h
// ========
// Source file for mesh sweeper.
//
// Author: Paulo Pagliosa
// Last revision: 18/11/2019

#include "geometry/MeshSweeper.h"
#include <vector>

namespace cg
{ // begin namespace cg

namespace internal
{ // begin namespace internal

inline void
setBoxVertices(vec3f* v)
{
  const vec3f p1{-1, -1, -1};
  const vec3f p2{+1, -1, -1};
  const vec3f p3{+1, +1, -1};
  const vec3f p4{-1, +1, -1};
  const vec3f p5{-1, -1, +1};
  const vec3f p6{+1, -1, +1};
  const vec3f p7{+1, +1, +1};
  const vec3f p8{-1, +1, +1};

  v[ 0] = p1; v[ 1] = p5; v[ 2] = p8; v[ 3] = p4; // x = -1
  v[ 4] = p2; v[ 5] = p3; v[ 6] = p7; v[ 7] = p6; // x = +1
  v[ 8] = p1; v[ 9] = p2; v[10] = p6; v[11] = p5; // y = -1
  v[12] = p4; v[13] = p8; v[14] = p7; v[15] = p3; // y = +1
  v[16] = p1; v[17] = p4; v[18] = p3; v[19] = p2; // z = -1
  v[20] = p5; v[21] = p6; v[22] = p7; v[23] = p8; // z = +1
}

inline void
setBoxVertexNormals(vec3f* n)
{
  const vec3f n1{-1, 0, 0};
  const vec3f n2{+1, 0, 0};
  const vec3f n3{0, -1, 0};
  const vec3f n4{0, +1, 0};
  const vec3f n5{0, 0, -1};
  const vec3f n6{0, 0, +1};

  n[ 0] = n[ 1] = n[ 2] = n[ 3] = n1; // x = -1
  n[ 4] = n[ 5] = n[ 6] = n[ 7] = n2; // x = +1
  n[ 8] = n[ 9] = n[10] = n[11] = n3; // y = -1
  n[12] = n[13] = n[14] = n[15] = n4; // y = +1
  n[16] = n[17] = n[18] = n[19] = n5; // z = -1
  n[20] = n[21] = n[22] = n[23] = n6; // z = +1
}

inline void
setBoxTriangles(TriangleMesh::Triangle* t)
{
  t[ 0].setVertices( 0,  1,  2); t[ 1].setVertices( 2,  3,  0);
  t[ 2].setVertices( 4,  5,  7); t[ 3].setVertices( 5,  6,  7);
  t[ 4].setVertices( 8,  9, 11); t[ 5].setVertices( 9, 10, 11);
  t[ 6].setVertices(12, 13, 14); t[ 7].setVertices(14, 15, 12);
  t[ 8].setVertices(16, 17, 19); t[ 9].setVertices(17, 18, 19);
  t[10].setVertices(20, 21, 22); t[11].setVertices(22, 23, 20);
}

} // end namespace internal


/////////////////////////////////////////////////////////////////////
//
// MeshSweeper implementation
// ===========
TriangleMesh*
MeshSweeper::makeBox()
{
  const int nv{24}; // number of vertices and normals
  const int nt{12}; // number of triangles
  TriangleMesh::Data data;

  data.numberOfVertices = nv;
  data.vertices = new vec3f[nv];
  internal::setBoxVertices(data.vertices);
  data.vertexNormals = new vec3f[nv];
  internal::setBoxVertexNormals(data.vertexNormals);
  data.numberOfTriangles = nt;
  data.triangles = new TriangleMesh::Triangle[nt];
  internal::setBoxTriangles(data.triangles);
  return new TriangleMesh{std::move(data)};
}

TriangleMesh*
MeshSweeper::makeCone(int ns)
{
  const int nt{ns * 2}; // number of triangles
  const int nv{nt + 2}; // number of vertices and normals
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
    data.vertices[j] = {0, 0, 0};
    data.vertexNormals[i] = h;
    data.vertexNormals[j] = N;
  }

  auto triangle = data.triangles;

  for (int t = ns + 1, i = 0; i < ns; i++)
  {
    int j{(i + 1) % ns};

    (triangle + ns)->setVertices(nv - 1, j + t, i + t);
    (triangle++)->setVertices(ns, i, j);
  }
  return new TriangleMesh{std::move(data)};
}

TriangleMesh*
MeshSweeper::makeSphere(int ns)
{
  // Number of meridians: ns.
  // Number of parallels: ns + 1 (including the poles).
  // Number of layers: number of parallels - 1.
  // Number of vertices per parallel: ns + 1. The first and the last
  // vertices have the same position and normal vector, but distinct
  // texture coordinates (note that there are ns + 1 north and south
  // poles, each one with its own texture coordinates).
  if (ns < 4)
    ns = 4;
  else if (ns % 2 == 1)
    ++ns;

  const auto np = ns + 1; // number of vertices per parallel
  const auto nl = ns; // number of layers
  const auto nv = np * (nl + 1); // number of vertices
  const auto nt = ns * (nl - 1) * 2; // number of triangles
  TriangleMesh::Data data;

  data.numberOfVertices = nv;
  data.vertices = new vec3f[nv];
  data.vertexNormals = new vec3f[nv];
  data.uv = new vec2f[nv];
  data.numberOfTriangles = nt;
  data.triangles = new TriangleMesh::Triangle[nt];

  {
    constexpr auto pi = math::pi<float>();
    const auto mStep = pi / ns * 2;
    const auto pStep = pi / nl;
    auto pAngle = pi / 2;
    auto vertex = data.vertices;
    auto normal = data.vertexNormals;
    auto uv = data.uv;

    for (int p = 0; p <= nl; ++p, pAngle -= pStep)
    {
      auto t = cos(pAngle);
      auto y = sin(pAngle);
      auto v = 1 - (float)p / nl;
      float mAngle{0};

      for (int m = 0; m <= ns; ++m, mAngle += mStep)
      {
        *vertex++ = *normal++ = {t * cos(mAngle), y, t * sin(mAngle)};
        *uv++ = {1 - (float)m / ns, v};
      }
    }
  }

  auto triangle = data.triangles;

  for (int p = 0; p < nl; ++p)
  {
    auto i = p * np;
    auto j = i + np;

    for (int m = 0; m < ns; ++m, ++i, ++j)
    {
      auto k = i + 1;

      if (p != 0)
        triangle++->setVertices(i, j, k);
      if (p != nl - 1)
        triangle++->setVertices(k, j, j + 1);
    }
  }
  return new TriangleMesh{std::move(data)};
}

} // end namespace cg
