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
// OVERVIEW: Transform.cpp
// ========
// Source file for scene object transform.
//
// Author(s): Paulo Pagliosa (and your name)
// Last revision: 23/09/2019

#include "SceneObject.h"

namespace cg
{ // begin namespace cg

template <typename real>
inline Matrix4x4<real>
inverseTRS(const Matrix4x4<real>& trs)
{
  auto u = Vector3<real>{trs[0]};
  auto v = Vector3<real>{trs[1]};
  auto w = Vector3<real>{trs[2]};

  u *= math::inverse(u.squaredNorm());
  v *= math::inverse(v.squaredNorm());
  w *= math::inverse(w.squaredNorm());

  auto p = Vector3<real>{trs[3]};
  Matrix4x4<real> inv;

  inv[0].set(u.x, v.x, w.x);
  inv[1].set(u.y, v.y, w.y);
  inv[2].set(u.z, v.z, w.z);
  inv[3].set(-(u.dot(p)), -(v.dot(p)), -(w.dot(p)), real(1));
  return inv;
}

template <typename real>
inline Vector3<real>
translation(const Matrix4x4<real>& trs)
{
  return Vector3<real>{trs[3]};
}

template <typename real>
inline Vector3<real>
scale(const Quaternion<real>& q, const Matrix4x4<real>& m)
{
  using mat3 = Matrix3x3<real>;
  return (mat3{q.inverse()} * mat3{m}).diagonal();
}


/////////////////////////////////////////////////////////////////////
//
// Transform implementation
// =========
Transform::Transform():
  Component{"Transform"},
  _localPosition{0.0f},
  _localRotation{quatf::identity()},
  _localEulerAngles{0.0f},
  _localScale{1.0f},
  _matrix{1.0}
{
  _position = _localPosition;
  _rotation = _localRotation;
  _lossyScale = _localScale;
  _inverseMatrix = _matrix;
}

inline mat4f
Transform::localMatrix() const
{
  return mat4f::TRS(_localPosition, _localRotation, _localScale);
}

inline mat4f
Transform::inverseLocalMatrix() const
{
  mat3f r{_localRotation};

  r[0] *= math::inverse(_localScale[0]);
  r[1] *= math::inverse(_localScale[1]);
  r[2] *= math::inverse(_localScale[2]);

  mat4f m;

  m[0].set(r[0][0], r[1][0], r[2][0]);
  m[1].set(r[0][1], r[1][1], r[2][1]);
  m[2].set(r[0][2], r[1][2], r[2][2]);
  m[3][0] = -(r[0].dot(_localPosition));
  m[3][1] = -(r[1].dot(_localPosition));
  m[3][2] = -(r[2].dot(_localPosition));
  m[3][3] = 1.0f;
  return m;
}

void
Transform::setPosition(const vec3f& position)
{
  setLocalPosition(parent()->inverseTransform(position));
}

void
Transform::setRotation(const quatf& rotation)
{
  setLocalRotation(parent()->_rotation.inverse() * rotation);
}

void
Transform::translate(const vec3f& t, Space space)
{
  if (space == Space::Local)
    setPosition(_position + transformDirection(t));
  else
    setPosition(_position + t);
}

void
Transform::rotate(const quatf& q, Space space)
{
  if (space == Space::World)
    setLocalRotation(_localRotation * (_rotation.inverse() * q * _rotation));
  else
    setLocalRotation(_localRotation * q);
}

void
Transform::reset()
{
  _localPosition = _localEulerAngles = vec3f{0.0f};
  _localRotation = quatf::identity();
  _localScale = vec3f{1.0f};
  update();
}

void
Transform::update()
{
  auto p = parent();

  _matrix = p->_matrix * localMatrix();
  _position = translation(_matrix);
  _rotation = p->_rotation * _localRotation;
  _lossyScale = scale(_rotation, _matrix);
  _inverseMatrix = inverseLocalMatrix() * p->_inverseMatrix;
  // TODO: update the transform of all scene object's children.
  changed = true;
}

void
Transform::parentChanged()
{
  auto p = parent();
  auto m = p->_inverseMatrix * _matrix;

  _localPosition = translation(m);
  _localRotation = p->_rotation.inverse() * _rotation;
  _localEulerAngles = _localRotation.eulerAngles();
  _localScale = scale(_localRotation, m);
  _matrix = p->_matrix * localMatrix();
  _lossyScale = scale(_rotation, _matrix);
  _inverseMatrix = inverseLocalMatrix() * p->_inverseMatrix;
  // TODO: update the transform of all scene object's children.
  changed = true;
}

void
Transform::print(FILE* out) const
{
  fprintf(out, "Name: %s\n", sceneObject()->name());
  _localPosition.print("Local position: ", out);
  _localEulerAngles.print("Local rotation: ", out);
  _localScale.print("Local scale: ", out);
  _position.print("Position: ", out);
  _rotation.eulerAngles().print("Rotation: ", out);
  _lossyScale.print("Lossy scale: ", out);
  _matrix.print("Local2WorldMatrix", out);
  _inverseMatrix.print("World2LocalMatrix", out);
}

} // end namespace cg
