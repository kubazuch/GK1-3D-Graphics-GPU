#include "bezier_surface.h"

#include "glm/gtc/type_ptr.hpp"
#include "imgui/imgui.h"
#include "ImGuizmo/ImGuizmo.h"
#include "kEn/core/input.h"
#include "kEn/renderer/shader.h"
#include "kEn/scene/light.h"
#include "kEn/renderer/renderer.h"
#include "kEn/renderer/render_command.h"
#include "kEn/util/platform_utils.h"

kEn::buffer_layout bezier_surface::bezier_layout = {
	{kEn::shader_data_types::int_, "a_Index"},
	{kEn::shader_data_types::float2, "a_TexCoord"}
};

bezier_surface::bezier_surface(int N, int M)
	: N_(N), M_(M), selected_point_(nullptr), control_point_model_("sphere.obj")
{
	bezier_surface_shader_ = kEn::shader::create("surface", { false, true });
	bezier_surface_shader_->bind();
	bezier_surface_shader_->set_int("u_Texture", 0);
	bezier_surface_shader_->set_int("u_NormalTexture", 1);
	bezier_surface_shader_->set_int("u_AOTexture", 2);
	bezier_surface_shader_->set_int("u_HeightTexture", 3);
	bezier_surface_shader_->set_int("u_HDensity", horizontal_density);
	bezier_surface_shader_->set_int("u_VDensity", vertical_density);
	bezier_surface_shader_->set_material("u_Material", material_);

	bezier_normal_shader_ = kEn::shader::create("surface", { true, true });
	bezier_normal_shader_->bind();
	bezier_normal_shader_->set_int("u_HDensity", horizontal_density);
	bezier_normal_shader_->set_int("u_VDensity", vertical_density);
	bezier_normal_shader_->set_bool("u_Geometry", true);

	control_surface_shader_ = kEn::shader::create("control_surface", { false, true });
	control_surface_shader_->bind();
	control_surface_shader_->set_float3("u_Color", vertex::vertex_color);

	control_point_shader_ = kEn::shader::create("control_point");

	for (int i = 0; i < N_; ++i)
	{
		std::vector<vertex*> row;
		for (int j = 0; j < M_; ++j)
		{
			auto v = new vertex(i, j);

			row.push_back(v);
		}

		control_points_.push_back(std::move(row));
	}
	reset_grid();
	selected_point_ = nullptr;

	generate_vertex_buffer();
}

