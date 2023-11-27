#include "sphere.h"

#include "glm/gtc/type_ptr.hpp"
#include "imgui/imgui.h"
#include "kEn/renderer/renderer.h"
#include "kEn/renderer/render_command.h"
#include "kEn/renderer/shader.h"
#include "kEn/util/platform_utils.h"

kEn::buffer_layout sphere::sphere_layout = {
	{kEn::shader_data_types::float2, "a_TexCoord"}
};

sphere::sphere()
{
	surface_shader_ = kEn::shader::create("sphere", { false, true });
	surface_shader_->bind();
	surface_shader_->set_int("u_Texture", 0);
	surface_shader_->set_int("u_NormalTexture", 1);
	surface_shader_->set_int("u_AOTexture", 2);
	surface_shader_->set_int("u_HeightTexture", 3);
	surface_shader_->set_int("u_HDensity", horizontal_density);
	surface_shader_->set_int("u_VDensity", vertical_density);
	surface_shader_->set_material("u_Material", material_);
	
	generate_vertex_buffer();
}

void sphere::render(const kEn::camera& cam, const kEn::point_light& light) const
{
	material_.bind();

	surface_shader_->bind();
	surface_shader_->set_float3("u_CameraPos", cam.get_transform().transformed_pos());
	surface_shader_->set_light("u_Light", light);


	if (draw_wireframe)
		kEn::render_command::set_wireframe(true);
	kEn::renderer::submit_tessellated(*surface_shader_, *vertex_array_, 4 * 5 * 5, transform_);
	if (draw_wireframe)
		kEn::render_command::set_wireframe(false);
}

