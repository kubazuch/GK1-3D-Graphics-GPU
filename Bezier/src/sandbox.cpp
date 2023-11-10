#include <kEn.h>
#include <kEn/core/assert.h>

#include <imgui/imgui.h>

#include "vertex.h"
#include "kEn/camera/camera.h"
#include "kEn/core/transform.h"
#include "kEn/renderer/texture.h"

class fizzbuzz_layer : public kEn::layer
{
public:
	fizzbuzz_layer() : layer("FizzBuzz")
	{
		dispatcher_ = std::make_unique<kEn::event_dispatcher>();

		camera_ = std::make_shared<kEn::orthographic_camera>(-16.f/9.f, 16.f / 9.f, -1.f, 1.f);
		//camera_ = std::make_shared<kEn::perspective_camera>(glm::radians(70.f), 16.f/9.f, 0.01f, 100.f);
		//camera_->set_position({ 0,0,2 });
		dispatcher_->subscribe<kEn::window_resize_event>(KEN_EVENT_SUBSCRIBER(camera_->on_window_resize));

		vertex_array_ = vertex::generate_vertex_buffer();

		shader_ = kEn::shader::create("test");
		texture_ = kEn::texture2D::create("l.jpg");

		shader_->bind();
		shader_->set_int("u_Texture", 0);
	}

	void on_update(double delta, double time) override
	{
		shader_->bind();
		//camera_.set_rotation(glm::rotate(camera_.rotation(), (float) delta, { 0, 1.0f, 0.0f }));
		transform_.rotate({ 0, 1, 0 }, (float) delta);
		//transform_.set_pos({ 0, 0, sin(time)});
		//vertex_buffer_->modify_data([time](void* buffer) {
		//	auto vertices = static_cast<float*>(buffer);
		//	vertices[2] = 0.25f*glm::cos(3*time);
		//});
	}

	void on_render() override
	{
		kEn::render_command::set_clear_color({ 1.0f, 0.0f, 1.0f, 1.0f });
		kEn::render_command::clear();

		kEn::renderer::begin_scene(camera_);
		{
			texture_->bind();
			kEn::renderer::submit(shader_, vertex_array_, transform_);
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

		const auto pos = kEn::input::get_mouse_pos();
		ImGui::Text("Mouse pos: %.1f, %.1f", pos.x, pos.y);
		ImGui::End();
	}

	void on_event(kEn::base_event& event) override
	{
		dispatcher_->dispatch(event);
	}

private:
	float time_ = 0;

	std::shared_ptr<kEn::camera> camera_;
	kEn::transform transform_;

	std::unique_ptr<kEn::event_dispatcher> dispatcher_;
	std::shared_ptr<kEn::vertex_array> vertex_array_;
	std::shared_ptr<kEn::shader> shader_;
	std::shared_ptr<kEn::texture> texture_;

	std::shared_ptr<kEn::mutable_vertex_buffer> vertex_buffer_;
};

class sandbox : public kEn::application
{
public:
	sandbox()
	{
		srand(time(NULL));
		push_layer(new fizzbuzz_layer());
	}

};

kEn::application* kEn::create_application()
{
	return new sandbox();
}
