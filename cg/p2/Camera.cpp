//[]---------------------------------------------------------------[]
//|                                                                 |
//| Copyright (C) 2019 Orthrus Group.                               |
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
// OVERVIEW: Camera.cpp
// ========
// Source file for camera.
//
// Author(s): Paulo Pagliosa (and your name)
// Last revision: 21/09/2019

#include "Camera.h"

namespace cg
{ // begin namespace cg


/////////////////////////////////////////////////////////////////////
//
// Camera implementation
// ======
Camera* Camera::_current;

Camera::Camera(float aspect):
  Component{"Camera"},
  _viewAngle{60},
  _height{10},
  _aspectRatio{aspect},
  _F{0.01f},
  _B{1000.0f},
  _projectionType{Camera::Perspective}
{
  updateProjection();
}

Camera::~Camera()
{
  if (this == _current)
    _current = nullptr;
}

void
Camera::setViewAngle(float value)
{
  if (!math::isEqual(_viewAngle, value))
  {
    _viewAngle = std::min(std::max(value, minAngle), maxAngle);
    if (_projectionType == Perspective)
      updateProjection();
  }
}

void
Camera::setHeight(float value)
{
  if (!math::isEqual(_height, value))
  {
    _height = std::max(value, minHeight);
    if (_projectionType == Parallel)
      updateProjection();
  }
}

void
Camera::setAspectRatio(float value)
{
  if (!math::isEqual(_aspectRatio, value))
  {
    _aspectRatio = std::max(value, minAspect);
    updateProjection();
  }
}

void
Camera::setClippingPlanes(float F, float B)
{
  if (F > B)
    std::swap(F, B);
  if (F < minFrontPlane)
    F = minFrontPlane;
  if ((B - F) < minDepth)
    B = F + minDepth;
  if (!math::isEqual(_F, F) || !math::isEqual(_B, B))
  {
    _F = F;
    _B = B;
    updateProjection();
  }
}

void
Camera::setProjectionType(ProjectionType value)
{
  if (_projectionType != value)
  {
    _projectionType = value;
    updateProjection();
  }
}

inline mat4f
lookAt(const vec3f& p, const vec3f& u, const vec3f& v, const vec3f& n)
{
  mat4f m;

  m[0].set(u[0], v[0], n[0]);
  m[1].set(u[1], v[1], n[1]);
  m[2].set(u[2], v[2], n[2]);
  m[3].set(-u.dot(p), -v.dot(p), -n.dot(p), 1.0f);
  return m;
}

void
Camera::updateView() const
{
  auto t = const_cast<Camera*>(this)->transform();

  if (!t->changed)
    return;

  const auto& p = t->position();
  auto r = mat3f{t->rotation()};

  _worldToCameraMatrix = lookAt(p, r[0], r[1], r[2]);
  _cameraToWorldMatrix.set(r, p);
  t->changed = false;
}

void
Camera::reset(float aspect)
{
  _aspectRatio = aspect;
  _viewAngle = 60;
  _height = 10;
  _F = 0.01f;
  _B = 1000.0f;
  _projectionType = Perspective;
  updateProjection();
}

void
Camera::updateProjection()
{
  if (_projectionType == Parallel)
  {
    auto t = _height * 0.5f;
    auto r = t * _aspectRatio;

    _projectionMatrix = mat4f::ortho(-r, r, -t, t, _F, _B);
  }
  else
    _projectionMatrix = mat4f::perspective(_viewAngle, _aspectRatio, _F, _B);
}

void
Camera::setCurrent(Camera* camera)
{
  if (camera != _current)
  {
    if (camera != nullptr && camera->sceneObject() == nullptr)
      return; // TODO: throw an exception
    _current = camera;
  }
}

} // end namespace cg
