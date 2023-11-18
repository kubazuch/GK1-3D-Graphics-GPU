#pragma once

#include "vertex.h"
#include "kEn/camera/camera.h"
#include "kEn/event/mouse_events.h"
#include "kEn/renderer/framebuffer.h"
#include "kEn/renderer/shader.h"
#include "kEn/renderer/texture.h"
#include "kEn/renderer/vertex_array.h"
#include "kEn/renderer/mesh/obj_model.h"

class bezier_surface
{
public:
	bezier_surface(int N = 4, int M = 4);
	~bezier_surface();

	kEn::transform& transform() { return transform_; }
	vertex* selected_point() const { return selected_point_; }
	bool on_mouse_click(kEn::mouse_button_pressed_event& event);

	void render() const;
	void vertex_moved(bool update) const;
	void imgui(const kEn::camera&);

	bool on_window_resize(kEn::window_resize_event& event);

	bool draw_wireframe = false, draw_control_frame = true, draw_normals = false;
private:
	int N_, M_;
	int horizontal_density = 3, vertical_density = 3;
	std::unique_ptr<kEn::vertex_array> vertex_array_;
	std::vector<std::vector<vertex*>> control_points_;
	vertex* selected_point_;
	kEn::transform transform_;
	std::shared_ptr<kEn::framebuffer> framebuffer_;

	std::unique_ptr<kEn::shader> bezier_surface_shader_, control_point_shader_, control_surface_shader_, bezier_normal_shader_;
	std::shared_ptr<kEn::texture> bezier_surface_texture_, control_point_texture_;
	kEn::obj_model control_point_model_;

private:
	static kEn::buffer_layout bezier_layout;

	void generate_vertex_buffer();
};
