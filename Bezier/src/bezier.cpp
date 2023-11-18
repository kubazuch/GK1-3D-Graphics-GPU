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
#include "kEn/renderer/texture.h"
#include "kEn/renderer/mesh/obj_model.h"

class main_layer : public kEn::layer
{
public:
	main_layer() : layer("FizzBuzz"), window_center_(640, 360)
	{
		dispatcher_ = std::make_unique<kEn::event_dispatcher>();

		//camera_ = std::make_shared<kEn::orthographic_camera>(-16.f/9.f, 16.f / 9.f, -1.f, 1.f);
		camera_ = std::make_shared<kEn::perspective_camera>(glm::radians(70.f), 16.f / 9.f, 0.01f, 100.f);
		camera_->set_pos({ 0,0.5,-1 });
		camera_->look_at({ 0,0,0 }, { 0,1,0 });
		dispatcher_->subscribe<kEn::window_resize_event>(KEN_EVENT_SUBSCRIBER(camera_->on_window_resize));
		dispatcher_->subscribe<kEn::window_resize_event>(KEN_EVENT_SUBSCRIBER(on_window_resize));
		dispatcher_->subscribe<kEn::window_resize_event>(KEN_EVENT_SUBSCRIBER(surface_.on_window_resize));

		dispatcher_->subscribe<kEn::key_pressed_event>(KEN_EVENT_SUBSCRIBER(on_key_press));
		dispatcher_->subscribe<kEn::mouse_button_pressed_event>(KEN_EVENT_SUBSCRIBER(surface_.on_mouse_click));
	}

	void on_update(double delta, double time) override
	{
		const float sensitivity = 0.1f;
		const float speed = 0.2f;

		if (kEn::input::is_key_pressed(kEn::key::escape))
		{
			kEn::input::set_cursor_visible(true);
			mouse_locked_ = false;
		}

		if (mouse_locked_)
		{
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
		}

		float move_amount = kEn::input::is_key_pressed(kEn::key::left_control) ? 3.f * delta * speed : delta * speed;

		glm::vec3 direction{0.f};
		if (kEn::input::is_key_pressed(kEn::key::up) || kEn::input::is_key_pressed(kEn::key::w))
		{
			direction -= camera_->get_transform().front();
		}
		if (kEn::input::is_key_pressed(kEn::key::down) || kEn::input::is_key_pressed(kEn::key::s))
		{
			direction += camera_->get_transform().front();
		}
		if (kEn::input::is_key_pressed(kEn::key::left) || kEn::input::is_key_pressed(kEn::key::a))
		{
			direction -= camera_->get_transform().right();
		}
		if (kEn::input::is_key_pressed(kEn::key::right) || kEn::input::is_key_pressed(kEn::key::d))
		{
			direction += camera_->get_transform().right();
		}
		if (kEn::input::is_key_pressed(kEn::key::space) || kEn::input::is_key_pressed(kEn::key::q))
		{
			direction += glm::vec3(0, 1, 0);
		}
		if (kEn::input::is_key_pressed(kEn::key::left_shift) || kEn::input::is_key_pressed(kEn::key::e))
		{
			direction -= glm::vec3(0, 1, 0);
		}

		if(direction.x || direction.y || direction.z)
		{
			camera_->fma(glm::normalize(direction), move_amount);
		}
	}

	void on_render() override
	{
		kEn::render_command::set_clear_color({ 0.0f, 0.01f, 0.22f, 1.0f });
		kEn::render_command::clear();
		kEn::renderer::begin_scene(camera_);
		{
			surface_.render();
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

		if(!mouse_locked_)
			surface_.imgui(*camera_);
	}

	void on_event(kEn::base_event& event) override
	{
		dispatcher_->dispatch(event);
	}

	bool on_window_resize(kEn::window_resize_event& event)
	{
		window_center_ = glm::vec2(event.width() / 2, event.height() / 2);
		return false;
	}

	bool on_key_press(kEn::key_pressed_event& event)
	{
		if (event.key() == kEn::key::f)
		{
			kEn::input::set_mouse_pos(window_center_);
			kEn::input::set_cursor_visible(false);
			mouse_locked_ = true;
		}

		return mouse_locked_;
	}

private:
	float time_ = 0;
	bool mouse_locked_ = false;

	bezier_surface surface_;
	std::shared_ptr<kEn::camera> camera_;
	std::unique_ptr<kEn::event_dispatcher> dispatcher_;

	glm::vec2 window_center_;
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
