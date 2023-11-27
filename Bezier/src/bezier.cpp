#include <kEn.h>
#include <kEn/core/assert.h>

#include <imgui/imgui.h>

#include "bezier_surface.h"
#include "vertex.h"
#include "glm/gtc/type_ptr.hpp"
#include "imgui/imgui_internal.h"
#include "ImGuizmo/ImGuizmo.h"
#include "kEn/camera/camera.h"
#include "kEn/core/transform.h"
#include "kEn/event/key_events.h"
#include "kEn/event/mouse_events.h"
#include "kEn/renderer/light.h"
#include "kEn/renderer/texture.h"
#include "kEn/renderer/mesh/obj_model.h"

enum class camera_mode
{
	none = 0,
	free_look,
	orbit
};

class main_layer : public kEn::layer
{
public:
	main_layer() : layer("BezierLayer"), ambient_color_{ 0.0f, 0.01f, 0.22f }, window_center_(640, 360)
	{
		dispatcher_ = std::make_unique<kEn::event_dispatcher>();

		//camera_ = std::make_shared<kEn::orthographic_camera>(-16.f/9.f, 16.f / 9.f, -1.f, 1.f);
		camera_ = std::make_shared<kEn::perspective_camera>(glm::radians(70.f), 16.f / 9.f, 0.01f, 100.f);
		camera_->set_pos({ 0,0.5,1 });
		camera_->look_at({ 0,0,0 }, { 0,1,0 });
		camera_->set_parent(&camera_mount_);
		mode_ = camera_mode::none;

		kEn::framebuffer_spec spec;
		spec.width = 1280;
		spec.height = 720;
		spec.attachments = { kEn::framebuffer_texture_format::RGBA8, kEn::framebuffer_texture_format::RED_INT };
		framebuffer_ = kEn::framebuffer::create(spec);

		mouse_pick_shader_ = kEn::shader::create("control_point");

		light_.transform.set_pos({ 0, 0.5, 0 });
		light_.transform.set_scale(glm::vec3{0.02f});

		dispatcher_->subscribe<kEn::window_resize_event>(KEN_EVENT_SUBSCRIBER(camera_->on_window_resize));
		dispatcher_->subscribe<kEn::window_resize_event>(KEN_EVENT_SUBSCRIBER(on_window_resize));

		dispatcher_->subscribe<kEn::key_pressed_event>(KEN_EVENT_SUBSCRIBER(on_key_press));
		dispatcher_->subscribe<kEn::mouse_button_pressed_event>(KEN_EVENT_SUBSCRIBER(on_mouse_click));

		surface_.set_ambient(ambient_color_);
	}

