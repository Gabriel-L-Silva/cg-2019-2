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
// OVERVIEW: RayTracer.cpp
// ========
// Source file for simple ray tracer.
//
// Author(s): Paulo Pagliosa (and your name)
// Last revision: 16/10/2019

#include "Camera.h"
#include "RayTracer.h"
#include "Light.h"
#include <time.h>

using namespace std;

namespace cg
{ // begin namespace cg

void
printElapsedTime(const char* s, clock_t time)
{
  printf("%sElapsed time: %.4f s\n", s, (float)time / CLOCKS_PER_SEC);
}


/////////////////////////////////////////////////////////////////////
//
// RayTracer implementation
// =========
RayTracer::RayTracer(Scene& scene, Camera* camera):
  Renderer{scene, camera},
  _maxRecursionLevel{6},
  _minWeight{MIN_WEIGHT}
{
  // TODO: BVH
}

void
RayTracer::render()
{
  throw std::runtime_error("RayTracer::render() invoked");
}

inline float
windowHeight(Camera* c)
{
  if (c->projectionType() == Camera::Parallel)
    return c->height();
  return c->nearPlane() * tan(math::toRadians(c->viewAngle() * 0.5f)) * 2;

}
void
RayTracer::renderImage(Image& image)
{
  auto t = clock();
  const auto& m = _camera->cameraToWorldMatrix();

  // VRC axes
  _vrc.u = m[0];
  _vrc.v = m[1];
  _vrc.n = m[2];
  // init auxiliary mapping variables
  _W = image.width();
  _H = image.height();
  _Iw = math::inverse(float(_W));
  _Ih = math::inverse(float(_H));

  auto height = windowHeight(_camera);

  _W >= _H ? _Vw = (_Vh = height) * _W * _Ih : _Vh = (_Vw = height) * _H * _Iw;
  // init pixel ray
  _pixelRay.origin = _camera->transform()->position();
  _pixelRay.direction = -_vrc.n;
  _camera->clippingPlanes(_pixelRay.tMin, _pixelRay.tMax);
  _numberOfRays = _numberOfHits = 0;
  scan(image);
  printf("\nNumber of rays: %llu", _numberOfRays);
  printf("\nNumber of hits: %llu", _numberOfHits);
  printElapsedTime("\nDONE! ", clock() - t);
}

void
RayTracer::setPixelRay(float x, float y)
//[]---------------------------------------------------[]
//|  Set pixel ray                                      |
//|  @param x coordinate of the pixel                   |
//|  @param y cordinates of the pixel                   |
//[]---------------------------------------------------[]
{
  auto p = imageToWindow(x, y);

  switch (_camera->projectionType())
  {
    case Camera::Perspective:
      _pixelRay.direction = (p - _camera->nearPlane() * _vrc.n).versor();
      break;

    case Camera::Parallel:
      _pixelRay.origin = _camera->transform()->position() + p;
      break;
  }
}

void
RayTracer::scan(Image& image)
{
  ImageBuffer scanLine{_W, 1};

  for (int j = 0; j < _H; j++)
  {
    auto y = (float)j + 0.5f;

    printf("Scanning line %d of %d\r", j + 1, _H);
    for (int i = 0; i < _W; i++)
      scanLine[i] = shoot((float)i + 0.5f, y);
    image.setData(0, j, scanLine);
  }
}

Color
RayTracer::shoot(float x, float y)
//[]---------------------------------------------------[]
//|  Shoot a pixel ray                                  |
//|  @param x coordinate of the pixel                   |
//|  @param y cordinates of the pixel                   |
//|  @return RGB color of the pixel                     |
//[]---------------------------------------------------[]
{
  // set pixel ray
  setPixelRay(x, y);

  // trace pixel ray
  Color color = trace(_pixelRay, 0, 1.0f);

  // adjust RGB color
  if (color.r > 1.0f)
    color.r = 1.0f;
  if (color.g > 1.0f)
    color.g = 1.0f;
  if (color.b > 1.0f)
    color.b = 1.0f;
  // return pixel color
  return color;
}

Color
RayTracer::trace(const Ray& ray, uint32_t level, float weight)
//[]---------------------------------------------------[]
//|  Trace a ray                                        |
//|  @param the ray                                     |
//|  @param recursion level                             |
//|  @param ray weight                                  |
//|  @return color of the ray                           |
//[]---------------------------------------------------[]
{
  if (level > _maxRecursionLevel)
    return Color::black;
  _numberOfRays++;

  Intersection hit;

  return intersect(ray, hit) ? shade(ray, hit, level, weight) : background();
}

inline constexpr auto
rt_eps()
{
  return 1e-3f;
}

bool
RayTracer::intersect(const Ray& ray, Intersection& hit)
//[]---------------------------------------------------[]
//|  Ray/object intersection                            |
//|  @param the ray (input)                             |
//|  @param information on intersection (output)        |
//|  @return true if the ray intersects an object       |
//[]---------------------------------------------------[]
{
  hit.object = nullptr;
  hit.distance = ray.tMax;
  // TODO: insert your code here
	float minDistance = math::Limits<float>::inf();

	auto it = _scene->getPrimitiveIter();
	auto end = _scene->getPrimitiveEnd();
	for (; it != end; it++)
	{
		if (auto p = dynamic_cast<Primitive*>((Component*)(*it)))
		{
			if (p->intersect(ray, hit))
			{
				_numberOfHits++;
				if (hit.object != nullptr)
				{
					if (hit.distance < minDistance)
					{
						hit.object = p;
						minDistance = hit.distance;
					}
				}
				else
				{
					hit.object = p;
					minDistance = hit.distance;
				}
			}
		}
	}
  return hit.object != nullptr;
}

inline Color
RayTracer::directLight(const Ray& ray, Intersection& hit)
{
	//vec3f L{ 0.0f };
	Color IL = Color::black;
	auto material = hit.object->material;
	auto ambientLight = _scene->ambientLight;
	auto flatMode = 0;
	auto A = ambientLight * float(1 - flatMode); //IA
	auto OAIA = material.ambient * A;
	vec3f V;
	auto c = OAIA;
	float temp;
	float pw;
	auto prim = hit.object;
	vec3f L;
	

	auto data = hit.object->mesh()->data();
	auto N = data.vertexNormals[data.triangles[hit.triangleIndex].v[0]] * hit.p.x
		+ data.vertexNormals[data.triangles[hit.triangleIndex].v[1]] * hit.p.y
		+ data.vertexNormals[data.triangles[hit.triangleIndex].v[2]] * hit.p.z;
	
	N = (hit.object->sceneObject()->transform()->worldToLocalMatrix()).transposed().transform(N);
	N = N.versor();


	auto it = _scene->getPrimitiveIter();
	auto end = _scene->getPrimitiveEnd();
	for (; it != end; it++)
	{
		if (auto l = dynamic_cast<Light*>((Component*)(*it)))
		{
			auto p = ray.origin + hit.distance * ray.direction;
			auto LPosition = l->sceneObject()->transform()->position(); 
			auto LDirection = l->sceneObject()->transform()->rotation() * vec3f(0, 1, 0);
			L = l->type() == Light::Type::Directional ? LDirection : (LPosition - p);
			
		/*	if (N.dot(LDirection.versor()) < 0)
				N = -N;*/
			
			p += rt_eps() * N;
			V = (p - Camera::current()->transform()->position()).normalize();
			if (!shadow({ p,LDirection.versor() }))
			{

				switch (l->type())
				{
				case (0): //directional
					IL = l->color;
					break;

				case (1): //point 
					temp = L.length();
					pw = (pow(temp, l->decayValue()));
					IL = l->color * (1/pw);
					break;

				case (2): //spot
					temp = L.length();
					L = L.versor();
					float angle = LDirection.dot(L);
					pw = (pow(temp, l->decayValue()));
					IL = (acos(angle < 0 ? 0 : angle) < math::toRadians(l->openningAngle())) ? l->color * (1/pw) * pow(angle, l->decayExponent()) : Color::black;

					break;
				}
			}
			auto R = (reflect(LDirection, N)).normalize();

			//auto ODIL = elementWise({ material.diffuse.r,material.diffuse.g,material.diffuse.b,material.diffuse.a }, IL);
			//auto OSIL = elementWise({ material.spot.r,material.spot.g,material.spot.b,material.spot.a }, IL);

			auto ODIL = material.diffuse * IL;
			auto OSIL = material.spot * IL;

			auto firstTemp = ODIL * max(N.dot(LDirection),0.0f);
			auto secTemp = OSIL * pow(max(R.dot(V), 0.0f), material.shine);
			//firstTemp = elementWise(ODIL, max(dot(N, L), float(flatMode)); 
			//firstTemp = elementWise(OSIL, max(dot(N, L), float(flatMode) - 1);
			c += firstTemp + secTemp;
		}
	}
	return Color{c.x,c.y,c.z,c.w};
}

inline float
maxColor(Color c)
{
	float ret;
	auto r = c.r;
	auto g = c.g;
	auto b = c.b;

	ret = r > b ? r : b;
	ret = ret > g ? ret : g;
	return ret;
}

inline vec3f
RayTracer::reflect(vec3f v, vec3f n)
{
	return (2.0f * (float)n.dot(v) * n) * (1 / pow(n.length(), 2)) - v;
}

Color
RayTracer::shade(const Ray& ray, Intersection& hit, int level, float weight)
//[]---------------------------------------------------[]
//|  Shade a point P                                    |
//|  @param the ray (input)                             |
//|  @param information on intersection (input)         |
//|  @param recursion level                             |
//|  @param ray weight                                  |
//|  @return color at point P                           |
//[]---------------------------------------------------[]
{
  // TODO: insert your code here
	auto c = directLight(ray,hit);
	//auto Or = hit.object->material.specular;

	//if (Or != Color::black)
	//{
	//	auto w = weight * maxColor(Or);
	//	if (w > _minWeight)
	//	{
	//		auto data = hit.object->mesh()->data();
	//		auto r = reflect(ray.direction, data.vertexNormals[hit.triangleIndex]);
	//		c += trace({ hit.p,r }, level + 1, w);//Ray
	//	}
	//}
	return c;
}

Color
RayTracer::background() const
//[]---------------------------------------------------[]
//|  Background                                         |
//|  @return background color                           |
//[]---------------------------------------------------[]
{
  return _scene->backgroundColor;
}

bool
RayTracer::shadow(const Ray& ray)
//[]---------------------------------------------------[]
//|  Verifiy if ray is a shadow ray                     |
//|  @param the ray (input)                             |
//|  @return true if the ray intersects an object       |
//[]---------------------------------------------------[]
{
  Intersection hit;
  return intersect(ray, hit);
}

} // end namespace cg
