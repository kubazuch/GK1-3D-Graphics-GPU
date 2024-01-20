#pragma once

#include "vertex.h"
#include "kEn/scene/camera/camera.h"
#include "kEn/event/mouse_events.h"
#include "kEn/renderer/framebuffer.h"
#include "kEn/scene/light.h"
#include "kEn/renderer/shader.h"
#include "kEn/renderer/texture.h"
#include "kEn/renderer/vertex_array.h"
#include "kEn/scene/mesh/obj_model.h"

class bezier_surface
{
public:
	bezier_surface(int N = 4, int M = 4);
	~bezier_surface();

	kEn::transform& transform() { return transform_; }
	vertex* selected_point() const { return selected_point_; }

	void render(const kEn::camera& cam, const kEn::point_light& light) const;
	void render_mouse_pick(kEn::shader& shader) const;
	void vertex_moved(bool update) const;
	void imgui(const kEn::camera&);

	const kEn::obj_model& get_control_point_model() const { return control_point_model_; }
	void set_ambient(const glm::vec3& color) const { bezier_surface_shader_->bind(); bezier_surface_shader_->set_float3("u_Ambient", color); }

private:
	void reset_grid();

public:
	bool draw_wireframe = false, draw_control_frame = true, draw_normals = false;

private:
	int N_, M_;
	int horizontal_density = 3, vertical_density = 3;
	kEn::material material_;

	std::unique_ptr<kEn::vertex_array> vertex_array_;
	std::vector<std::vector<vertex*>> control_points_;
	vertex* selected_point_;
	kEn::transform transform_;

	std::shared_ptr<kEn::shader> bezier_surface_shader_, control_point_shader_, control_surface_shader_, bezier_normal_shader_;
	kEn::obj_model control_point_model_;

	friend class main_layer;
private:
	static kEn::buffer_layout bezier_layout;

	void generate_vertex_buffer();
};