	void on_update(double delta, double) override
	{

		if (kEn::input::is_key_pressed(kEn::key::escape))
		{
			kEn::input::set_cursor_visible(true);
			mode_ = camera_mode::none;
		}

		if (mode_ == camera_mode::free_look)
		{
			const float sensitivity = 0.1f;
			const float speed = 0.2f;
			glm::vec2 delta_pos = kEn::input::get_mouse_pos() - window_center_;
			bool rotY = delta_pos.x != 0;
			bool rotX = delta_pos.y != 0;


			if (rotY)
			{
				glm::quat rot({ 0, -glm::radians(delta_pos.x) * sensitivity, 0 });
				camera_->rotate(rot);
			}

			if(rotX)
			{
				glm::quat rot({ -glm::radians(delta_pos.y) * sensitivity, 0, 0 });
				camera_->rotate_local(rot);
			}

			if(rotX || rotY)
			{
				kEn::input::set_mouse_pos(window_center_);
			}

			float move_amount = kEn::input::is_key_pressed(kEn::key::left_control) ? 3.f * delta * speed : delta * speed;
			glm::vec3 direction{0.f};
			if (kEn::input::is_key_pressed(kEn::key::up) || kEn::input::is_key_pressed(kEn::key::w))
			{
				direction -= camera_->get_transform().local_front();
			}
			if (kEn::input::is_key_pressed(kEn::key::down) || kEn::input::is_key_pressed(kEn::key::s))
			{
				direction += camera_->get_transform().local_front();
			}
			if (kEn::input::is_key_pressed(kEn::key::left) || kEn::input::is_key_pressed(kEn::key::a))
			{
				direction -= camera_->get_transform().local_right();
			}
			if (kEn::input::is_key_pressed(kEn::key::right) || kEn::input::is_key_pressed(kEn::key::d))
			{
				direction += camera_->get_transform().local_right();
			}
			if (kEn::input::is_key_pressed(kEn::key::space) || kEn::input::is_key_pressed(kEn::key::q))
			{
				direction += glm::vec3(0, 1, 0);
			}
			if (kEn::input::is_key_pressed(kEn::key::left_shift) || kEn::input::is_key_pressed(kEn::key::e))
			{
				direction -= glm::vec3(0, 1, 0);
			}

			if (direction.x || direction.y || direction.z)
			{
				camera_->fma(glm::normalize(direction), move_amount);
			}
		}

		if (mode_ == camera_mode::orbit)
		{
			const float speed = 0.2f;
			glm::vec2 delta_pos = kEn::input::get_mouse_pos() - window_center_;

			if (delta_pos.x || delta_pos.y)
			{
				kEn::input::set_mouse_pos(window_center_);
			}

			float move_amount = kEn::input::is_key_pressed(kEn::key::left_control) ? 3.f * delta * speed : delta * speed;
			glm::vec3 direction{0.f};
			if (kEn::input::is_key_pressed(kEn::key::up) || kEn::input::is_key_pressed(kEn::key::w))
			{
				direction -= camera_->get_transform().local_front();
			}
			if (kEn::input::is_key_pressed(kEn::key::down) || kEn::input::is_key_pressed(kEn::key::s))
			{
				direction += camera_->get_transform().local_front();
			}
			if (kEn::input::is_key_pressed(kEn::key::space) || kEn::input::is_key_pressed(kEn::key::q))
			{
				direction += camera_->get_transform().local_up();
			}
			if (kEn::input::is_key_pressed(kEn::key::left_shift) || kEn::input::is_key_pressed(kEn::key::e))
			{
				direction -= camera_->get_transform().local_up();
			}

			if (direction.x || direction.y || direction.z)
			{
				camera_->fma(glm::normalize(direction), move_amount);
				camera_->look_at(camera_mount_.pos(), { 0,1,0 });
			}

			camera_mount_.rotate({ 0,1,0 }, move_amount);
			camera_->recalculate_view();
		}

		const float speed = 0.1f;
		static float time = 0.0f;
		if (animate_light)
		{
			time += speed * delta;
			glm::vec3 new_pos(glm::cos(13 * time), 0, glm::sin(13 * time));
		
			new_pos *= 0.5f * glm::sin(time);
			new_pos.y = light_.transform.pos().get().y;
		
			light_.transform.set_pos(new_pos);
		}
	}

	void on_render() override
	{
		kEn::render_command::set_clear_color({ ambient_color_, 1.0f });
		kEn::render_command::clear();

		kEn::renderer::begin_scene(camera_);
		{
			surface_.render(*camera_, light_);

			// Draw control points for mouse picking
			framebuffer_->bind();
			framebuffer_->clear_attachment(0, 0);
			framebuffer_->clear_attachment(1, -1);

			mouse_pick_shader_->bind();
			mouse_pick_shader_->set_int("u_Id", 113);
			kEn::renderer::submit(*mouse_pick_shader_, *surface_.get_control_point_model().vertex_array_, light_.transform);

			surface_.render_mouse_pick(*mouse_pick_shader_);

			framebuffer_->unbind();
		}
		kEn::renderer::end_scene();
	}

	void on_attach() override
	{
		kEn::render_command::set_tessellation_patch_vertices(4);
		KEN_DEBUG("Attached!");
	}

	void on_detach() override
	{
		KEN_DEBUG("Detached!");
	}

