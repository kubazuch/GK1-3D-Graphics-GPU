#include "bezier_surface.h"

#include "glm/gtc/type_ptr.hpp"
#include "imgui/imgui.h"
#include "ImGuizmo/ImGuizmo.h"
#include "kEn/core/input.h"
#include "kEn/renderer/renderer.h"
#include "kEn/renderer/render_command.h"
#include "kEn/renderer/shader.h"

kEn::buffer_layout bezier_surface::bezier_layout = {
	{kEn::shader_data_types::int_, "a_Index"},
	{kEn::shader_data_types::float2, "a_TexCoord"}
};

bezier_surface::bezier_surface(int N, int M)
	: N_(N), M_(M), selected_point_(nullptr), control_point_model_("sphere.obj")
{
	kEn::framebuffer_spec spec;
	spec.width = 1280;
	spec.height = 720;
	spec.attachments = { kEn::framebuffer_texture_format::RGBA8, kEn::framebuffer_texture_format::RED_INT };
	framebuffer_ = kEn::framebuffer::create(spec);

	bezier_surface_shader_ = kEn::shader::create("surface", { false, true });
	bezier_surface_shader_->bind();
	bezier_surface_shader_->set_int("u_HDensity", horizontal_density);
	bezier_surface_shader_->set_int("u_VDensity", vertical_density);
	bezier_surface_shader_->set_bool("u_Geometry", false);

	bezier_normal_shader_ = kEn::shader::create("surface", { true, true });
	bezier_normal_shader_->bind();
	bezier_normal_shader_->set_int("u_HDensity", horizontal_density);
	bezier_normal_shader_->set_int("u_VDensity", vertical_density);
	bezier_normal_shader_->set_bool("u_Geometry", true);

	control_surface_shader_ = kEn::shader::create("control_surface", { false, true });
	control_surface_shader_->bind();
	control_surface_shader_->set_float4("u_Color", vertex::vertex_color);

	control_point_shader_ = kEn::shader::create("control_point");
	control_point_shader_->bind();
	control_point_shader_->set_float4("u_Color", vertex::vertex_color);

	bezier_surface_texture_ = kEn::texture2D::create("l.jpg");
	control_point_texture_ = kEn::texture2D::create("5h.jpg");

	for (int i = 0; i < N_; ++i)
	{
		std::vector<vertex*> row;
		for (int j = 0; j < M_; ++j)
		{
			auto v = new vertex(i, j);

			v->transform_.set_pos({ glm::mix(-0.5, 0.5, (float)i / 3.0f), .0f, glm::mix(-0.5, 0.5, (float)j / 3.0f) });
			v->transform_.set_parent(&transform_);
			v->transform_.set_scale(glm::vec3(0.015f));

			bezier_surface_shader_->bind();
			bezier_surface_shader_->set_float3("u_ControlPoints[" + std::to_string(i * M_ + j) + "]", v->transform_.pos());

			control_surface_shader_->bind();
			control_surface_shader_->set_float3("u_ControlPoints[" + std::to_string(i * M_ + j) + "]", v->transform_.pos());

			bezier_normal_shader_->bind();
			bezier_normal_shader_->set_float3("u_ControlPoints[" + std::to_string(i * M_ + j) + "]", v->transform_.pos());

			row.push_back(v);
		}

		control_points_.push_back(std::move(row));
	}

	selected_point_ = nullptr;

	generate_vertex_buffer();
}

bezier_surface::~bezier_surface()
{
	for (int i = 0; i < N_; ++i)
	{
		for (int j = 0; j < M_; ++j)
		{
			delete control_points_[i][j];
		}
	}
}

