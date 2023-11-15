#pragma once

#include <ostream>

#include "glm/gtx/string_cast.hpp"
#include "kEn/core/transform.h"

struct vertex
{
	vertex() : selected_(false) {}
	vertex(const vertex&)
	{
		selected_ = false;
	}

	friend std::ostream& operator<<(std::ostream& os, const vertex& obj)
	{
		return os
			<< "transform_: " << glm::to_string(obj.transform_.local_to_world_matrix())
			<< " selected_: " << obj.selected_;
	}

	kEn::transform transform_;
	bool selected_;

public:
	static glm::vec4 vertex_color;
};
