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
// OVERVIEW: Transform.cpp
// ========
// Source file for scene object transform.
//
// Author(s): Paulo Pagliosa (and your name)
// Last revision: 25/08/2018

#include "Transform.h"

namespace cg
{ // begin namespace cg

namespace internal
{ // begin namespace internal

template <typename real>
inline Matrix4x4<real>
inverseTRS(const Matrix4x4<real>& trs)
{
  auto u = trs[0];
  auto v = trs[1];
  auto w = trs[2];

  u *= math::inverse(u.squaredNorm());
  v *= math::inverse(v.squaredNorm());
  w *= math::inverse(w.squaredNorm());

  auto p = trs[3];
  Matrix4x4<real> inv;

  inv[0].set(u.x, v.x, w.x);
  inv[1].set(u.y, v.y, w.y);
  inv[2].set(u.z, v.z, w.z);
  inv[3].set(-(u.dot(p)), -(v.dot(p)), -(w.dot(p)), real(1));
  return inv;
}

} // end namespace internal


/////////////////////////////////////////////////////////////////////
//
// Transform implementation
// =========
Transform::Transform():
  Component{"Transform"},
  _localPosition{0.0f},
  _localEulerAngles{0.0f},
  _localScale{1.0f}
{
  update();
}

inline mat4f
Transform::localMatrix() const
{
  return mat4f::TRS(_localPosition, _localEulerAngles, _localScale);
}

void
Transform::setPosition(const vec3f& position)
{
  // Coming soon (PP).
}

void
Transform::setEulerAngles(const vec3f& angles)
{
  // Coming soon (PP).
}

void
Transform::translate(const vec3f& t, Space space)
{
  // Coming soon (PP).
}

void
Transform::rotate(const quatf& q, Space space)
{
  // Coming soon (PP).
}

void
Transform::update()
{
  _matrix = localMatrix();
  _inverseMatrix = internal::inverseTRS(_matrix);
  // TODO: this method is not finished yet (PP).
}

} // end namespace cg
