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
#include "kEn/renderer/mesh/obj_model.h"

class fizzbuzz_layer : public kEn::layer
{
public:
	fizzbuzz_layer() : layer("FizzBuzz"), model("cube.obj")
	{
		dispatcher_ = std::make_unique<kEn::event_dispatcher>();

		//camera_ = std::make_shared<kEn::orthographic_camera>(-16.f/9.f, 16.f / 9.f, -1.f, 1.f);
		camera_ = std::make_shared<kEn::perspective_camera>(glm::radians(70.f), 16.f/9.f, 0.01f, 100.f);
		camera_->set_position({ 0,0.4,2 });
		dispatcher_->subscribe<kEn::window_resize_event>(KEN_EVENT_SUBSCRIBER(camera_->on_window_resize));

		vertex_array_ = vertex::generate_vertex_buffer();
		vertex_buffer_ = std::dynamic_pointer_cast<kEn::mutable_vertex_buffer, kEn::vertex_buffer>(vertex_array_->vertex_buffers()[0]);

		shader_ = kEn::shader::create("test");
		shader_2 = kEn::shader::create("test2");
		texture_2 = kEn::texture2D::create("5h.jpg");
		texture_ = kEn::texture2D::create("l.jpg");

		shader_->bind();
		shader_->set_int("u_Texture", 0);
		shader_2->bind();
		shader_2->set_int("u_Texture", 0);

		transform_.rotate({ 1,0,0 }, glm::pi<float>() / 2);

		for(int i = 0; i < 4; i++)
		{
			for(int j = 0; j < 4; j++)
			{
				transform_2[i][j].set_pos({ glm::mix(-0.5, 0.5, (float)i / 3.0f), .0f, glm::mix(-0.5, 0.5, (float)j / 3.0f) });
				transform_2[i][j].set_parent(&transform_);
				transform_2[i][j].set_scale(glm::vec3(0.02f));
			}
		}
	}

	void on_update(double delta, double time) override
	{
		shader_->bind();
		//camera_.set_rotation(glm::rotate(camera_.rotation(), (float) delta, { 0, 1.0f, 0.0f }));
		transform_.rotate({ 1, 0, 0 }, (float) delta/10);
		//transform_.set_pos({ 0, 0, sin(time)});
		vertex_buffer_->modify_data([this](void* buffer) {
			auto vertices = static_cast<float*>(buffer);
			vertices[0] = transform_2[0][0].local_to_parent_matrix()[3][0];
			vertices[1] = transform_2[0][0].local_to_parent_matrix()[3][1];
			vertices[2] = transform_2[0][0].local_to_parent_matrix()[3][2];
		});
	}

	void on_render() override
	{
		kEn::render_command::set_clear_color({ 1.0f, 0.0f, 1.0f, 1.0f });
		kEn::render_command::clear();

		kEn::renderer::begin_scene(camera_);
		{
			texture_->bind();
			kEn::renderer::submit(shader_2, vertex_array_, transform_);

			texture_2->bind();
			for (int i = 0; i < 4; i++)
			{
				for (int j = 0; j < 4; j++)
				{
					kEn::renderer::submit(shader_, model.vertex_array_, transform_2[i][j]);
				}
			}
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

		if(ImGuizmo::Manipulate(glm::value_ptr(camera_->view_matrix()), glm::value_ptr(camera_->projection_matrix()), ImGuizmo::TRANSLATE, ImGuizmo::LOCAL, glm::value_ptr(transform_2[0][0].local_to_world_matrix()), NULL,  NULL, NULL,  NULL))
		{
			transform_2[0][0].model_matrix_updated();
		}

		ImGui::InputFloat3("Tr", glm::value_ptr(transform_2[0][0].pos()));
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
	kEn::transform transform_2[4][4];

	std::unique_ptr<kEn::event_dispatcher> dispatcher_;
	std::shared_ptr<kEn::vertex_array> vertex_array_;
	std::shared_ptr<kEn::shader> shader_, shader_2;
	std::shared_ptr<kEn::texture> texture_, texture_2;

	std::shared_ptr<kEn::mutable_vertex_buffer> vertex_buffer_;

	kEn::obj_model model;
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
