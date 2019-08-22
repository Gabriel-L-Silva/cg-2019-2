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
// OVERVIEW: Transform.h
// ========
// Class definition for scene object transform.
//
// Author(s): Paulo Pagliosa (and your name)
// Last revision: 25/08/2018

#ifndef __Transform_h
#define __Transform_h

#include "Component.h"
#include "math/Matrix4x4.h"

namespace cg
{ // begin namespace cg


/////////////////////////////////////////////////////////////////////
//
// Transform: scene object transform class
// =========
class Transform final: public Component
{
public:
  enum class Space
  {
    Local,
    World
  };

  /// Constructs an identity transform.
  Transform();

  /// Returns the parent of this transform.
  Transform* parent() const; // implemented in SceneObject.h

  /// Returns the local position of this transform.
  const vec3f& localPosition() const
  {
    return _localPosition;
  }

  /// Returns the local Euler angles (in degrees) of this transform.
  vec3f localEulerAngles() const
  {
    return _localEulerAngles;
  }

  /// Returns the local rotation of this transform.
  const quatf& localRotation() const
  {
    return quatf::eulerAngles(_localEulerAngles);
  }

  /// Returns the local scale of this transform.
  const vec3f& localScale() const
  {
    return _localScale;
  }

  /// Sets the local position of this transform.
  void setLocalPosition(const vec3f& position)
  {
    _localPosition = position;
    update();
  }

  /// Sets the local Euler angles (in degrees) of this transform.
  void setLocalEulerAngles(const vec3f& angles)
  {
    _localEulerAngles = angles;
    update();
  }

  /// Sets the local rotation of this transform.
  void setLocalRotation(const quatf& rotation)
  {
    setLocalEulerAngles(rotation.eulerAngles());
  }

  /// Sets the local scale of this transform.
  void setLocalScale(const vec3f& scale)
  {
    _localScale = scale;
    update();
  }

  /// Sets the local uniform scale of this transform.
  void setLocalScale(float scale)
  {
    setLocalScale(vec3f{scale});
  }

  /// Returns the world position of this transform.
  const vec3f& position() const
  {
    return _position;
  }

  /// Returns the world Euler angles (in degrees) of this transform.
  vec3f eulerAngles() const
  {
    return _eulerAngles;
  }

  /// Returns the world rotation of this transform.
  const quatf& rotation() const
  {
    return quatf::eulerAngles(_eulerAngles);
  }

  /// Sets the world position of this transform.
  void setPosition(const vec3f& position);

  /// Sets the world Euler angles (in degrees) of this transform.
  void setEulerAngles(const vec3f& angles);

  /// Sets the world rotation of this transform.
  void setRotation(const quatf& rotation)
  {
    setEulerAngles(rotation.eulerAngles());
  }

  /// Translates this transform relative to the \c space axes.
  void translate(const vec3f&, Space = Space::Local);

  /// Rotates this transform around the \c space axes.
  void rotate(const vec3f& angles, Space space = Space::Local)
  {
    rotate(quatf::eulerAngles(angles), space);
  }

  /// Rotates this transform around an \c axis.
  void rotate(const vec3f& axis, float angle, Space space = Space::Local)
  {
    rotate(quatf{angle, axis}, space);
  }

  /// Returns the local to world _matrix of this transform.
  const mat4f& localToWorldMatrix() const
  {
    return _matrix;
  }

  /// Returns the world to local _matrix of this transform.
  const mat4f& worldToLocalMatrix() const
  {
    return _inverseMatrix;
  }

  /// Transforms \c p from local space to world space.
  vec3f transform(const vec3f& p) const
  {
    return _matrix.transform3x4(p);
  }

  /// Transforms \c p from world space to local space.
  vec3f inverseTransform(const vec3f& p) const
  {
    return _inverseMatrix.transform3x4(p);
  }

  /// Transforms \c v from local space to world space.
  vec3f transformVector(const vec3f& v) const
  {
    return _matrix.transformVector(v);
  }

  /// Transforms \c v from world space to local space.
  vec3f inverseTransformVector(const vec3f& v) const
  {
    return _inverseMatrix.transformVector(v);
  }

private:
  vec3f _localPosition;
  vec3f _localEulerAngles;
  vec3f _localScale;
  vec3f _position;
  vec3f _eulerAngles;
  mat4f _matrix;
  mat4f _inverseMatrix;

  mat4f localMatrix() const;

  void rotate(const quatf&, Space = Space::Local);
  void update();

  friend class SceneObject;

}; // Transform

} // end namespace cg

#endif // __Transform_h