void bezier_surface::reset_grid()
{
	for (int i = 0; i < N_; ++i)
	{
		for (int j = 0; j < M_; ++j)
		{
			auto v = control_points_[i][j];

			v->transform_.set_local_pos({ glm::mix(-0.5, 0.5, (float)i / 3.0f), .0f, glm::mix(-0.5, 0.5, (float)j / 3.0f) });
			v->transform_.set_parent(transform_);
			v->transform_.set_local_scale(glm::vec3(0.015f));

			bezier_surface_shader_->bind();
			bezier_surface_shader_->set_float3("u_ControlPoints[" + std::to_string(i * M_ + j) + "]", v->transform_.local_pos());

			control_surface_shader_->bind();
			control_surface_shader_->set_float3("u_ControlPoints[" + std::to_string(i * M_ + j) + "]", v->transform_.local_pos());

			bezier_normal_shader_->bind();
			bezier_normal_shader_->set_float3("u_ControlPoints[" + std::to_string(i * M_ + j) + "]", v->transform_.local_pos());
		}
	}
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

void bezier_surface::render(const kEn::camera& cam, const kEn::point_light& light) const
{
	material_.bind();

	bezier_surface_shader_->bind();
	bezier_surface_shader_->set_float3("u_CameraPos", cam.transform().pos());
	bezier_surface_shader_->set_light("u_Light", light);

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

	// Draw light
	control_point_shader_->bind();
	control_point_shader_->set_float3("u_Color", light.color);
	kEn::renderer::submit(*control_point_shader_, *control_point_model_.vertex_array_, light.transform());

	// Draw control points
	control_point_shader_->set_float3("u_Color", vertex::vertex_color);
	kEn::render_command::depth_testing(false);
	for (int i = 0; i < N_; ++i)
	{
		for (int j = 0; j < M_; ++j)
		{
			kEn::renderer::submit(*control_point_shader_, *control_point_model_.vertex_array_, control_points_[i][j]->transform_);
		}
	}
	kEn::render_command::depth_testing(true);
}

void bezier_surface::render_mouse_pick(kEn::shader& shader) const
{
	for (int i = 0; i < N_; ++i)
	{
		for (int j = 0; j < M_; ++j)
		{
			shader.set_int("u_Id", i * M_ + j);
			kEn::renderer::submit(shader, *control_point_model_.vertex_array_, control_points_[i][j]->transform_);
		}
	}
}

void bezier_surface::vertex_moved(bool update) const
{
	if (update)
		selected_point_->transform_.model_matrix_updated();
	else
		selected_point_->transform_.set_local_pos(selected_point_->transform_.local_pos());

	bezier_surface_shader_->bind();
	bezier_surface_shader_->set_float3("u_ControlPoints[" + std::to_string(selected_point_->x * M_ + selected_point_->y) + "]", selected_point_->transform_.local_pos());
	control_surface_shader_->bind();
	control_surface_shader_->set_float3("u_ControlPoints[" + std::to_string(selected_point_->x * M_ + selected_point_->y) + "]", selected_point_->transform_.local_pos());
	bezier_normal_shader_->bind();
	bezier_normal_shader_->set_float3("u_ControlPoints[" + std::to_string(selected_point_->x * M_ + selected_point_->y) + "]", selected_point_->transform_.local_pos());
}

void bezier_surface::imgui(const kEn::camera& camera)
{
	ImGui::Checkbox("Wireframe", &draw_wireframe);
	ImGui::Checkbox("Control grid", &draw_control_frame);
	ImGui::Checkbox("Normal vectors", &draw_normals);
	if(ImGui::Button("Reset surface"))
	{
		reset_grid();
	}

	if (selected_point_ && ImGuizmo::Manipulate(glm::value_ptr(camera.view_matrix()), glm::value_ptr(camera.projection_matrix()), ImGuizmo::TRANSLATE, ImGuizmo::LOCAL, glm::value_ptr(selected_point_->transform_.local_to_world_matrix()), NULL, NULL, NULL, NULL))
	{
		vertex_moved(true);
	}


	if(ImGui::CollapsingHeader("Selection"))
	{
		if (selected_point_)
		{
			glm::vec3& k = selected_point_->transform_.local_pos();
			if(ImGui::DragFloat3("Pos##1", glm::value_ptr(k), 0.01f))
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

	bezier_surface_shader_->bind();
	if (ImGui::CollapsingHeader("Material"))
	{
		if (ImGui::TreeNode("Phong properties"))
		{
			if (ImGui::SliderFloat("ambient", &material_.ambient_factor, 0, 1))
			{
			}

			if (ImGui::SliderFloat("diffuse", &material_.diffuse_factor, 0, 1))
			{
			}

			if (ImGui::SliderFloat("specular", &material_.specular_factor, 0, 1))
			{
			}

			if (ImGui::SliderFloat("shininess ", &material_.shininess_factor, 1, 100))
			{
			}

			bezier_surface_shader_->set_material("u_Material", material_);

			ImGui::TreePop();
		}

		if (ImGui::TreeNode("Texture"))
		{
			static int texture_type = 0;
			if (ImGui::RadioButton("Solid", &texture_type, 0))
			{
				bezier_surface_shader_->bind();
				bezier_surface_shader_->set_bool("u_UseTexture", false);
			}
			ImGui::SameLine();
			if (ImGui::RadioButton("Image", &texture_type, 1))
			{
				bezier_surface_shader_->bind();
				bezier_surface_shader_->set_bool("u_UseTexture", true);
			}

			if (texture_type == 0)
			{
			}
			else
			{
				if(const auto texture = material_.texture(kEn::texture_type::diffuse))
					ImGui::Image((ImTextureID)texture->renderer_id(), ImVec2{ 250 * (float)texture->width() / (float)texture->height(), 250.f }, ImVec2{ 0.0f, 1.0f }, ImVec2{ 1.0f, 0.0f });
				if (ImGui::Button("Select"))
				{
					if (const std::filesystem::path path = kEn::file_dialog::open_image_file(); !path.empty())
					{
						material_.set_texture(kEn::texture_type::diffuse, kEn::texture2D::create(path));
					}
				}
			}

			ImGui::TreePop();
		}
		
		if (ImGui::TreeNode("Normal map"))
		{
			static int normal_map_type = 0;
			if (ImGui::RadioButton("None", &normal_map_type, 0))
			{
				bezier_surface_shader_->bind();
				bezier_surface_shader_->set_int("u_NormalMode", normal_map_type);
			}
			ImGui::SameLine();
			if (ImGui::RadioButton("Replacing", &normal_map_type, 1))
			{
				bezier_surface_shader_->bind();
				bezier_surface_shader_->set_int("u_NormalMode", normal_map_type);
			}
			ImGui::SameLine();
			if (ImGui::RadioButton("Adding", &normal_map_type, 2))
			{
				bezier_surface_shader_->bind();
				bezier_surface_shader_->set_int("u_NormalMode", normal_map_type);
			}			

			if (normal_map_type)
			{
				if (const auto normal_texture = material_.texture(kEn::texture_type::normal))
					ImGui::Image((ImTextureID)normal_texture->renderer_id(), ImVec2{ 250 * (float)normal_texture->width() / (float)normal_texture->height(), 250.f }, ImVec2{ 0.0f, 1.0f }, ImVec2{ 1.0f, 0.0f });
				if (ImGui::Button("Select"))
				{
					if (const std::filesystem::path path = kEn::file_dialog::open_image_file(); !path.empty())
					{
						material_.set_texture(kEn::texture_type::normal, kEn::texture2D::create(path));
					}
				}

				static bool invert_normal_y = false;
				if(ImGui::Checkbox("Invert Y", &invert_normal_y))
				{
					bezier_surface_shader_->bind();
					bezier_surface_shader_->set_bool("u_InvertNormalY", invert_normal_y);
				}
			}

			ImGui::TreePop();
		}

		if (ImGui::TreeNode("AO"))
		{
			static int texture_type = 0;
			if (ImGui::RadioButton("None", &texture_type, 0))
			{
				bezier_surface_shader_->bind();
				bezier_surface_shader_->set_bool("u_UseAO", false);
			}
			ImGui::SameLine();
			if (ImGui::RadioButton("Image", &texture_type, 1))
			{
				bezier_surface_shader_->bind();
				bezier_surface_shader_->set_bool("u_UseAO", true);
			}

			if (texture_type == 1)
			{
				if (const auto texture = material_.texture(kEn::texture_type::ambient_occlusion))
					ImGui::Image((ImTextureID)texture->renderer_id(), ImVec2{ 250 * (float)texture->width() / (float)texture->height(), 250.f }, ImVec2{ 0.0f, 1.0f }, ImVec2{ 1.0f, 0.0f });
				if (ImGui::Button("Select"))
				{
					if (const std::filesystem::path path = kEn::file_dialog::open_image_file(); !path.empty())
					{
						material_.set_texture(kEn::texture_type::ambient_occlusion, kEn::texture2D::create(path));
					}
				}
			}

			ImGui::TreePop();
		}

		if (ImGui::TreeNode("Height"))
		{
			static int texture_type = 0;
			if (ImGui::RadioButton("None", &texture_type, 0))
			{
				bezier_surface_shader_->bind();
				bezier_surface_shader_->set_bool("u_UseHeight", false);
			}
			ImGui::SameLine();
			if (ImGui::RadioButton("Image", &texture_type, 1))
			{
				bezier_surface_shader_->bind();
				bezier_surface_shader_->set_bool("u_UseHeight", true);
			}

			if (texture_type == 1)
			{
				if (const auto texture = material_.texture(kEn::texture_type::height))
					ImGui::Image((ImTextureID)texture->renderer_id(), ImVec2{ 250 * (float)texture->width() / (float)texture->height(), 250.f }, ImVec2{ 0.0f, 1.0f }, ImVec2{ 1.0f, 0.0f });
				if (ImGui::Button("Select"))
				{
					if (const std::filesystem::path path = kEn::file_dialog::open_image_file(); !path.empty())
					{
						material_.set_texture(kEn::texture_type::height, kEn::texture2D::create(path));
					}
				}
			}

			static float height_mod = 1.0f / 50.f;
			if(ImGui::SliderFloat("Multiplier", &height_mod, 0, 1))
			{
				bezier_surface_shader_->bind();
				bezier_surface_shader_->set_float("u_HeightMod", height_mod);
			}

			ImGui::TreePop();
		}
	}
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
			glm::vec3 pos = control_points_[i][j]->transform_.local_pos();
			data.push_back({ i * M_ + j, 0.5f + pos.x , 0.5f + pos.z });

			pos = control_points_[i + 1][j]->transform_.local_pos();
			data.push_back({ (i + 1) * M_ + j, 0.5f + pos.x , 0.5f + pos.z });

			pos = control_points_[i][j + 1]->transform_.local_pos();
			data.push_back({ i * M_ + j + 1 , 0.5f + pos.x , 0.5f + pos.z });

			pos = control_points_[i + 1][j + 1]->transform_.local_pos();
			data.push_back({ (i + 1) * M_ + j + 1, 0.5f + pos.x , 0.5f + pos.z });
		}
	}

	vertex_array_ = kEn::vertex_array::create();
	auto vertex_buffer_ = kEn::vertex_buffer::create(reinterpret_cast<float*>(data.data()), data.size() * sizeof(vertex));
	vertex_buffer_->set_layout(bezier_layout);

	vertex_array_->add_vertex_buffer(vertex_buffer_);
}
