#pragma once

#include <ostream>

#include "glm/gtx/string_cast.hpp"
#include "kEn/core/transform.h"

struct vertex
{
	vertex(int x, int y) : selected_(false), x(x), y(y) {}

	friend std::ostream& operator<<(std::ostream& os, const vertex& obj)
	{
		return os
			<< "transform_: " << glm::to_string(obj.transform_.local_to_world_matrix())
			<< " selected_: " << obj.selected_;
	}

	kEn::transform transform_;
	bool selected_;
	int x, y;

public:
	static glm::vec3 vertex_color;
};
