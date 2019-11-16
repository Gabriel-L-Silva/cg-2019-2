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
// OVERVIEW: GLRenderer.cpp
// ========
// Source file for OpenGL renderer.
//
// Author(s): Paulo Pagliosa (and your name)
// Last revision: 09/09/2019

#include "GLRenderer.h"
#include "P3.h"

namespace cg
{ // begin namespace cg


//////////////////////////////////////////////////////////
//
// GLRenderer implementation
// ==========
void
GLRenderer::update()
{
  Renderer::update();
  // TODO
}

void
GLRenderer::render()
{
	auto _program = getProgram();
	const auto& bc = _scene->backgroundColor;

	_program->use(); //garante que o programP está em uso 

	glClearColor(bc.r, bc.g, bc.b, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	// TODONE
	auto c = camera();
	//const auto& p = c->transform()->position();
	//auto vp = vpMatrix(c);
	//_program->setUniformMat4("vpMatrix", vp);
	//_program->setUniformVec4("ambientLight", scene()->ambientLight);
	//_program->setUniformVec3("lightPosition", p);
	
	const auto& s = _scene;
	auto it = s->getPrimitiveIter();
	auto end = s->getPrimitiveEnd();
	for (; it != end; it++)
	{
		auto component = (Component*)* it;
		auto o = (*it)->sceneObject();

		if (auto primitive = dynamic_cast<Primitive*>(component))
		{
			auto m = glMesh(primitive->mesh());

			if (nullptr == m)
				return;

			auto t = primitive->transform();
			auto normalMatrix = mat3f{ t->worldToLocalMatrix() }.transposed();

			_program->setUniformMat4("transform", t->localToWorldMatrix());
			_program->setUniformMat3("normalMatrix", normalMatrix);
			_program->setUniformVec4("material.ambient", primitive->material.ambient);
			_program->setUniformVec4("material.diffuse", primitive->material.diffuse);
			_program->setUniformVec4("material.spot", primitive->material.spot);
			_program->setUniform("material.shine", primitive->material.shine);
			_program->setUniform("flatMode", (int)0);
			m->bind();
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
			glDrawElements(GL_TRIANGLES, m->vertexCount(), GL_UNSIGNED_INT, 0);
		}
	}
}

} // end namespace cg
