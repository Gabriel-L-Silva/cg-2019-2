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
// OVERVIEW: GLGraphics.h
// ========
// Class definition for OpenGL graphics.
//
// Author: Paulo Pagliosa
// Last revision: 26/10/2018

#ifndef __GLGraphics_h
#define __GLGraphics_h

#include "graphics/GLGraphicsBase.h"
#include "graphics/GLMesh.h"

namespace cg
{ // begin namespace cg


/////////////////////////////////////////////////////////////////////
//
// GLGraphics: OpenGL graphics class
// ==========
class GLGraphics: public GLGraphicsBase
{
public:
  enum PolygonMode
  {
    LINE = GL_LINE,
    FILL = GL_FILL
  };

  // Default constructor.
  GLGraphics();

  auto polygonMode() const
  {
    return _polygonMode;
  }

  void setPolygonMode(PolygonMode mode)
  {
    glPolygonMode(GL_FRONT_AND_BACK, _polygonMode = mode);
  }

  auto meshColor() const
  {
    return _meshColor;
  }

  void setMeshColor(const Color& color)
  {
    _meshColor = color;
  }

  auto gridColor() const
  {
    return _gridColor;
  }

  void setGridColor(const Color& color)
  {
    _gridColor = color;
  }

  void setVectorColor(const Color& color)
  {
    setLineColor(_meshColor = color);
  }

  void drawMesh(TriangleMesh&, const mat4f&, const mat3f&);
  void drawMesh(TriangleMesh&, const vec3f&, const mat3f&, const vec3f&);

  void drawMesh(TriangleMesh& mesh)
  {
    drawMesh(mesh, mat4f::identity(), mat3f::identity());
  }

  void drawPoint(const vec3f&);
  void drawLine(const vec3f&, const vec3f&);
  void drawTriangle(const vec3f&, const vec3f&, const vec3f&);
  void drawCircle(const vec3f&, float, const vec3f&);
  void drawBounds(const Bounds3f&);
  void drawVector(const vec3f&, const vec3f&, float, TriangleMesh&);
  void drawNormals(TriangleMesh&, const mat4f&, const mat3f&);
  void drawAxes(const vec3f&, const mat3f&, float = 1.0f);
  void drawGround(float, float);

  void setView(const vec3f& position, const mat4f& vpMatrix)
  {
    _lightPosition = position;
    _vpMatrix = vpMatrix;
  }

  static TriangleMesh* circle();
  static TriangleMesh* cone();

private:
  using Base = GLGraphicsBase;

  GLSL::Program _meshDrawer;
  mat4f _vpMatrix;
  vec3f _lightPosition;
  GLint _flatMode;
  GLint _transformLoc;
  GLint _normalMatrixLoc;
  GLint _vpMatrixLoc;
  GLint _lightPositionLoc;
  GLint _colorLoc;
  GLint _flatModeLoc;
  Color _meshColor;
  Color _gridColor;
  PolygonMode _polygonMode;

  void drawPolyline(const vec3f*, int, const mat4f&, bool = false);

}; // GLGraphics

} // end namespace cg

#endif // __GLGraphics
