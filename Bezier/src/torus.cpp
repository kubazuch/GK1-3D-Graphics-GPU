#include "torus.h"

#include "glm/gtc/type_ptr.hpp"
#include "kEn/renderer/renderer.h"
#include "kEn/renderer/render_command.h"
#include "kEn/renderer/shader.h"
#include "kEn/util/platform_utils.h"

kEn::buffer_layout torus::torus_layout = {
	{kEn::shader_data_types::float2, "a_TexCoord"}
};

torus::torus()
{
	material_.set_texture(kEn::texture_type::diffuse, kEn::texture2D::create("TCom_Leather_Plain09_008x008_1K_albedo.png"));
	material_.diffuse_factor = 1;
	material_.specular_factor = 1;
	material_.shininess_factor = 3;

	generate_vertex_buffer();
}

std::shared_ptr<kEn::game_component> torus::clone() const
{
	return std::make_shared<torus>();
}

void torus::update(float delta)
{
}

void torus::render(kEn::shader& shader)
{
	if (!parent_.has_value()) return;

	shader.set_int("u_HDensity", horizontal_density);
	shader.set_int("u_VDensity", vertical_density);
	shader.set_float("u_MinorRadius", minor_radius);
	shader.set_material("u_Material", material_);

	material_.bind();

	if (draw_wireframe)
		kEn::render_command::set_wireframe(true);
	kEn::renderer::submit_tessellated(shader, *vertex_array_, 4 * 5 * 6, transform());
	if (draw_wireframe)
		kEn::render_command::set_wireframe(false);
}


void torus::imgui()
{
	ImGui::Checkbox("Wireframe", &draw_wireframe);
	ImGui::SliderFloat("Minor radius", &minor_radius, 0.1f, 1);

	if (ImGui::TreeNode("Tessellation"))
	{
		ImGui::SliderInt("Horizontal density", &horizontal_density, 2, 100);
		ImGui::SliderInt("Vertical density", &vertical_density, 2, 100);
		ImGui::TreePop();
	}

	if (ImGui::TreeNode("Material"))
	{
		material_.imgui();
		ImGui::TreePop();
	}
}


void torus::generate_vertex_buffer()
{
	struct vertex
	{
		float tx;
		float ty;
	};

	vertex points_[6][5];
	for (int i = 0; i < 6; ++i)
	{
		std::vector<vertex> row;
		for (int j = 0; j < 5; ++j)
		{
			points_[i][j] = { (float)i / 5.0f, (float)j / 4.0f };
		}
	}

	std::vector<vertex> data;

	for (int i = 0; i < 6 - 1; ++i)
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
	auto vertex_buffer_ = kEn::vertex_buffer::create(data.data(), data.size() * sizeof(vertex));
	vertex_buffer_->set_layout(torus_layout);

	vertex_array_->add_vertex_buffer(vertex_buffer_);
}
