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
// OVERVIEW: RayTracer.h
// ========
// Class definition for simple ray tracer.
//
// Author(s): Paulo Pagliosa (and your name)
// Last revision: 16/10/2019

#ifndef __RayTracer_h
#define __RayTracer_h

#include "graphics/Image.h"
#include "Intersection.h"
#include "Renderer.h"

namespace cg
{ // begin namespace cg

#define MIN_WEIGHT float(0.001)
#define MAX_RECURSION_LEVEL uint32_t(20)


/////////////////////////////////////////////////////////////////////
//
// RayTracer: simple ray tracer class
// =========
class RayTracer: public Renderer
{
public:
  // Constructor
  RayTracer(Scene&, Camera* = 0);

  auto maxRecursionLevel() const
  {
    return _maxRecursionLevel;
  }

  auto minWeight() const
  {
    return _minWeight;
  }

  void setMaxRecursionLevel(uint32_t rl)
  {
    _maxRecursionLevel = std::min(rl, MAX_RECURSION_LEVEL);
  }

  void setMinWeight(float w)
  {
    _minWeight = std::max(w, MIN_WEIGHT);
  }

  void render();
  virtual void renderImage(Image&);

private:
  struct VRC
  {
    vec3f u;
    vec3f v;
    vec3f n;
  };

  uint32_t _maxRecursionLevel;
  float _minWeight;
  uint64_t _numberOfRays;
  uint64_t _numberOfHits;
  Ray _pixelRay;
  VRC _vrc;
  float _Vh;
  float _Vw;
  float _Ih;
  float _Iw;

  void scan(Image& image);
  void setPixelRay(float x, float y);
  Color shoot(float x, float y);
  bool intersect(const Ray&, Intersection&);
  Color trace(const Ray& ray, uint32_t level, float weight);
  Color shade(const Ray&, Intersection&, int, float);
  bool shadow(const Ray&);
  Color background() const;

  vec3f imageToWindow(float x, float y) const
  {
    return _Vw * (x * _Iw - 0.5f) * _vrc.u + _Vh * (y * _Ih - 0.5f) * _vrc.v;
  }

}; // RayTracer

} // end namespace cg

#endif // __RayTracer_h
