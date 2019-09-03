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

  /*
	manter uma lista de primitives
	*/

  /// Constructs an empty scene.
  Scene(const char* name):
    SceneNode{name}
  {
    // do nothing

  }
	auto getPrimitiveIter()
	{
		return _primitives.getIter();
	}

	auto getPrimitiveEnd()
	{
		return _primitives.getEnd();
	}

	void addPrimitive(Reference<Primitive> p)
	{
		_primitives.add(p);
	}

	auto getRootIt() 
	{
		return _root.getIter();
	}

	auto getRootEnd() 
	{
		return _root.getEnd();
	}

	auto addRoot(Reference<SceneObject> object)
	{
		return _root.add(object);
	}

	auto remove(Reference<SceneObject> object) {
		_root.remove(object);
	}

	auto remove(Reference<Primitive> object) {
		_primitives.remove(object);
	}

	bool isRootEmpty()
	{
		return _root.isEmpty();
	}
private:
	// lista de todos os sceneObj que tem primitive
	Collection<Reference<Primitive>> _primitives;
	Collection<Reference<SceneObject>> _root;
	
}; // Scene

} // end namespace cg

#endif // __Scene_h
