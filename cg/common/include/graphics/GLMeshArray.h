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
// OVERVIEW: GLMeshArray.h
// ========
// Class definition for GL mesh array.
//
// Author: Paulo Pagliosa
// Last revision: 21/08/2018

#ifndef __GLMeshArray_h
#define __GLMeshArray_h

#include "graphics/Color.h"
#include "graphics/GLProgram.h"
#include "math/Matrix4x4.h"

namespace cg
{ // begin namespace cg


//////////////////////////////////////////////////////////
//
// GLMeshArray: GL mesh array class
// ===========
class GLMeshArray
{
public:
  template <typename Data>
  struct VertexBuffer
  {
    const Data* data;
    const int location;
  };

  struct Triangle
  {
    int v[3];
  };

  GLMeshArray(int nv,
    const VertexBuffer<vec4f>& v,
    const VertexBuffer<Color>& c,
    int nt,
    const Triangle* t)
  {
    glGenVertexArrays(1, &_vao);
    glBindVertexArray(_vao);
    glGenBuffers(3, _buffers);
    glBindBuffer(GL_ARRAY_BUFFER, _buffers[0]);
    glBufferData(GL_ARRAY_BUFFER, size(v.data, nv), v.data, GL_STATIC_DRAW);
    glVertexAttribPointer(v.location, 4, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(v.location);
    glBindBuffer(GL_ARRAY_BUFFER, _buffers[1]);
    glBufferData(GL_ARRAY_BUFFER, size(c.data, nv), c.data, GL_STATIC_DRAW);
    glVertexAttribPointer(c.location, 4, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(_colorLocation = c.location);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _buffers[2]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, size(t, nt), t, GL_STATIC_DRAW);
    _vertexCount = nt * 3;
  }

  ~GLMeshArray()
  {
    glDeleteBuffers(3, _buffers);
    glDeleteVertexArrays(1, &_vao);
  }

  void bind()
  {
    glBindVertexArray(_vao);
  }

  void useVertexColors()
  {
    glBindBuffer(GL_ARRAY_BUFFER, _buffers[1]);
    glEnableVertexAttribArray(_colorLocation);
  }
  
  void setVertexColor(const Color& color)
  {
    glBindBuffer(GL_ARRAY_BUFFER, _buffers[1]);
    glDisableVertexAttribArray(_colorLocation);
    glVertexAttrib4fv(1, (float*)&color);
  }

  auto vertexCount() const
  {
    return _vertexCount;
  }

private:
  GLuint _vao;
  GLuint _buffers[3];
  GLuint _colorLocation;
  int _vertexCount;

  template <typename T>
  static auto size(const T* p, int n)
  {
    return sizeof(T) * n;
  }

}; // GLMeshArray

} // end namespace cg

#endif // __GLMeshArray_h
