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
// OVERVIEW: Assets.cpp
// ========
// Source file for assets.
//
// Author(s): Paulo Pagliosa (and your name)
// Last revision: 01/10/2019

#include "Assets.h"
#include "graphics/Application.h"
#include <filesystem>

namespace cg
{ // begin namespace cg

namespace fs = std::filesystem;


/////////////////////////////////////////////////////////////////////
//
// Assets implementation
// ======
MeshMap Assets::_meshes;

void
Assets::initialize()
{
  fs::path meshPath{Application::assetFilePath("meshes/")};

  if (fs::is_directory(meshPath))
  {
    auto p = fs::directory_iterator(meshPath);

    for (auto e = fs::directory_iterator(); p != e; ++p)
      if (fs::is_regular_file(p->status()))
        _meshes[p->path().filename().string()] = nullptr;
  }
}

TriangleMesh*
Assets::loadMesh(MeshMapIterator mit)
{
  if (mit == _meshes.end())
    return nullptr;

  TriangleMesh* m{mit->second};

  if (m == nullptr)
  {
    auto filename = "meshes/" + mit->first;

    m = Application::loadMesh(filename.c_str());
    _meshes[mit->first] = m;
  }
  return m;
}

} // end namespace cg
