#include "vertex.h"

kEn::buffer_layout vertex::vertex_layout = {
	{kEn::shader_data_types::float3, "a_Position"},
	{kEn::shader_data_types::float2, "a_TexCoord"}
};

std::unique_ptr<kEn::vertex_array> vertex::generate_vertex_buffer(int N, int M)
{
	const int stride = vertex_layout.stride() / sizeof(float);

	const auto vertices = new float[stride * N * M];
	const auto indices = new uint32_t[(N - 1) * (M - 1) * 6];

	int k = 0;
	for(int i = 0; i < M; ++i)
	{
		for(int j = 0; j < N; ++j)
		{
			const int index = i * N + j;
			const float x = (float)j / (float)(N - 1);
			const float y = (float)i / (float)(N - 1);

			const int offset = stride * index;
			vertices[offset]     = x-0.5f;    // x
			vertices[offset + 1] = y - 0.5f;    // y
			vertices[offset + 2] = 0.0f; // z
			vertices[offset + 3] = x;    // u
			vertices[offset + 4] = y;    // v

			if (i == M-1 || j == N - 1)
				continue;

			indices[k]     = index;
			indices[k + 1] = index + 1;
			indices[k + 2] = index + M;
			indices[k + 3] = index + 1;
			indices[k + 4] = index + 1 + M;
			indices[k + 5] = index + M;
			k += 6;
		}
	}

	auto vertex_array = kEn::vertex_array::create();
	auto vertex_buffer = kEn::mutable_vertex_buffer::create(vertices, vertex_layout.stride() * N * M);
	vertex_buffer->set_layout(vertex_layout);

	auto index_buffer = kEn::index_buffer::create(indices, (N - 1) * (M - 1) * 6);

	vertex_array->add_vertex_buffer(vertex_buffer);
	vertex_array->set_index_buffer(index_buffer);

	delete[] vertices;
	delete[] indices;

	return std::move(vertex_array);
}
