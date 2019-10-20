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
// OVERVIEW: Renderer.h
// ========
// Class definition for generic renderer.
//
// Author(s): Paulo Pagliosa (and your name)
// Last revision: 21/09/2019

#ifndef __Renderer_h
#define __Renderer_h

#include "Camera.h"
#include "Scene.h"

namespace cg
{ // begin namespace cg


//////////////////////////////////////////////////////////
//
// Renderer: generic renderer class
// ========
class Renderer: public virtual SharedObject
{
public:
  // Constructors
  Renderer() = default;

  Renderer(Scene&, Camera* = nullptr);

  auto scene() const
  {
    return _scene;
  }

  auto camera() const
  {
    return _camera;
  }

  void imageSize(int& w, int &h) const
  {
    w = _W;
    h = _H;
  }

  void setScene(Scene&);
  void setCamera(Camera*);
  void setImageSize(int, int);

  vec3f project(const vec3f&) const;
  vec3f unproject(const vec3f&) const;

  virtual void update();
  virtual void render() = 0;

protected:
  Reference<Scene> _scene;
  Reference<Camera> _camera;
  int _W{};
  int _H{};

}; // Renderer

} // end namespace cg

#endif // __Renderer_h