	void on_imgui() override
	{
		ImGuiIO& io = ImGui::GetIO();
		ImGuizmo::SetRect(0, 0, io.DisplaySize.x, io.DisplaySize.y);

		if(mode_ == camera_mode::none)
		{
			if (light_selected_ && ImGuizmo::Manipulate(glm::value_ptr(camera_->view_matrix()), glm::value_ptr(camera_->projection_matrix()), ImGuizmo::TRANSLATE, ImGuizmo::LOCAL, glm::value_ptr(light_.transform.local_to_world_matrix()), NULL, NULL, NULL, NULL))
			{
				light_.transform.model_matrix_updated();
			}

			ImGui::Begin("Bezier Surface");

			surface_.imgui(*camera_);

			if (ImGui::CollapsingHeader("Light"))
			{
				if (ImGui::ColorEdit3("Color##2", glm::value_ptr(light_.color.get())))
				{
					light_.color.set_dirty();
				}
				if (ImGui::DragFloat3("Pos##2", glm::value_ptr(light_.transform.pos().get()), 0.01f))
				{
					light_.transform.set_pos(light_.transform.pos());
				}

				ImGui::Checkbox("Light animation", &animate_light);
			}

			if (ImGui::CollapsingHeader("Environment"))
			{
				if (ImGui::ColorEdit3("Ambient", glm::value_ptr(ambient_color_)))
				{
					surface_.set_ambient(ambient_color_);
				}
			}

			ImGui::End();
		}
	}

	void on_event(kEn::base_event& event) override
	{
		dispatcher_->dispatch(event);
	}

	bool on_window_resize(kEn::window_resize_event& event)
	{
		window_center_ = glm::vec2(event.width() / 2, event.height() / 2);
		framebuffer_->resize(event.width(), event.height());
		return false;
	}

	bool on_key_press(kEn::key_pressed_event& event)
	{
		if (event.key() == kEn::key::f)
		{
			kEn::input::set_mouse_pos(window_center_);
			kEn::input::set_cursor_visible(false);
			mode_ = camera_mode::free_look;
		}
		else if (event.key() == kEn::key::o)
		{
			kEn::input::set_mouse_pos(window_center_);
			kEn::input::set_cursor_visible(false);
			mode_ = camera_mode::orbit;
			camera_->look_at(camera_mount_.pos(), { 0,1,0 });
		}

		return mode_ != camera_mode::none;
	}

	bool on_mouse_click(kEn::mouse_button_pressed_event& event)
	{
		framebuffer_->bind();
		int pixelData = framebuffer_->read_pixel(1, static_cast<int>(kEn::input::get_mouse_x()), static_cast<int>(framebuffer_->get_spec().height) - static_cast<int>(kEn::input::get_mouse_y()));
		framebuffer_->unbind();

		if (pixelData < 0) {
			light_selected_ = false;
			surface_.selected_point_ = nullptr;
			return false;
		}
		else if (pixelData == 113)
		{
			light_selected_ = true;
			surface_.selected_point_ = nullptr;
			return false;
		}

		light_selected_ = false;
		surface_.selected_point_ = surface_.control_points_[pixelData / surface_.M_][pixelData % surface_.M_];
		return false;
	}


private:
	float time_ = 0;

	bezier_surface surface_;
	camera_mode mode_;
	kEn::transform camera_mount_;
	std::shared_ptr<kEn::camera> camera_;
	std::unique_ptr<kEn::event_dispatcher> dispatcher_;
	std::shared_ptr<kEn::framebuffer> framebuffer_;

	std::unique_ptr<kEn::shader> mouse_pick_shader_;

	glm::vec3 ambient_color_;

	glm::vec2 window_center_;
	kEn::point_light light_;
	bool light_selected_ = false;
	bool animate_light = false;
};

class bezier : public kEn::application
{
public:
	bezier()
	{
		srand(time(NULL));
		push_layer(new main_layer());
	}

};

kEn::application* kEn::create_application()
{
	return new bezier();
}
