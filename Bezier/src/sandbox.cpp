#include <kEn.h>
#include <kEn/core/assert.h>

#include <imgui/imgui.h>

#include "vertex.h"
#include "glm/gtc/type_ptr.hpp"
#include "imgui/imgui_internal.h"
#include "ImGuizmo/ImGuizmo.h"
#include "kEn/camera/camera.h"
#include "kEn/core/transform.h"
#include "kEn/renderer/texture.h"

class fizzbuzz_layer : public kEn::layer
{
public:
	fizzbuzz_layer() : layer("FizzBuzz")
	{
		dispatcher_ = std::make_unique<kEn::event_dispatcher>();

		//camera_ = std::make_shared<kEn::orthographic_camera>(-16.f/9.f, 16.f / 9.f, -1.f, 1.f);
		camera_ = std::make_shared<kEn::perspective_camera>(glm::radians(70.f), 16.f/9.f, 0.01f, 100.f);
		camera_->set_position({ 0,0.4,2 });
		dispatcher_->subscribe<kEn::window_resize_event>(KEN_EVENT_SUBSCRIBER(camera_->on_window_resize));

		vertex_array_ = vertex::generate_vertex_buffer();
		vertex_buffer_ = std::dynamic_pointer_cast<kEn::mutable_vertex_buffer, kEn::vertex_buffer>(vertex_array_->vertex_buffers()[0]);

		shader_ = kEn::shader::create("test");
		texture_ = kEn::texture2D::create("l.jpg");

		shader_->bind();
		shader_->set_int("u_Texture", 0);

		transform_.rotate({ 1,0,0 }, glm::pi<float>() / 2);

		transform_2.set_pos({ -.5f, .0f, -.5f });
		transform_2.set_parent(&transform_);
	}

	void on_update(double delta, double time) override
	{
		shader_->bind();
		//camera_.set_rotation(glm::rotate(camera_.rotation(), (float) delta, { 0, 1.0f, 0.0f }));
		transform_.rotate({ 1, 0, 0 }, (float) delta/10);
		//transform_.set_pos({ 0, 0, sin(time)});
		vertex_buffer_->modify_data([this](void* buffer) {
			auto vertices = static_cast<float*>(buffer);
			vertices[0] = transform_2.local_to_parent_matrix()[3][0];
			vertices[1] = transform_2.local_to_parent_matrix()[3][1];
			vertices[2] = transform_2.local_to_parent_matrix()[3][2];
		});
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

		ImGuiIO& io = ImGui::GetIO();
		ImGuizmo::SetRect(0, 0, io.DisplaySize.x, io.DisplaySize.y);

		if(ImGuizmo::Manipulate(glm::value_ptr(camera_->view_matrix()), glm::value_ptr(camera_->projection_matrix()), ImGuizmo::TRANSLATE, ImGuizmo::LOCAL, glm::value_ptr(transform_2.local_to_world_matrix()), NULL,  NULL, NULL,  NULL))
		{
			transform_2.model_matrix_updated();
		}

		float matrixTranslation[3], matrixRotation[3], matrixScale[3];
		ImGui::InputFloat3("Tr", glm::value_ptr(transform_2.pos()));
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
	kEn::transform transform_2;

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
