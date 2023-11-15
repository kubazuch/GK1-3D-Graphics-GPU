#pragma once

#include "vertex.h"
#include "kEn/event/mouse_events.h"
#include "kEn/renderer/texture.h"
#include "kEn/renderer/vertex_array.h"
#include "kEn/renderer/mesh/obj_model.h"

namespace kEn
{
	class shader;
}

class bezier_plane
{
public:
	bezier_plane(int N = 4, int M = 4);
	~bezier_plane();

	kEn::transform& transform() { return transform_; }
	vertex* selected_point() const { return selected_point_; }
	bool on_mouse_click(kEn::mouse_button_pressed_event& event);

	void render() const;
	void vertex_moved() const;

	bool draw_wireframe = false;
private:
	int N_, M_;
	std::unique_ptr<kEn::vertex_array> vertex_array_;
	std::shared_ptr<kEn::mutable_vertex_buffer> vertex_buffer_;
	std::vector<std::vector<vertex*>> control_points_;
	vertex* selected_point_;
	kEn::transform transform_;

	std::shared_ptr<kEn::shader> bezier_surface_shader_, control_point_shader_;
	std::shared_ptr<kEn::texture> bezier_surface_texture_, control_point_texture_;
	kEn::obj_model control_point_model_;

private:
	static kEn::buffer_layout bezier_layout;

	void generate_vertex_buffer();
};
