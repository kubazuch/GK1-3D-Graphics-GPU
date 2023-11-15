#include "bezier_plane.h"

#include "kEn/core/input.h"
#include "kEn/renderer/renderer.h"
#include "kEn/renderer/render_command.h"
#include "kEn/renderer/shader.h"

kEn::buffer_layout bezier_plane::bezier_layout = {
	{kEn::shader_data_types::float3, "a_Position"},
	{kEn::shader_data_types::float2, "a_TexCoord"}
};

bezier_plane::bezier_plane(int N, int M)
	: N_(N), M_(M), selected_point_(nullptr), control_point_model_("sphere.obj")
{
	bezier_surface_shader_ = kEn::shader::create("test");
	control_point_shader_ = kEn::shader::create("control_point");
	control_point_shader_->bind();
	control_point_shader_->set_float4("u_Color", vertex::vertex_color);

	bezier_surface_texture_ = kEn::texture2D::create("l.jpg");
	control_point_texture_ = kEn::texture2D::create("5h.jpg");

	for (int i = 0; i < N_; ++i)
	{
		std::vector<vertex*> row;
		for(int j = 0; j < M_; ++j)
		{
			auto v = new vertex();

			v->transform_.set_pos({ glm::mix(-0.5, 0.5, (float)i / 3.0f), .0f, glm::mix(-0.5, 0.5, (float)j / 3.0f) });
			v->transform_.set_parent(&transform_);
			v->transform_.set_scale(glm::vec3(0.015f));

			row.push_back(v);
		}

		control_points_.push_back(std::move(row));
	}

	selected_point_ = control_points_[0][0];

	generate_vertex_buffer();
}

bezier_plane::~bezier_plane()
{
	for(int i = 0; i < N_; ++i)
	{
		for(int j = 0; j < M_; ++j)
		{
			delete control_points_[i][j];
		}
	}
}

void bezier_plane::render() const
{
	bezier_surface_texture_->bind();

	if (draw_wireframe)
		kEn::render_command::set_wireframe(true);
	kEn::renderer::submit(bezier_surface_shader_, *vertex_array_, transform_);
	if (draw_wireframe)
		kEn::render_command::set_wireframe(false);

	control_point_texture_->bind();
	for(int i = 0; i < N_; ++i)
	{
		for(int j = 0; j < M_; ++j)
		{
			kEn::renderer::submit(control_point_shader_, *control_point_model_.vertex_array_, control_points_[i][j]->transform_);
		}
	}
}

void bezier_plane::vertex_moved() const
{
	selected_point_->transform_.model_matrix_updated();
	vertex_buffer_->modify_data([this](void* buffer) {
		auto vertices = static_cast<float*>(buffer);
		vertices[0] = selected_point_->transform_.local_to_parent_matrix()[3][0];
		vertices[1] = selected_point_->transform_.local_to_parent_matrix()[3][1];
		vertices[2] = selected_point_->transform_.local_to_parent_matrix()[3][2];
	});
}

bool bezier_plane::on_mouse_click(kEn::mouse_button_pressed_event& event)
{
	KEN_INFO("{0}", glm::to_string(kEn::input::get_mouse_pos()));
	return false;
}

void bezier_plane::generate_vertex_buffer()
{
	const int stride = bezier_layout.stride() / sizeof(float);

	const auto vertices = new float[stride * N_ * M_];
	const auto indices = new uint32_t[(N_ - 1) * (M_ - 1) * 6];

	int k = 0;
	for (int i = 0; i < N_; ++i)
	{
		for (int j = 0; j < M_; ++j)
		{
			const int index = i * N_ + j;
			const glm::vec3 pos = control_points_[i][j]->transform_.pos();

			const int offset = stride * index;
			vertices[offset + 0] = pos.x;   // x
			vertices[offset + 1] = pos.y;   // y
			vertices[offset + 2] = pos.z;   // z
			vertices[offset + 3] = 0.5f - pos.x;   // u
			vertices[offset + 4] = 0.5f - pos.z;   // v

			if (i == M_ - 1 || j == N_ - 1)
				continue;

			indices[k + 0] = index;
			indices[k + 1] = index + 1;
			indices[k + 2] = index + M_;
			indices[k + 3] = index + 1;
			indices[k + 4] = index + 1 + M_;
			indices[k + 5] = index + M_;
			k += 6;
		}
	}

	vertex_array_ = kEn::vertex_array::create();
	vertex_buffer_ = kEn::mutable_vertex_buffer::create(vertices, bezier_layout.stride() * N_ * M_);
	vertex_buffer_->set_layout(bezier_layout);

	auto index_buffer = kEn::index_buffer::create(indices, (N_ - 1) * (M_ - 1) * 6);

	vertex_array_->add_vertex_buffer(vertex_buffer_);
	vertex_array_->set_index_buffer(index_buffer);

	delete[] vertices;
	delete[] indices;
}
