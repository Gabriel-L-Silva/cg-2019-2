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
// OVERVIEW: BVH.h
// ========
// Class definition for BVH.
//
// Author(s): Paulo Pagliosa (and your name)
// Last revision: 18/11/2019

#ifndef __BVH_h
#define __BVH_h

#include "graphics/GLMesh.h"
#include "Intersection.h"
#include <functional>
#include <vector>

namespace cg
{ // begin namespace cg

struct BVHNodeInfo
{
  Bounds3f bounds;
  bool isLeaf;
  int firstTriangleIndex;
  int numberOfTriangles;

}; // BVHNodeInfo

using BVHNodeFunction = std::function<void(const BVHNodeInfo&)>;

class BVH: public SharedObject
{
public:
  BVH(TriangleMesh& mesh, int maxTrisPerNode = 16);

  ~BVH() override;

  const TriangleMesh* mesh() const
  {
    return _mesh;
  }

  Bounds3f bounds() const;
  void iterate(BVHNodeFunction f) const;

  bool intersect(const Ray& ray, Intersection& hit) const;

private:
  struct Node;

  using TriangleIndexArray = std::vector<int>;

  Reference<TriangleMesh> _mesh;
  TriangleIndexArray _triangles;
  Node* _root{};
  int _nodeCount{};
  int _maxTrisPerNode;

  struct TriangleInfo;

  using TriangleInfoArray = std::vector<TriangleInfo>;

  Node* makeLeaf(TriangleInfoArray&,
    int start,
    int end,
    TriangleIndexArray&);

  Node* makeNode(TriangleInfoArray&,
    int start,
    int end,
    TriangleIndexArray&);

}; // BVH

} // end namespace cg

#endif // __BVH_h
