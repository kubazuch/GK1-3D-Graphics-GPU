#pragma once

#include "vertex.h"
#include "kEn/camera/camera.h"
#include "kEn/event/mouse_events.h"
#include "kEn/renderer/framebuffer.h"
#include "kEn/renderer/shader.h"
#include "kEn/renderer/texture.h"
#include "kEn/renderer/vertex_array.h"
#include "kEn/renderer/mesh/obj_model.h"

struct material_phong
{
	float ka = 0.5f;
	float kd = 0.5f;
	float ks = 0.5f;
	float m = 50.f;

	glm::vec3 color{1.f};
};

struct light
{
	kEn::transform transform_;
	glm::vec3 color{1.f};
	bool selected = false;
};

class bezier_surface
{
public:
	bezier_surface(int N = 4, int M = 4);
	~bezier_surface();

	kEn::transform& transform() { return transform_; }
	vertex* selected_point() const { return selected_point_; }
	bool on_mouse_click(kEn::mouse_button_pressed_event& event);

	void render(const kEn::camera& cam) const;
	void vertex_moved(bool update) const;
	void imgui(const kEn::camera&);

	bool on_window_resize(kEn::window_resize_event& event);
	void update(double delta, double time);

private:
	void reset_grid();

public:
	bool draw_wireframe = false, draw_control_frame = true, draw_normals = false, animate_light = false;

private:
	int N_, M_;
	int horizontal_density = 3, vertical_density = 3;
	material_phong material_;
	light light_;
	glm::vec3 ambient_color_;

	std::unique_ptr<kEn::vertex_array> vertex_array_;
	std::vector<std::vector<vertex*>> control_points_;
	vertex* selected_point_;
	kEn::transform transform_;
	std::shared_ptr<kEn::framebuffer> framebuffer_;

	std::unique_ptr<kEn::shader> bezier_surface_shader_, control_point_shader_, control_surface_shader_, bezier_normal_shader_;
	std::shared_ptr<kEn::texture> bezier_surface_texture_, bezier_normal_texture_;
	kEn::obj_model control_point_model_;

private:
	static kEn::buffer_layout bezier_layout;

	void generate_vertex_buffer();
};
