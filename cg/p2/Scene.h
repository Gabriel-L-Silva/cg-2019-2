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
// OVERVIEW: Scene.h
// ========
// Class definition for scene.
//
// Author(s): Paulo Pagliosa, Gabriel Lucas da Silva e Lucas Santana
// Last revision: 25/08/2018

#ifndef __Scene_h
#define __Scene_h

#include "SceneObject.h"
#include "graphics/Color.h"
#include <vector>
#include "Primitive.h"
namespace cg
{ // begin namespace cg


/////////////////////////////////////////////////////////////////////
//
// Scene: scene class
// =====
class Scene: public SceneNode
{
public:
  Color backgroundColor{Color::gray};
	Color ambientLight{ Color::black };

  /*
	manter uma lista de primitives
	*/

  /// Constructs an empty scene.
  Scene(const char* name):
    SceneNode{name}
  {
    // do nothing
  }

	~Scene()
	{
		_root.clear();
		renderables.clear();
	}
	auto getPrimitiveIter()
	{
		return renderables.getIter();
	}

	auto getPrimitiveEnd()
	{
		return renderables.getEnd();
	}

	void addRenderable(Reference<Component> p)
	{
		renderables.add(p);
	}

	auto getRootIt() 
	{
		return _root.getIter();
	}

	auto getRootEnd() 
	{
		return _root.getEnd();
	}

	auto getRootSize()
	{
		return _root.getSize();
	}

	auto addRoot(Reference<SceneObject> object)
	{
		return _root.add(object);
	}

	auto remove(Reference<SceneObject> object) {
		_root.remove(object);
	}

	auto remove(Reference<Component> object) {
		renderables.remove(object);
	}

	bool isRootEmpty()
	{
		return _root.isEmpty();
	}

	auto root() const
	{
		return _root;
	}

private:
	// lista de todos os sceneObj que tem primitive
	Collection<Reference<Component>> renderables;
	Collection<Reference<SceneObject>> _root;
	
}; // Scene

} // end namespace cg

#endif // __Scene_h
