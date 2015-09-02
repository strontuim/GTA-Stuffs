/*
	THIS FILE IS A PART OF GTA V SCRIPT HOOK SDK
				http://dev-c.com
			(C) Alexander Blade 2015
*/

#pragma once

#include "..\inc\natives.h"
#include "..\inc\types.h"
#include "..\inc\enums.h"

#include "..\inc\main.h"
#include "keyboard.h"


#include <string>
#include <ctime>
#include <sstream>
#include <vector>

#define ABS(x) x > 0 ? x : -x

void				ScriptMain();
namespace Utilities
{
	void				notify(const std::string &str);
	std::string			floatToString(float f);
	template<typename T>
	std::string				xToString(T x)
	{
		std::ostringstream	ss;
		ss << x;
		std::string str = ss.str();
		return (str);
	}
	Vector3				getPlayerCoords(void);
	void				drawRectangle(float x1, float y1, float x2, float y2, Vector3 color, float alpha);
	void				putText(const std::string &text, float x, float y, float size = 0.25f);
	DWORD				get_hash(const std::string &str);
	Object				create_object(const std::string &name, Vector3 pos);
}

typedef struct
{
	uint8_t		mem_a[14], mem_b[15];
	intptr_t	a, b;
} memory_data;