void bezier_surface::render() const
{
	bezier_surface_texture_->bind();

	// Draw bezier surface
	if (draw_wireframe)
		kEn::render_command::set_wireframe(true);
	kEn::renderer::submit_tessellated(*bezier_surface_shader_, *vertex_array_, 4 * N_ * M_, transform_);
	if (draw_wireframe)
		kEn::render_command::set_wireframe(false);

	// Draw bezier surface normals
	if(draw_normals)
		kEn::renderer::submit_tessellated(*bezier_normal_shader_, *vertex_array_, 4 * N_ * M_, transform_);

	// Draw control mesh
	if(draw_control_frame)
	{
		kEn::render_command::set_wireframe(true);
		kEn::renderer::submit_tessellated(*control_surface_shader_, *vertex_array_, 4 * N_ * M_, transform_);
		kEn::render_command::set_wireframe(false);
	}

	// Draw control points
	kEn::render_command::clear_depth();
	control_point_texture_->bind();
	for (int i = 0; i < N_; ++i)
	{
		for (int j = 0; j < M_; ++j)
		{
			kEn::renderer::submit(*control_point_shader_, *control_point_model_.vertex_array_, control_points_[i][j]->transform_);
		}
	}

	// Draw control points for mouse picking
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

void bezier_surface::vertex_moved(bool update) const
{
	if (update)
		selected_point_->transform_.model_matrix_updated();
	else
		selected_point_->transform_.set_pos(selected_point_->transform_.pos());

	bezier_surface_shader_->bind();
	bezier_surface_shader_->set_float3("u_ControlPoints[" + std::to_string(selected_point_->x * M_ + selected_point_->y) + "]", selected_point_->transform_.pos());
	control_surface_shader_->bind();
	control_surface_shader_->set_float3("u_ControlPoints[" + std::to_string(selected_point_->x * M_ + selected_point_->y) + "]", selected_point_->transform_.pos());
	bezier_normal_shader_->bind();
	bezier_normal_shader_->set_float3("u_ControlPoints[" + std::to_string(selected_point_->x * M_ + selected_point_->y) + "]", selected_point_->transform_.pos());
}

void bezier_surface::imgui(const kEn::camera& camera)
{
	ImGui::Begin("Bezier Surface");
	ImGui::Checkbox("Wireframe", &draw_wireframe);
	ImGui::Checkbox("Control grid", &draw_control_frame);
	ImGui::Checkbox("Normal vectors", &draw_normals);

	if (selected_point_ && ImGuizmo::Manipulate(glm::value_ptr(camera.view_matrix()), glm::value_ptr(camera.projection_matrix()), ImGuizmo::TRANSLATE, ImGuizmo::LOCAL, glm::value_ptr(selected_point_->transform_.local_to_world_matrix()), NULL, NULL, NULL, NULL))
	{
		vertex_moved(true);
	}

	if(ImGui::CollapsingHeader("Selection"))
	{
		if (selected_point_)
		{
			if(ImGui::InputFloat3("Pos", glm::value_ptr(selected_point_->transform_.pos())))
			{
				vertex_moved(false);
			}
		}
		else
		{
			ImGui::Text("Nothing");
		}
	}


	if (ImGui::CollapsingHeader("Tessellation"))
	{
		if (ImGui::SliderInt("Horizontal density", &horizontal_density, 1, 100))
		{
			bezier_surface_shader_->bind();
			bezier_surface_shader_->set_int("u_HDensity", horizontal_density);
			bezier_normal_shader_->bind();
			bezier_normal_shader_->set_int("u_HDensity", horizontal_density);
		}
		if (ImGui::SliderInt("Vertical density", &vertical_density, 1, 100))
		{
			bezier_surface_shader_->bind();
			bezier_surface_shader_->set_int("u_VDensity", vertical_density);
			bezier_normal_shader_->bind();
			bezier_normal_shader_->set_int("u_VDensity", vertical_density);
		}
	}

	ImGui::End();
}

bool bezier_surface::on_mouse_click(kEn::mouse_button_pressed_event& event)
{
	framebuffer_->bind();
	int pixelData = framebuffer_->read_pixel(1, kEn::input::get_mouse_x(), framebuffer_->get_spec().height - kEn::input::get_mouse_y());
	framebuffer_->unbind();
	if (pixelData < 0) {
		selected_point_ = nullptr;
		return false;
	}

	selected_point_ = control_points_[pixelData / M_][pixelData % M_];
	return false;
}

bool bezier_surface::on_window_resize(kEn::window_resize_event& event)
{
	framebuffer_->resize(event.width(), event.height());
	return false;
}

void bezier_surface::generate_vertex_buffer()
{
	struct vertex
	{
		int pos;
		float tx;
		float ty;
	};

	std::vector<vertex> data;

	for (int i = 0; i < N_ - 1; ++i)
	{
		for (int j = 0; j < M_ - 1; ++j)
		{
			glm::vec3 pos = control_points_[i][j]->transform_.pos();
			data.push_back({ i * M_ + j, 0.5f + pos.x , 0.5f + pos.z });

			pos = control_points_[i + 1][j]->transform_.pos();
			data.push_back({ (i + 1) * M_ + j, 0.5f + pos.x , 0.5f + pos.z });

			pos = control_points_[i][j + 1]->transform_.pos();
			data.push_back({ i * M_ + j + 1 , 0.5f + pos.x , 0.5f + pos.z });

			pos = control_points_[i + 1][j + 1]->transform_.pos();
			data.push_back({ (i + 1) * M_ + j + 1, 0.5f + pos.x , 0.5f + pos.z });
		}
	}

	vertex_array_ = kEn::vertex_array::create();
	auto vertex_buffer_ = kEn::vertex_buffer::create(reinterpret_cast<float*>(data.data()), data.size() * sizeof(vertex));
	vertex_buffer_->set_layout(bezier_layout);

	vertex_array_->add_vertex_buffer(vertex_buffer_);
}
