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
		auto org = t->worldToLocalMatrix().transform(ray.origin);
		auto dirL = t->worldToLocalMatrix().transformVector(ray.direction);
		auto d = math::inverse(dirL.length()); // ||s||
		Ray localRay{ org, dirL };
		float tMin;
		float tMax;

		//localRay.direction *= d; // normaliza raio local
		if (_mesh->bounds().intersect(localRay, tMin, tMax))
		{
			// mesh data
			auto& data = _mesh->data();
			auto distance = math::Limits<float>::inf();
			auto intersect = false;
			for (int i = 0; i < data.numberOfTriangles; ++i)
			{
				auto p0 = data.vertices[data.triangles[i].v[0]];
				auto p1 = data.vertices[data.triangles[i].v[1]];
				auto p2 = data.vertices[data.triangles[i].v[2]];

				vec3f e1{ p1 - p0 };
				vec3f e2{ p2 - p0 };
				vec3f s1{ vec3f::cross(localRay.direction, e2) };

				auto s1_e1 = vec3f::dot(s1, e1);

				// se s1 * e1
				if (math::isZero(s1_e1)) continue;

				auto invD = math::inverse(s1_e1);

				vec3f s{ localRay.origin - p0 };
				vec3f s2{ vec3f::cross(s, e1) };

				auto t = s2.dot(e2) * invD;
				if (!isgreaterequal(t, 0.0f)) continue;
				if ((distance = t * d) > hit.distance) continue;

				auto b1 = s1.dot(s) * invD;
				if (!isgreaterequal(b1, 0.0f)) continue;

				auto b2 = s2.dot(localRay.direction) * invD;
				if (!isgreaterequal(b2, 0.0f)) continue;

				if (b1 + b2 <= 1.0f)
				{
					// calcular info da intersecao
					// t = t_l / |s|
					hit.distance = distance;
					hit.triangleIndex = i;
					hit.object = this;
					// coord baricentricas
					hit.p = vec3f{ 1 - b1 - b2, b1, b2 };
					intersect = true; // making sure it intersected
				}
			}
			return intersect;
		}
		return false;
	}
}