void sphere::imgui(const kEn::camera&)
{
	ImGui::Checkbox("Wireframe", &draw_wireframe);

	if (ImGui::CollapsingHeader("Tessellation"))
	{
		if (ImGui::SliderInt("Horizontal density", &horizontal_density, 2, 100))
		{
			surface_shader_->bind();
			surface_shader_->set_int("u_HDensity", horizontal_density);
		}
		if (ImGui::SliderInt("Vertical density", &vertical_density, 2, 100))
		{
			surface_shader_->bind();
			surface_shader_->set_int("u_VDensity", vertical_density);
		}
	}

	surface_shader_->bind();
	if (ImGui::CollapsingHeader("Material"))
	{
		if (ImGui::TreeNode("Phong properties"))
		{
			if (ImGui::SliderFloat("ambient", &material_.ambient_factor.get(), 0, 1))
			{
				material_.ambient_factor.set_dirty();
			}

			if (ImGui::SliderFloat("diffuse", &material_.diffuse_factor.get(), 0, 1))
			{
				material_.diffuse_factor.set_dirty();
			}

			if (ImGui::SliderFloat("specular", &material_.specular_factor.get(), 0, 1))
			{
				material_.specular_factor.set_dirty();
			}

			if (ImGui::SliderFloat("shininess ", &material_.shininess_factor.get(), 1, 100))
			{
				material_.shininess_factor.set_dirty();
			}

			surface_shader_->set_material("u_Material", material_);

			ImGui::TreePop();
		}

		if (ImGui::TreeNode("Texture"))
		{
			static int texture_type = 0;
			if (ImGui::RadioButton("Solid", &texture_type, 0))
			{
				surface_shader_->bind();
				surface_shader_->set_bool("u_UseTexture", false);
			}
			ImGui::SameLine();
			if (ImGui::RadioButton("Image", &texture_type, 1))
			{
				surface_shader_->bind();
				surface_shader_->set_bool("u_UseTexture", true);
			}

			if (texture_type == 0)
			{
				if (ImGui::ColorEdit3("Color##1", glm::value_ptr(material_.color.get())))
				{
					surface_shader_->bind();
					surface_shader_->set_float3("u_Material.color", material_.color);
				}
			}
			else
			{
				if (const auto texture = material_.texture())
					ImGui::Image((ImTextureID)texture->renderer_id(), ImVec2{ 250 * (float)texture->width() / (float)texture->height(), 250.f }, ImVec2{ 0.0f, 1.0f }, ImVec2{ 1.0f, 0.0f });
				if (ImGui::Button("Select"))
				{
					if (const std::filesystem::path path = kEn::file_dialog::open_image_file(); !path.empty())
					{
						material_.set_texture(kEn::texture2D::create(path));
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
				surface_shader_->bind();
				surface_shader_->set_int("u_NormalMode", normal_map_type);
			}
			ImGui::SameLine();
			if (ImGui::RadioButton("Replacing", &normal_map_type, 1))
			{
				surface_shader_->bind();
				surface_shader_->set_int("u_NormalMode", normal_map_type);
			}
			ImGui::SameLine();
			if (ImGui::RadioButton("Adding", &normal_map_type, 2))
			{
				surface_shader_->bind();
				surface_shader_->set_int("u_NormalMode", normal_map_type);
			}

			if (normal_map_type)
			{
				if (const auto normal_texture = material_.normal_texture())
					ImGui::Image((ImTextureID)normal_texture->renderer_id(), ImVec2{ 250 * (float)normal_texture->width() / (float)normal_texture->height(), 250.f }, ImVec2{ 0.0f, 1.0f }, ImVec2{ 1.0f, 0.0f });
				if (ImGui::Button("Select"))
				{
					if (const std::filesystem::path path = kEn::file_dialog::open_image_file(); !path.empty())
					{
						material_.set_normal_texture(kEn::texture2D::create(path));
					}
				}

				static bool invert_normal_y = false;
				if (ImGui::Checkbox("Invert Y", &invert_normal_y))
				{
					surface_shader_->bind();
					surface_shader_->set_bool("u_InvertNormalY", invert_normal_y);
				}
			}

			ImGui::TreePop();
		}

		if (ImGui::TreeNode("AO"))
		{
			static int texture_type = 0;
			if (ImGui::RadioButton("None", &texture_type, 0))
			{
				surface_shader_->bind();
				surface_shader_->set_bool("u_UseAO", false);
			}
			ImGui::SameLine();
			if (ImGui::RadioButton("Image", &texture_type, 1))
			{
				surface_shader_->bind();
				surface_shader_->set_bool("u_UseAO", true);
			}

			if (texture_type == 1)
			{
				if (const auto texture = material_.ao_texture())
					ImGui::Image((ImTextureID)texture->renderer_id(), ImVec2{ 250 * (float)texture->width() / (float)texture->height(), 250.f }, ImVec2{ 0.0f, 1.0f }, ImVec2{ 1.0f, 0.0f });
				if (ImGui::Button("Select"))
				{
					if (const std::filesystem::path path = kEn::file_dialog::open_image_file(); !path.empty())
					{
						material_.set_ao_texture(kEn::texture2D::create(path));
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
				surface_shader_->bind();
				surface_shader_->set_bool("u_UseHeight", false);
			}
			ImGui::SameLine();
			if (ImGui::RadioButton("Image", &texture_type, 1))
			{
				surface_shader_->bind();
				surface_shader_->set_bool("u_UseHeight", true);
			}

			if (texture_type == 1)
			{
				if (const auto texture = material_.height_texture())
					ImGui::Image((ImTextureID)texture->renderer_id(), ImVec2{ 250 * (float)texture->width() / (float)texture->height(), 250.f }, ImVec2{ 0.0f, 1.0f }, ImVec2{ 1.0f, 0.0f });
				if (ImGui::Button("Select"))
				{
					if (const std::filesystem::path path = kEn::file_dialog::open_image_file(); !path.empty())
					{
						material_.set_height_texture(kEn::texture2D::create(path));
					}
				}
			}

			static float height_mod = 1.0f / 50.f;
			if (ImGui::SliderFloat("Multiplier", &height_mod, 0, 1))
			{
				surface_shader_->bind();
				surface_shader_->set_float("u_HeightMod", height_mod);
			}

			ImGui::TreePop();
		}
	}
}

void sphere::generate_vertex_buffer()
{
	struct vertex
	{
		float tx;
		float ty;
	};

	vertex points_[5][5];
	for (int i = 0; i < 5; ++i)
	{
		std::vector<vertex> row;
		for (int j = 0; j < 5; ++j)
		{
			points_[i][j] = { (float)i / 4.0f * 2 * glm::pi<float>(), ((float)j / 4.0f - 0.5f) * glm::pi<float>() };
		}
	}

	std::vector<vertex> data;

	for (int i = 0; i < 5 - 1; ++i)
	{
		for (int j = 0; j < 5 - 1; ++j)
		{
			data.push_back(points_[i][j]);
			data.push_back(points_[i + 1][j]);
			data.push_back(points_[i][j + 1]);
			data.push_back(points_[i + 1][j + 1]);
		}
	}

	vertex_array_ = kEn::vertex_array::create();
	auto vertex_buffer_ = kEn::vertex_buffer::create(reinterpret_cast<float*>(data.data()), data.size() * sizeof(vertex));
	vertex_buffer_->set_layout(sphere_layout);

	vertex_array_->add_vertex_buffer(vertex_buffer_);
}
