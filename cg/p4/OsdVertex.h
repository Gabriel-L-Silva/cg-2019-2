// OVERVIEW: OsdVertex.h
// ========
// Class definition for OsdVertex need for using the OSD API.


#ifndef __OsdVertex_h
#define __OsdVertex_h

#include "math/Vector3.h"

namespace cg
{ // begin namespace cg


/////////////////////////////////////////////////////////////////////
//
// OsdVertex: OsdVertex class
// ======
class OsdVertex
{
public:
  // Minimal required interface ----------------------
  OsdVertex() { }

  OsdVertex(OsdVertex const& src) {
    _position[0] = src._position[0];
    _position[1] = src._position[1];
    _position[2] = src._position[2];
  }

  void Clear(void* = 0) {
    _position[0] = _position[1] = _position[2] = 0.0f;
  }

  void AddWithWeight(OsdVertex const& src, float weight) {
    _position[0] += weight * src._position[0];
    _position[1] += weight * src._position[1];
    _position[2] += weight * src._position[2];
  }

  // Public interface ------------------------------------
  void SetPosition(float x, float y, float z) {
    _position[0] = x;
    _position[1] = y;
    _position[2] = z;
  }

  const float* GetPosition() const {
    return _position;
  }

private:
  float _position[3];
};
} // end namespace cg

#endif // __Camera_h
