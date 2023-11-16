#include "bezier_surface.h"

#include "glm/gtc/type_ptr.hpp"
#include "imgui/imgui.h"
#include "ImGuizmo/ImGuizmo.h"
#include "kEn/core/input.h"
#include "kEn/renderer/renderer.h"
#include "kEn/renderer/render_command.h"
#include "kEn/renderer/shader.h"

kEn::buffer_layout bezier_surface::bezier_layout = {
	{kEn::shader_data_types::float3, "a_Position"},
	{kEn::shader_data_types::float2, "a_TexCoord"}
};

bezier_surface::bezier_surface(int N, int M)
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
			auto v = new vertex(i, j);

			v->transform_.set_pos({ glm::mix(-0.5, 0.5, (float)i / 3.0f), .0f, glm::mix(-0.5, 0.5, (float)j / 3.0f) });
			v->transform_.set_parent(&transform_);
			v->transform_.set_scale(glm::vec3(0.015f));

			row.push_back(v);
		}

		control_points_.push_back(std::move(row));
	}

	selected_point_ = nullptr;

	kEn::framebuffer_spec spec;
	spec.width = 1280;
	spec.height = 720;
	spec.attachments = { kEn::framebuffer_texture_format::RGBA8, kEn::framebuffer_texture_format::RED_INT };
	framebuffer_ = kEn::framebuffer::create(spec);

	generate_vertex_buffer();
}

bezier_surface::~bezier_surface()
{
	for(int i = 0; i < N_; ++i)
	{
		for(int j = 0; j < M_; ++j)
		{
			delete control_points_[i][j];
		}
	}
}

void bezier_surface::render() const
{
	bezier_surface_texture_->bind();

	if (draw_wireframe)
		kEn::render_command::set_wireframe(true);
	kEn::renderer::submit(*bezier_surface_shader_, *vertex_array_, transform_);
	if (draw_wireframe)
		kEn::render_command::set_wireframe(false);

	control_point_texture_->bind();
	for(int i = 0; i < N_; ++i)
	{
		for(int j = 0; j < M_; ++j)
		{
			kEn::renderer::submit(*control_point_shader_, *control_point_model_.vertex_array_, control_points_[i][j]->transform_);
		}
	}

	framebuffer_->bind();
	framebuffer_->clear_attachment(0, 0);
	framebuffer_->clear_attachment(1, -1);
	for (int i = 0; i < N_; ++i)
	{
		for (int j = 0; j < M_; ++j)
		{
			control_point_shader_->set_int("u_Id", i * M_ + j);
			kEn::renderer::submit(*control_point_shader_, *control_point_model_.vertex_array_, control_points_[i][j]->transform_);
		}
	}
	framebuffer_->unbind();
}

void bezier_surface::vertex_moved() const
{
	selected_point_->transform_.model_matrix_updated();
	vertex_buffer_->modify_data([this](void* buffer) {
		auto vertices = static_cast<float*>(buffer);
		int offset = 5*(selected_point_->x * M_ + selected_point_->y);
		vertices[offset + 0] = selected_point_->transform_.local_to_parent_matrix()[3][0];
		vertices[offset + 1] = selected_point_->transform_.local_to_parent_matrix()[3][1];
		vertices[offset + 2] = selected_point_->transform_.local_to_parent_matrix()[3][2];
	});
}

void bezier_surface::imgui(const kEn::camera& camera)
{
	ImGui::Begin("Surface");
	if (selected_point_)
	{
		if (ImGuizmo::Manipulate(glm::value_ptr(camera.view_matrix()), glm::value_ptr(camera.projection_matrix()), ImGuizmo::TRANSLATE, ImGuizmo::LOCAL, glm::value_ptr(selected_point_->transform_.local_to_world_matrix()), NULL, NULL, NULL, NULL))
		{
			vertex_moved();
		}

		ImGui::InputFloat3("Tr", glm::value_ptr(selected_point_->transform_.pos()));
	}

	ImGui::Checkbox("Wireframe", &draw_wireframe);
	auto texture_id = framebuffer_->get_color_attachment_renderer_id();
	ImGui::Image((void*)texture_id, ImVec2{ 320.f, 180.f }, ImVec2{ 0,1 }, ImVec2{ 1,0 });

	ImGui::End();
}

bool bezier_surface::on_mouse_click(kEn::mouse_button_pressed_event& event)
{
	framebuffer_->bind();
	int pixelData = framebuffer_->read_pixel(1, kEn::input::get_mouse_x(), framebuffer_->get_spec().height - kEn::input::get_mouse_y());
	framebuffer_->unbind();
	if (pixelData == -1) //TODO: unselect, but first handle imgui events
		return false;

	selected_point_ = control_points_[pixelData / M_][pixelData % M_];
	return false;
}

void bezier_surface::generate_vertex_buffer()
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
