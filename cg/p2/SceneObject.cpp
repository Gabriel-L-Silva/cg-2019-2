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
// OVERVIEW: SceneObject.cpp
// ========
// Source file for scene object.
//
// Author(s): Paulo Pagliosa (and your name)
// Last revision: 25/08/2018

#include "SceneObject.h"
#include "Scene.h"
#include "Primitive.h"
#include "Camera.h"

namespace cg
{ // begin namespace cg


/////////////////////////////////////////////////////////////////////
//
// SceneObject implementation
// ===========

void
SceneObject::setEditorParent()
{
	_parent = nullptr;
}

void
SceneObject::setParent(SceneObject* parent, bool creating)
{
	auto p = _parent;
  if (parent == nullptr)
	{
		_parent = nullptr;
		_scene->addRoot(this);
	}
	else
	{
		parent->add(this);
	}
	if (p != nullptr)
		p->remove(this);
	else if (!creating)
		_scene->remove(this);
	transform()->parentChanged();
}

void 
SceneObject::add(Reference<SceneObject> object)
{
	object->_parent = this;
	_children.add(object);
}

void
SceneObject::remove(Reference<SceneObject> object)
{
	_children.remove(object);
}

void
SceneObject::add(Reference<Component> object)
{
	if (auto primitive = dynamic_cast<Primitive*>((Component*)object)) {
		_scene->addRenderable(object);
	}
	else if (auto cam = dynamic_cast<Camera*>((Component*)object))
	{
		_scene->addRenderable(object);
	}
	object->_sceneObject = this;
	_components.add(object);
}

void
SceneObject::remove(Reference<Component> object)
{
	_components.remove(object);
}

bool
SceneObject::childrenContain(Reference<SceneObject> obj)
{
	auto ret = false;
	if (!_children.isEmpty())
	{
		auto it = _children.getIter();
		auto end = _children.getEnd();
		if (std::find(it, end, obj) != _children.getEnd())
			return true;
		else
		{
			it = _children.getIter();
			end = _children.getEnd();
			for (; it != end; it++)
			{
				ret = (*it)->childrenContain(obj);
				if (ret) break;
			}
		}
	}
	return ret;
}

} // end namespace cg
