// OVERVIEW: OsdPrimitive.h
// ========
// Class definition for OSD primitive.


#ifndef __OsdPrimitive_h
#define __OsdPrimitive_h
#define EPSILON 0.000000000000001

#include "Component.h"
#include <opensubdiv/osd/glMesh.h>
#include "Material.h"
#include "Intersection.h"
#include "BVH.h"

namespace cg
{ // begin namespace cg


/////////////////////////////////////////////////////////////////////
//
// OsdPrimitive: OsdPrimitive class
// =========
  class OsdPrimitive : public Component
  {
  public:
    Material material;

    OsdPrimitive(OpenSubdiv::Osd::GLMeshInterface* mesh, const std::string& meshName) :
      Component{ "OsdPrimitive" },
      _mesh{ mesh },
      _meshName(meshName)
    {
      // do nothing
    }

    OpenSubdiv::Osd::GLMeshInterface* mesh() const
    {
      return _mesh;
    }

    const char* const meshName() const
    {
      return _meshName.c_str();
    }

    void setMesh(OpenSubdiv::Osd::GLMeshInterface* mesh, const std::string& meshName)
    {
      _mesh = mesh;
      _meshName = meshName;
    }

    void setMeshName(const std::string& meshName)
    {
      _meshName = meshName;
    }

  private:
    OpenSubdiv::Osd::GLMeshInterface* _mesh;
    std::string _meshName;
    Reference<BVH> _BVH;


  }; // OsdPrimitive

} // end namespace cg

#endif // __OsdPrimitive_h
