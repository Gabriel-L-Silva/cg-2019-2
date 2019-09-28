//[]---------------------------------------------------------------[]
//|                                                                 |
//| Copyright (C) 2018 Orthrus Group.                               |
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
// OVERVIEW: Primitive.h
// ========
// Class definition for primitive.
//
// Author(s): Paulo Pagliosa (and your name)
// Last revision: 25/08/2018

#ifndef __Primitive_h
#define __Primitive_h

#include "Component.h"
#include "graphics/GLMesh.h"

namespace cg
{ // begin namespace cg


/////////////////////////////////////////////////////////////////////
//
// Primitive: primitive class
// =========
class Primitive: public Component
{
public:
  Color color{Color::white};

  Primitive(TriangleMesh* mesh, const std::string& meshName):
    Component{"Primitive"},
    _mesh{mesh},
    _meshName(meshName)
  {
    // do nothing
  }

  TriangleMesh* mesh() const
  {
    return _mesh;
  }

  const char* const meshName() const
  {
    return _meshName.c_str();
  }

  void setMesh(TriangleMesh* mesh, const std::string& meshName)
  {
    _mesh = mesh;
    _meshName = meshName;
  }

private:
  Reference<TriangleMesh> _mesh;
  std::string _meshName;

}; // Primitive

} // end namespace cg

#endif // __Primitive_h
