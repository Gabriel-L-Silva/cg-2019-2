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
// OVERVIEW: SceneEditor.h
// ========
// Class definition for scene editor.
//
// Author(s): Paulo Pagliosa (and your name)
// Last revision: 23/09/2019

#ifndef __SceneEditor_h
#define __SceneEditor_h

#include "Camera.h"
#include "Scene.h"
#include "graphics/GLGraphics3.h"

namespace cg
{ // begin namespace cg


/////////////////////////////////////////////////////////////////////
//
// SceneEditor: scene editor class
// ===========
class SceneEditor: public GLGraphics3
{
public:
  bool showGround{true};

  SceneEditor(Scene& scene):
    _scene{&scene},
    _editor{"\0x1bSceneEditor", scene},
    _camera{new Camera}
  {
    SceneObject::makeUse(&_editor);
    _editor.setParent(scene.root());
    _editor.addComponent(_camera);
  }

  const Camera* camera() const
  {
    return _camera;
  }

  Camera* camera()
  {
    return _camera;
  }

  void setDefaultView(float aspect = 1);
  void zoom(float s);
  void rotateView(float ax, float ay);
  void orbit(float ax, float ay);
  void pan(const vec3f& d);

  void pan(float dx, float dy, float dz)
  {
    pan({dx, dy, dz});
  }
    
  auto orbitDistance() const
  {
    return _orbitDistance;
  }

  void newFrame();

private:
  Reference<Scene> _scene;
  SceneObject _editor;
  Reference<Camera> _camera;
  float _orbitDistance{10};

}; // SceneEditor

} // end namespace cg

#endif // __SceneEditor_h
