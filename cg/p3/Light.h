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
// OVERVIEW: Light.h
// ========
// Class definition for light.
//
// Author(s): Paulo Pagliosa (and your name)
// Last revision: 14/10/2019

#ifndef __Light_h
#define __Light_h

#include "Component.h"
#include "graphics/Color.h"

namespace cg
{ // begin namespace cg


/////////////////////////////////////////////////////////////////////
//
// Light: light class
// =====
class Light: public Component
{
public:

  enum Type
  {
    Directional,
    Point,
    Spot
  };

  Color color{Color::white};

	Light() :
		Component{ "Light" },
		_type{ Directional },
		_openningAngle{ 45 }
  {
    // do nothing
  }

  auto type() const
  {
    return _type;
  }

	void setColor(Color c)
	{
		color = c;
	}
  void setType(Type type)
  {
    _type = type;
  }

	int decayValue() const
	{
		return _decayValue;
	}

	void setDecayValue(int value)
	{
		if (value <= 2 && value >= 0)
			_decayValue = value;
	}

	int decayExponent() const
	{
		return _decayExponent;
	}

	void setDecayExponent(int value)
	{
		_decayExponent = value;
	}

	float openningAngle()
	{
		return _openningAngle;
	}

	void setOpeningAngle(float angle)
	{
		_openningAngle = angle;
	}

private:
  Type _type;
	int _decayValue; // spot  and point
	int _decayExponent; // spot
	float _openningAngle; // spot


}; // Light

} // end namespace cg

#endif // __Light_h
