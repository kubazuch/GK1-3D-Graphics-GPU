#pragma once

#include "kEn/renderer/vertex_array.h"

class vertex
{
private:

public:
	static kEn::buffer_layout vertex_layout;

	static std::unique_ptr<kEn::vertex_array> generate_vertex_buffer(int N = 4, int M = 4);
};
