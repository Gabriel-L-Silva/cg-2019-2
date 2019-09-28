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
// OVERVIEW: Vector2.h
// ========
// Class definition for 2D vector.
//
// Author: Paulo Pagliosa
// Last revision: 05/09/2019

#ifndef __Vector2_h
#define __Vector2_h

#include "math/Real.h"
#include <algorithm>
#include <cstdio>

namespace cg
{ // begin namespace cg


/////////////////////////////////////////////////////////////////////
//
// Vector2: 2D vector class
// =======
template <typename real>
class Vector2
{
public:

  using vec2 = Vector2<real>;
  using value_type = real;

  real x;
  real y;

  /// Default constructor.
  HOST DEVICE
  Vector2()
  {
    // do nothing
  }

  /// Constructs a Vector2 object from (x, y).
  HOST DEVICE
  Vector2(real x, real y)
  {
    set(x, y);
  }

  /// Constructs a Vector2 object from v[2].
  HOST DEVICE
  explicit Vector2(const real v[])
  {
    set(v);
  }

  /// Constructs a Vector2 object with (s, s).
  HOST DEVICE
  explicit Vector2(real s)
  {
    set(s);
  }

  template <typename V>
  HOST DEVICE
  explicit Vector2(const V& v):
    x{real(v.x)},
    y{real(v.y)}
  {
    // do nothing
  }

  /// Sets this object to v.
  HOST DEVICE
  void set(const vec2& v)
  {
    *this = v;
  }

  /// Sets the coordinates of this object to (x, y).
  HOST DEVICE
  void set(real x, real y)
  {
    this->x = x;
    this->y = y;
  }

  /// Sets the coordinates of this object to v[2].
  HOST DEVICE
  void set(const real v[])
  {
    x = v[0];
    y = v[1];
  }

  /// Sets the coordinates of this object to (s, s).
  HOST DEVICE
  void set(real s)
  {
    x = y = s;
  }


  /// Sets the coordinates of this object from v.
  template <typename V>
  HOST DEVICE
  void set(const V& v)
  {
    set(real(v.x), real(v.y));
  }

  template <typename V>
  HOST DEVICE
  vec2& operator =(const V& v)
  {
    set(v);
    return *this;
  }

  /// Returns a null vector.
  HOST DEVICE
  static vec2 null()
  {
    return vec2{real(0)};
  }

  /// Returns ths size of this object.
  HOST DEVICE
  constexpr int size() const
  {
    return 2;
  }

  /// Returns true if this object is equal to v.
  HOST DEVICE
  bool equals(const vec2& v, real eps = math::Limits<real>::eps()) const
  {
    return math::isNull(x - v.x, y - v.y, eps);
  }

  HOST DEVICE
  bool operator ==(const vec2& v) const
  {
    return equals(v);
  }

  /// Returns true if this object is not equal to v.
  HOST DEVICE
  bool operator !=(const vec2& v) const
  {
    return !operator ==(v);
  }

  /// Returns a reference to this object += v.
  HOST DEVICE
  vec2& operator +=(const vec2& v)
  {
    x += v.x;
    y += v.y;
    return *this;
  }

  /// Returns a reference to this object -= v.
  HOST DEVICE
  vec2& operator -=(const vec2& v)
  {
    x -= v.x;
    y -= v.y;
    return *this;
  }

  /// Returns a reference to this object *= s.
  HOST DEVICE
  vec2& operator *=(real s)
  {
    x *= s;
    y *= s;
    return *this;
  }

  /// Returns a reference to this object *= v.
  HOST DEVICE
  vec2& operator *=(const vec2& v)
  {
    x *= v.x;
    y *= v.y;
    return *this;
  }

  /// Returns a reference to the i-th coordinate of this object.
  HOST DEVICE
  real& operator [](int i)
  {
    return (&x)[i];
  }

