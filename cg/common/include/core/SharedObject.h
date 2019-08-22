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
// OVERVIEW: SharedObject.h
// ========
// Class definition for shared object.
//
// Author: Paulo Pagliosa
// Last revision: 23/08/2018

#ifndef __SharedObject_h
#define __SharedObject_h

#include <type_traits>

namespace cg
{ // begin namespace cg

//
// Forward definition
//
class SharedObject;

template <typename T>
inline constexpr bool isSharedObject()
{
  return std::is_assignable<SharedObject, T>::value;
}

#define ASSERT_SHARED(T, msg) static_assert(isSharedObject<T>(), msg)


/////////////////////////////////////////////////////////////////////
//
// SharedObject: shared object class
// ============
class SharedObject
{
public:
  /// Destructor.
  virtual ~SharedObject() = default;

  /// Returns the number of references of this object.
  auto referenceCount() const
  {
    return _referenceCount;
  }

  template <typename T>
  static T* makeUse(T* ptr)
  {
    ASSERT_SHARED(T, "Pointer to shared object expected");
    if (ptr != nullptr)
      ++ptr->_referenceCount;
    return ptr;
  }

  template <typename T>
  static void release(T* ptr)
  {
    ASSERT_SHARED(T, "Pointer to shared object expected");
    if (ptr != nullptr && --ptr->_referenceCount <= 0)
      delete ptr;
  }

protected:
  /// Constructs an unreferenced object.
  SharedObject() = default;

private:
  int _referenceCount{};
  
}; // SharedObject


/////////////////////////////////////////////////////////////////////
//
// Reference: shared object reference class
// =========
template <typename T>
class Reference
{
public:
  using reference = Reference<T>;

  Reference():
    _ptr{nullptr}
  {
    // do nothing
  }

  Reference(const reference& other):
    _ptr{SharedObject::makeUse(other._ptr)}
  {
    // do nothing
  }

  Reference(T* ptr):
    _ptr{SharedObject::makeUse(ptr)}
  {
    // do nothing
  }

  ~Reference()
  {
    SharedObject::release(_ptr);
  }

  reference& operator =(const reference& other)
  {
    return operator=(other._ptr);
  }

  reference& operator =(T* ptr)
  {
    SharedObject::release(_ptr);
    _ptr = SharedObject::makeUse(ptr);
    return *this;
  }

  bool operator ==(const reference& other) const
  {
    return operator ==(other._ptr);
  }

  bool operator ==(const T* ptr) const
  {
    return _ptr == ptr;
  }

  bool operator !=(const reference& other) const
  {
    return !operator ==(other);
  }

  bool operator !=(const T* ptr) const
  {
    return !operator ==(ptr);
  }

  operator T*() const
  {
    return _ptr;
  }

  auto operator->() const
  {
    return _ptr;
  }

  auto get() const
  {
    return _ptr;
  }

private:
  T* _ptr;

}; // Reference

} // end namespace cg

#endif // __SharedObject_h
