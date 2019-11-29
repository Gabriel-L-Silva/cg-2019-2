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
// OVERVIEW: Primitive.cpp
// ========
// Source file for primitive.
//
// Author(s): Paulo Pagliosa (and your name)
// Last revision: 30/10/2018

#include "Primitive.h"
#include "Transform.h"
#include "Intersection.h"
#include <iostream>
namespace cg
{ // begin namespace cg

bool
Primitive::intersect(const Ray& ray, Intersection& hit) const
{
  if (_mesh == nullptr)
    return false;

  auto t = const_cast<Primitive*>(this)->transform();
  auto o = t->worldToLocalMatrix().transform(ray.origin);
  auto D = t->worldToLocalMatrix().transformVector(ray.direction);
  auto d = math::inverse(D.length()); // ||s||
  Ray localRay{o, D};
	D = localRay.direction;
	o = localRay.origin;
  float tMin;
  float tMax;

  //localRay.direction *= d;
  if (_mesh->bounds().intersect(localRay, tMin, tMax))
  {
    // TODO: mesh intersection
		auto data = _mesh->data();
		bool ret = false;
		auto nt = data.numberOfTriangles;
		auto nv = data.numberOfVertices;

		for (int i = 0; i < nt; i++)
		{
			auto ti = data.triangles[i];
			auto p0 = data.vertices[ti.v[0]];
			auto p1 = data.vertices[ti.v[1]];
			auto p2 = data.vertices[ti.v[2]];

			auto e1 = p1 - p0;
			auto e2 = p2 - p0;
			auto s1 = D.cross(e2);

			auto s1e1 = s1.dot(e1);
			if (math::isZero(abs(s1e1)))
				continue;
			auto invd = 1 / s1e1;

			auto s = o - p0;
			auto s2 = s.cross(e1);
			auto t = s2.dot(e2) * invd;
			if (!isgreaterequal(t,0.0f))
				continue;

			auto dist = t * d;
			if (dist > hit.distance)
				continue;

			auto b1 = s1.dot(s) * invd;
			if (!isgreaterequal(b1, 0.0f))
				continue;

			auto b2 = s2.dot(D) * invd;
			if (!isgreaterequal(b2, 0.0f))
				continue;

			auto b1b2 = b1 + b2;
			if (isgreater(b1b2, 1))
				continue;


			hit.object = this;
			hit.triangleIndex = i;
			hit.distance = dist;
			hit.p = vec3f{ 1 - b1b2, b1, b2 };
			ret = true;
		}
		return ret;
  }
	return false;
}

} // end namespace cg
