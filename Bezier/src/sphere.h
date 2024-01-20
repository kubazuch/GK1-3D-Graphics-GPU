#pragma once

#include "imgui/imgui.h"
#include "ImGuizmo/ImGuizmo.h"

#include "kEn/scene/camera/camera.h"
#include "kEn/core/transform.h"
#include "kEn/scene/light.h"
#include "kEn/renderer/material.h"
#include "kEn/renderer/shader.h"
#include "kEn/scene/mesh/obj_model.h"

namespace kEn
{
	class vertex_array;
}

class sphere
{
public:
	sphere();

	kEn::transform& transform() { return transform_; }

	void render(const kEn::camera& cam, const kEn::point_light& light) const;
	void imgui(const kEn::camera&);

	void set_ambient(const glm::vec3& color) const { surface_shader_->bind(); surface_shader_->set_float3("u_Ambient", color); }

public:
	bool draw_wireframe = false, draw_normals = false, selected = false;

private:
	int horizontal_density = 3, vertical_density = 3;
	kEn::material material_;

	std::unique_ptr<kEn::vertex_array> vertex_array_;
	kEn::transform transform_;

	std::shared_ptr<kEn::shader> surface_shader_, normal_shader_;

	ImGuizmo::OPERATION operation_ = ImGuizmo::TRANSLATE;
	friend class main_layer;
private:
	static kEn::buffer_layout sphere_layout;

	void generate_vertex_buffer();
};
