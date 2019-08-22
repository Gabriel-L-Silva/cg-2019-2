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
// OVERVIEW: GLMesh.h
// ========
// Class definition for GL mesh array object.
//
// Author: Paulo Pagliosa
// Last revision: 15/09/2018

#ifndef __GLMesh_h
#define __GLMesh_h

#include "geometry/TriangleMesh.h"
#include "graphics/GLProgram.h"

namespace cg
{ // begin namespace cg


//////////////////////////////////////////////////////////
//
// GLMesh GL mesh array object class
// ======
class GLMesh: public SharedObject
{
public:
  GLMesh(const TriangleMesh& mesh)
  {
    glGenVertexArrays(1, &_vao);
    glBindVertexArray(_vao);
    glGenBuffers(3, _buffers);

    const auto& m = mesh.data();

    if (auto s = size<vec3f>(m.numberOfVertices))
    {
      glBindBuffer(GL_ARRAY_BUFFER, _buffers[0]);
      glBufferData(GL_ARRAY_BUFFER, s, m.vertices, GL_STATIC_DRAW);
      glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
      glEnableVertexAttribArray(0);
      glBindBuffer(GL_ARRAY_BUFFER, _buffers[1]);
      glBufferData(GL_ARRAY_BUFFER, s, m.vertexNormals, GL_STATIC_DRAW);
      glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);
      glEnableVertexAttribArray(1);
    }
    if (auto s = size<TriangleMesh::Triangle>(m.numberOfTriangles))
    {
      glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _buffers[2]);
      glBufferData(GL_ELEMENT_ARRAY_BUFFER, s, m.triangles, GL_STATIC_DRAW);
    }
    _vertexCount = m.numberOfTriangles * 3;
  }

  ~GLMesh()
  {
    glDeleteBuffers(3, _buffers);
    glDeleteVertexArrays(1, &_vao);
  }

  void bind()
  {
    glBindVertexArray(_vao);
  }

  auto vertexCount() const
  {
    return _vertexCount;
  }

private:
  GLuint _vao;
  GLuint _buffers[3];
  int _vertexCount;

  template <typename T>
  static auto size(int n)
  {
    return sizeof(T) * n;
  }

}; // GLMesh

inline GLMesh*
asGLMesh(SharedObject* object)
{
  return dynamic_cast<GLMesh*>(object);
}

inline GLMesh*
glMesh(TriangleMesh* mesh)
{
  if (nullptr == mesh)
    return nullptr;

  auto ma = asGLMesh(mesh->userData);

  if (nullptr == ma)
  {
    ma = new GLMesh{*mesh};
    mesh->userData = ma;
  }
  return ma;
}

} // end namespace cg

#endif // __GLMesh_h