  /// Returns the i-th coordinate of this object.
  HOST DEVICE
  const real& operator [](int i) const
  {
    return (&x)[i];
  }

  /// Returns a pointer to the elements of this object.
  HOST DEVICE
  explicit operator const real*() const
  {
    return &x;
  }

  /// Returns this object + v.
  HOST DEVICE
  vec2 operator +(const vec2& v) const
  {
    return vec2{x + v.x, y + v.y};
  }

  /// Returns this object - v.
  HOST DEVICE
  vec2 operator -(const vec2& v) const
  {
    return vec2{x - v.x, y - v.y};
  }

  /// Returns a vector in the direction opposite to this object.
  HOST DEVICE
  vec2 operator -() const
  {
    return vec2{-x, -y};
  }

  /// Returns the scalar multiplication of this object and s.
  HOST DEVICE
  vec2 operator *(real s) const
  {
    return vec2{x * s, y * s};
  }

  /// Returns the multiplication of this object and v.
  HOST DEVICE
  vec2 operator *(const vec2& v) const
  {
    return vec2{x * v.x, y * v.y};
  }

  /// Returns true if this object is null.
  HOST DEVICE
  bool isNull(real eps = math::Limits<real>::eps()) const
  {
    return math::isNull(x, y, eps);
  }

  /// Returns the squared norm of this object.
  HOST DEVICE
  real squaredNorm() const
  {
    return math::sqr(x) + math::sqr(y);
  }

  /// Returns the length of this object.
  HOST DEVICE
  real length() const
  {
    return real(sqrt(squaredNorm()));
  }

  /// Returns the maximum coordinate of this object.
  HOST DEVICE
  real max() const
  {
    return std::max(x, y);
  }

  /// Returns the minimum coordinate of this object.
  HOST DEVICE
  real min() const
  {
    return std::min(x, y);
  }

  /// Returns the inverse of this object.
  HOST DEVICE
  vec2 inverse() const
  {
    return vec2{1 / x, 1 / y};
  }

  /// Inverts and returns a reference to this object.
  HOST DEVICE
  vec2& invert()
  {
    x = 1 / x;
    y = 1 / y;
    return *this;
  }

  /// Negates and returns a reference to this object.
  HOST DEVICE
  vec2& negate()
  {
    x = -x;
    y = -y;
    return *this;
  }

  /// Normalizes and returns a reference to this object.
  HOST DEVICE
  vec2& normalize(real eps = math::Limits<real>::eps())
  {
    const auto len = length();

    if (!math::isZero(len, eps))
      operator *=(math::inverse(len));
    return *this;
  }

  /// Returns the unit vector of this this object.
  HOST DEVICE
  vec2 versor(real eps = math::Limits<real>::eps()) const
  {
    return vec2(*this).normalize(eps);
  }

  /// Returns the unit vector of v.
  HOST DEVICE
  static vec2 versor(const vec2& v, real eps = math::Limits<real>::eps())
  {
    return v.versor(eps);
  }

  /// Returns the dot product of this object and v.
  HOST DEVICE
  real dot(const vec2& v) const
  {
    return x * v.x + y * v.y;
  }

  /// Returns the dot product of this object and (x, y).
  HOST DEVICE
  real dot(real x, real y) const
  {
    return dot(vec2{x, y});
  }

  /// Returns the dot product of v and w.
  HOST DEVICE
  static real dot(const vec2& v, const vec2& w)
  {
    return v.dot(w);
  }

  void print(const char* s, FILE* f = stdout) const
  {
    fprintf(f, "%s<%g,%g>\n", s, x, y);
  }

}; // Vector2

/// Returns the scalar multiplication of s and v.
template <typename real>
HOST DEVICE inline Vector2<real>
operator *(double s, const Vector2<real>& v)
{
  return v * real(s);
}

using vec2f = Vector2<float>;
using vec2d = Vector2<double>;

} // end namespace cg

#endif // __Vector2_h
