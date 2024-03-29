//[]---------------------------------------------------------------[]
//|                                                                 |
//| Copyright (C) 2018, 2109 Orthrus Group.                         |
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
// OVERVIEW: GLRenderer.h
// ========
// Class definition for OpenGL renderer.
//
// Author(s): Paulo Pagliosa (and your name)
// Last revision: 21/09/2019

#ifndef __GLRenderer_h
#define __GLRenderer_h

#include "Renderer.h"
#include "graphics/GLGraphics3.h"

namespace cg
{ // begin namespace cg


//////////////////////////////////////////////////////////
//
// GLRenderer: OpenGL renderer class
// ==========
class GLRenderer: public Renderer
{
public:
	GLRenderer(Scene& scene, Camera* camera = nullptr) :
		Renderer{ scene, camera },
		_program{ }
  {
    // TODO
  }

  void update() override;
  void render() override;

	auto getProgram() const
	{
		return _program;
	}

	auto setProgram(GLSL::Program* program)
	{
		_program = program;
	}

private:
	GLSL::Program* _program;
}; // GLRenderer

} // end namespace cg

#endif // __GLRenderer_h
