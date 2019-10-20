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
// OVERVIEW: Camera.h
// ========
// Class definition for camera.
//
// Author(s): Paulo Pagliosa (and your name)
// Last revision: 21/09/2019

#ifndef __Camera_h
#define __Camera_h

#include "Transform.h"

namespace cg
{ // begin namespace cg


/////////////////////////////////////////////////////////////////////
//
// Camera: camera class
// ======
class Camera final: public Component
{
public:
  static constexpr float minAngle = 1;
  static constexpr float maxAngle = 179;
  static constexpr float minHeight = 0.01f;
  static constexpr float minAspect = 0.1f;
  static constexpr float minFrontPlane = 0.01f;
  static constexpr float minDepth = 0.01f;

  enum ProjectionType
  {
    Perspective,
    Parallel
  };

  /// Constructs a default camera.
  Camera(float aspect = 1);

  ~Camera() override;

  float viewAngle() const;
  float height() const;
  float aspectRatio() const;
  float clippingPlanes(float& F, float& B) const;
  ProjectionType projectionType() const;

  void setViewAngle(float viewAngle);
  void setHeight(float height);
  void setAspectRatio(float aspect);
  void setClippingPlanes(float F, float B);
  void setProjectionType(ProjectionType projectionType);

  mat4f worldToCameraMatrix() const;
  mat4f cameraToWorldMatrix() const;
  mat4f projectionMatrix() const;

  void reset(float aspect = 1);

  static Camera* current()
  {
    return _current;
  }

  static void setCurrent(Camera* camera);

private:
  float _viewAngle;
  float _height;
  float _aspectRatio;
  float _F;
  float _B;
  ProjectionType _projectionType;
  mutable mat4f _worldToCameraMatrix{1.0f};
  mutable mat4f _cameraToWorldMatrix{1.0f};
  mat4f _projectionMatrix;

  static Camera* _current;

  void updateView() const;
  void updateProjection();

}; // Camera

inline float
Camera::viewAngle() const
{
  return _viewAngle;
}

inline float
Camera::height() const
{
  return _height;
}

inline float
Camera::aspectRatio() const
{
  return _aspectRatio;
}

inline float
Camera::clippingPlanes(float& F, float& B) const
{
  F = _F;
  B = _B;
  return B - F;
}

inline Camera::ProjectionType
Camera::projectionType() const
{
  return _projectionType;
}

inline mat4f
Camera::worldToCameraMatrix() const
{
  updateView();
  return _worldToCameraMatrix;
}

inline mat4f
Camera::cameraToWorldMatrix() const
{
  updateView();
  return _cameraToWorldMatrix;
}

inline mat4f
Camera::projectionMatrix() const
{
  return _projectionMatrix;
}

//
// Auxiliary function
//
inline auto
vpMatrix(const Camera* c)
{
  return c->projectionMatrix() * c->worldToCameraMatrix();
}

} // end namespace cg

#endif // __Camera_h
