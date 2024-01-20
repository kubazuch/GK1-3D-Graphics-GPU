#pragma once

#include "imgui/imgui.h"
#include "ImGuizmo/ImGuizmo.h"

#include "kEn/scene/camera/camera.h"
#include "kEn/core/transform.h"
#include "kEn/scene/light.h"
#include "kEn/renderer/material.h"
#include "kEn/renderer/shader.h"
#include "kEn/scene/mesh/obj_model.h"

class main_layer;

class torus : public kEn::game_component
{
public:
	torus();

	void imgui();

	[[nodiscard]] std::shared_ptr<game_component> clone() const override;
	void update(float delta) override;
	void render(kEn::shader& shader) override;

	bool draw_wireframe = false, draw_normals = false, selected = false;

private:
	int horizontal_density = 30, vertical_density = 30;
	float minor_radius = 0.5f;
	kEn::material material_;

	std::unique_ptr<kEn::vertex_array> vertex_array_;

	std::shared_ptr<kEn::shader> phong_, gouraud_, flat_;

	friend class main_layer;
private:
	static kEn::buffer_layout torus_layout;

	void generate_vertex_buffer();
};
