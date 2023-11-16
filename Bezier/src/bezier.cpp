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
#include "kEn/event/mouse_events.h"
#include "kEn/renderer/framebuffer.h"
#include "kEn/renderer/texture.h"
#include "kEn/renderer/mesh/obj_model.h"

class main_layer : public kEn::layer
{
public:
	main_layer() : layer("FizzBuzz")
	{
		dispatcher_ = std::make_unique<kEn::event_dispatcher>();

		//camera_ = std::make_shared<kEn::orthographic_camera>(-16.f/9.f, 16.f / 9.f, -1.f, 1.f);
		camera_ = std::make_shared<kEn::perspective_camera>(glm::radians(70.f), 16.f/9.f, 0.01f, 100.f);
		camera_->set_position({ 0,0,1 });
		dispatcher_->subscribe<kEn::window_resize_event>(KEN_EVENT_SUBSCRIBER(camera_->on_window_resize));
		dispatcher_->subscribe<kEn::mouse_button_pressed_event>(KEN_EVENT_SUBSCRIBER(surface_.on_mouse_click));
	}

	void on_update(double delta, double time) override
	{
		//camera_.set_rotation(glm::rotate(camera_.rotation(), (float) delta, { 0, 1.0f, 0.0f }));
		//plane.transform().rotate({1, 0, 0}, (float)delta / 10);


		delta /= 3;
		if(kEn::input::is_key_pressed(kEn::key::up))
		{
			surface_.transform().rotate({ 1,0,0 }, -(float)delta);
		}
		 if(kEn::input::is_key_pressed(kEn::key::down))
		{
			surface_.transform().rotate({ 1,0,0 }, (float)delta);
		}
		 if (kEn::input::is_key_pressed(kEn::key::left))
		{
			surface_.transform().rotate({ 0,0,1 }, (float)delta);
		}
		 if (kEn::input::is_key_pressed(kEn::key::right))
		{
			surface_.transform().rotate({ 0,0,1 }, -(float)delta);
		}
	}

	void on_render() override
	{
		kEn::render_command::set_clear_color({ 0.61f, 0.2f, 0.83f, 1.0f });
		kEn::render_command::clear();
		kEn::renderer::begin_scene(camera_);
		{
			surface_.render();
		}
		kEn::renderer::end_scene();
	}

	void on_attach() override
	{
		KEN_DEBUG("Attached!");
	}

	void on_detach() override
	{
		KEN_DEBUG("Detached!");
	}

	void on_imgui() override
	{
		ImGui::Begin("Fizzbuzz!");
		ImGui::Text("Fizz or buzz? That is the question...");

		ImGuiIO& io = ImGui::GetIO();
		ImGuizmo::SetRect(0, 0, io.DisplaySize.x, io.DisplaySize.y);
		ImGui::End();

		surface_.imgui(*camera_);
	}

	void on_event(kEn::base_event& event) override
	{
		dispatcher_->dispatch(event);
	}

private:
	float time_ = 0;

	bezier_surface surface_;
	std::shared_ptr<kEn::camera> camera_;
	std::unique_ptr<kEn::event_dispatcher> dispatcher_;
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
