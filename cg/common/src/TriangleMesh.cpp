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
// OVERVIEW: TriangleMesh.cpp
// ========
// Source file for simple triangle mesh.
//
// Author: Paulo Pagliosa
// Last revision: 15/09/2018

#include "geometry/TriangleMesh.h"
#include <memory>

namespace cg
{ // begin namespace cg


//////////////////////////////////////////////////////////
//
// TriangleMesh implementation
// ============
static uint32_t nextMeshId;

TriangleMesh::TriangleMesh(const Data& data):
  id{++nextMeshId},
  _data{data}
{
  // do nothing
}

TriangleMesh::~TriangleMesh()
{
  delete []_data.vertices;
  delete []_data.vertexNormals;
  delete []_data.triangles;
}

Bounds3f
TriangleMesh::bounds() const
{
  Bounds3f bounds;

  for (int i = 0; i < _data.numberOfVertices; i++)
    bounds.inflate(_data.vertices[i]);
  return bounds;
}

void
TriangleMesh::computeNormals()
{
  auto nv = _data.numberOfVertices;

  if (_data.vertexNormals == nullptr)
    _data.vertexNormals = new vec3f[nv];

  auto t = _data.triangles;

  memset(_data.vertexNormals, 0, nv * sizeof(vec3f));
  for (int i = 0; i < _data.numberOfTriangles; ++i, ++t)
  {
    auto v0 = t->v[0];
    auto v1 = t->v[1];
    auto v2 = t->v[2];
    auto normal = triangle::normal(_data.vertices, v0, v1, v2);

    _data.vertexNormals[v0] += normal;
    _data.vertexNormals[v1] += normal;
    _data.vertexNormals[v2] += normal;
  }
  for (int i = 0; i < nv; ++i)
    _data.vertexNormals[i].normalize();
}

void
TriangleMesh::TRS(const mat4f& trs)
{
  auto nv = _data.numberOfVertices;

  for (int i = 0; i < nv; ++i)
    _data.vertices[i] = trs.transform3x4(_data.vertices[i]);
  if (_data.vertexNormals == nullptr)
    return;

  auto r = normalTRS(trs);

  for (int i = 0; i < nv; ++i)
    _data.vertexNormals[i] = (r * _data.vertexNormals[i]).versor();
}

} // end namespace cg
