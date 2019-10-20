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
// OVERVIEW: Renderer.cpp
// ========
// Source file for generic renderer.
//
// Author(s): Paulo Pagliosa (and your name)
// Last revision: 21/09/2019

#include "Renderer.h"

namespace cg
{ // begin namespace cg


//////////////////////////////////////////////////////////
//
// Renderer implementation
// ========
Renderer::Renderer(Scene& scene, Camera* camera):
  _scene{&scene},
  _camera{camera != nullptr ? camera : new Camera}
{
  // do nothing
}

void
Renderer::setScene(Scene& scene)
{
  if (&scene != _scene)
    _scene = &scene;
}

void
Renderer::setCamera(Camera* camera)
{
  if (camera != _camera)
    _camera = camera != nullptr ? camera : new Camera;
}

void
Renderer::setImageSize(int w, int h)
{
  _camera->setAspectRatio((float)(_W = w) / (float)(_H = h));
}

void
Renderer::update()
{
  // do nothing
}

inline vec3f
normalize(const vec4f& p)
{
  return vec3f{p} * math::inverse(p.w);
}

vec3f
Renderer::project(const vec3f& p) const
{
  auto w = normalize(vpMatrix(_camera) * vec4f{p, 1});

  w.x = (w.x * 0.5f + 0.5f) * _W;
  w.y = (w.y * 0.5f + 0.5f) * _H;
  w.z = (w.z * 0.5f + 0.5f);
  return w;
}

vec3f
Renderer::unproject(const vec3f& w) const
{
  vec4f p{w.x / _W * 2 - 1, w.y / _H * 2 - 1, w.z * 2 - 1, 1};
  mat4f m{vpMatrix(_camera)};

  m.invert();
  return normalize(m * p);
}

} // end namespace cg
