#include <kEn.h>
#include <kEn/core/assert.h>

#include <imgui/imgui.h>
#include <ImGuizmo/ImGuizmo.h>

#include "bezier_surface.h"
#include "sphere.h"
#include "vertex.h"
#include "glm/gtc/type_ptr.hpp"
#include "glm/gtx/euler_angles.hpp"
#include "kEn/camera/camera.h"
#include "kEn/core/transform.h"
#include "kEn/event/key_events.h"
#include "kEn/event/mouse_events.h"
#include "kEn/renderer/light.h"
#include "kEn/renderer/texture.h"
#include "kEn/renderer/mesh/model.h"
#include "kEn/renderer/scene/game_object.h"
#include "kEn/renderer/scene/core_components.h"

class obj_component : public kEn::game_component
{
public:
	obj_component(const std::string& model = "sphere.obj")
		: model_(std::filesystem::path("assets/models") / model)
	{
		kEn::texture_spec spec;
		spec.mag_filter = kEn::texture_spec::filter::NEAREST;
		material_.set_texture(kEn::texture_type::diffuse, kEn::texture2D::create(R"(C:\Users\uguno\Desktop\mine\3\tex\red_wool.png)", spec));
	}

	[[nodiscard]] std::shared_ptr<game_component> clone() const override
	{
		return std::make_shared<obj_component>();
	}
	void update(float delta) override {}
	void render(kEn::shader& shader) override
	{
		shader.set_material("u_Material", material_);
		material_.bind();
		model_.render(shader, transform());
	}


	kEn::model model_;
	kEn::material material_;
};

class main_layer : public kEn::layer
{
public:
	glm::vec3 eulerAngles(glm::quat q)
	{
		float theta1, theta2, theta3;
		glm::extractEulerAngleYXZ(glm::mat4_cast(q), theta1, theta2, theta3);
		return glm::vec3(theta2, theta1, theta3);
	}

	main_layer() : layer("BezierLayer"), ambient_color_{ 0.0f, 0.01f, 0.22f }, window_center_(640, 360)
	{
		dispatcher_ = std::make_unique<kEn::event_dispatcher>();
		kEn::texture_spec specc = kEn::texture_spec().set_mipmap_levels(1).set_mag_filter(kEn::texture_spec::filter::NEAREST);
		model_ = kEn::model::load(R"(C:\Users\uguno\Desktop\mine\4\test.obj)", specc);

		kEn::framebuffer_spec spec;
		spec.width = 1280;
		spec.height = 720;
		spec.attachments = { kEn::framebuffer_texture_format::RGBA8, kEn::framebuffer_texture_format::RED_INT };
		framebuffer_ = kEn::framebuffer::create(spec);

		// mouse_pick_shader_ = kEn::shader::create("control_point");


		phong_ = kEn::shader::create("phong");

		light_.transform.set_local_pos({ 0, 10, 0 });

		phong_->set_light("u_Light", light_);

		main.transform().set_local_pos({ 20, 0, 20 });
		sphere1.transform().set_local_pos({ 3,0,0 });
		sphere2.transform().set_local_pos({ 0,5,0 });
		main.add_children({ sphere1, camera_object_ });

		camera_object_.transform().set_local_pos({ 0,5,0 });

		main.add_component(std::make_shared<obj_component>());
		sphere1.add_component(std::make_shared<obj_component>());
		sphere2.add_component(std::make_shared<obj_component>("suzanne.obj"));
		sphere2.add_component(std::make_shared<kEn::look_at_component>(camera_object_));

		//camera_ = std::make_shared<kEn::orthographic_camera>(-16.f/9.f, 16.f / 9.f, -1.f, 1.f);
		camera_ = std::make_shared<kEn::perspective_camera>(glm::radians(90.f), 16.f / 9.f, 0.01f, 100.f);
		camera_object_.add_component(camera_);
		// camera_object_.add_component(std::make_shared<kEn::free_look_component>(0.1f));
		// camera_object_.add_component(std::make_shared<kEn::free_move_component>(2.f));
		camera_object_.add_component(std::make_shared<kEn::look_at_component>(sphere1));

		// surface_.set_ambient(ambient_color_);
		// sphere_.set_ambient(ambient_color_);

		dispatcher_->subscribe<kEn::window_resize_event>(KEN_EVENT_SUBSCRIBER(on_window_resize));
		// dispatcher_->subscribe<kEn::mouse_button_pressed_event>(KEN_EVENT_SUBSCRIBER(on_mouse_click));
	}

	void on_update(double delta, double) override
	{
		main.update_all(delta);
		camera_object_.update(delta);

		static bool state = true;
		static double progress = 0.0;
		const double speed = 8.0;
		const double radius = 3.;

		static glm::vec3 front = main.transform().front();
		static glm::vec3 left = -main.transform().right();
		static glm::quat rot = glm::rotation(front, left);

		progress += speed * delta;
		if(state)
		{
			if(progress >= 40.)
			{
				main.transform().fma(main.transform().front(), progress - 40.);
				state = !state;
				progress = 0.0;
			}
			else
			{
				main.transform().fma(main.transform().front(), speed * delta);
			}
		}
		else
		{
			if (progress >= radius / 2. * glm::pi<double>())
			{
				state = !state;
				progress = 0.0;
				main.transform().set_local_rot(rot);
				// front = main.transform().front();
				left = glm::vec3(left.z, 0, -left.x);
				rot = glm::rotation(front, left);
			}
			else
			{
				main.transform().rotate({ 0,1,0 }, speed * delta / radius);
			}
		}

		// main.transform().rotate({ 0,1,0 }, 2*delta);
		
		// time += speed * delta;
		// glm::vec3 new_pos(glm::cos(13 * time), 0, glm::sin(13 * time));
		//
		// new_pos *= 0.5f * glm::sin(time);
		// new_pos.y = main.transform().pos().y;
		//
		// main.transform().set_local_pos(new_pos);
		
	}

	void on_render() override
	{
		kEn::render_command::set_clear_color({ ambient_color_, 1.0f });
		kEn::render_command::clear();

		kEn::renderer::begin_scene(camera_);
		{
			main.render_all(*phong_);
			model_->render(*phong_, kEn::transform(glm::vec3(0,-4,0)));

			// Draw control points for mouse picking
			// framebuffer_->bind();
			// framebuffer_->clear_attachment(0, 0);
			// framebuffer_->clear_attachment(1, -1);
			//
			// mouse_pick_shader_->bind();
			// mouse_pick_shader_->set_int("u_Id", 113);
			// kEn::renderer::submit(*mouse_pick_shader_, *surface_.get_control_point_model().vertex_array_, light_.transform);
			//
			// surface_.render_mouse_pick(*mouse_pick_shader_);
			//
			// framebuffer_->unbind();
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
		ImGui::Begin("Test");
		model_->imgui();
		ImGui::End();

		ImGui::Begin("Info");
		glm::vec3 p(main.transform().pos());
		ImGui::InputFloat3("Pos", glm::value_ptr(p));
		p = main.transform().front();
		ImGui::InputFloat3("Front", glm::value_ptr(p));
		ImGui::End();

	// 	ImGuiIO& io = ImGui::GetIO();
	// 	ImGuizmo::SetRect(0, 0, io.DisplaySize.x, io.DisplaySize.y);
	//
	// 	{
	// 		if (light_selected_ && ImGuizmo::Manipulate(glm::value_ptr(camera_->view_matrix()), glm::value_ptr(camera_->projection_matrix()), ImGuizmo::TRANSLATE, ImGuizmo::LOCAL, glm::value_ptr(light_.transform.local_to_world_matrix()), NULL, NULL, NULL, NULL))
	// 		{
	// 			light_.transform.model_matrix_updated();
	// 		}
	//
	// 		ImGui::Begin("Bezier Surface");
	//
	// 		surface_.imgui(*camera_);
	//
	// 		ImGui::End();
	//
	//
	// 		ImGui::Begin("Sphere Surface");
	// 		if (ImGui::Checkbox("Gizmo", &sphere_.selected))
	// 		{
	// 			light_selected_ = false;
	// 			surface_.selected_point_ = nullptr;
	// 		}
	//
	// 		sphere_.imgui(*camera_);
	//
	// 		ImGui::End();
	//
	// 		ImGui::Begin("Common");
	// 		if (ImGui::CollapsingHeader("Light"))
	// 		{
	// 			if (ImGui::ColorEdit3("Color##2", glm::value_ptr(light_.color)))
	// 			{
	// 				// light_.color.set_dirty();
	// 			}
	// 			if (ImGui::DragFloat3("Pos##2", glm::value_ptr(light_.transform.local_pos()), 0.01f))
	// 			{
	// 				light_.transform.set_dirty();
	// 			}
	//
	// 			ImGui::Checkbox("Light animation", &animate_light);
	// 		}
	//
	// 		if (ImGui::CollapsingHeader("Environment"))
	// 		{
	// 			if (ImGui::ColorEdit3("Ambient", glm::value_ptr(ambient_color_)))
	// 			{
	// 				surface_.set_ambient(ambient_color_);
	// 				sphere_.set_ambient(ambient_color_);
	// 			}
	// 		}
	// 		ImGui::End();
	// 	}
	}

	void on_event(kEn::base_event& event) override
	{
		dispatcher_->dispatch(event);
		main.on_event(event);
		camera_object_.on_event(event);
	}

	bool on_window_resize(kEn::window_resize_event& event)
	{
		window_center_ = glm::vec2(event.width() / 2, event.height() / 2);
		framebuffer_->resize(event.width(), event.height());
		return false;
	}
	
	// bool on_mouse_click(kEn::mouse_button_pressed_event& event)
	// {
	// 	framebuffer_->bind();
	// 	int pixelData = framebuffer_->read_pixel(1, static_cast<int>(kEn::input::get_mouse_x()), static_cast<int>(framebuffer_->get_spec().height) - static_cast<int>(kEn::input::get_mouse_y()));
	// 	framebuffer_->unbind();
	//
	// 	if (pixelData < 0) {
	// 		light_selected_ = false;
	// 		surface_.selected_point_ = nullptr;
	// 		return false;
	// 	}
	// 	else if (pixelData == 113)
	// 	{
	// 		light_selected_ = true;
	// 		surface_.selected_point_ = nullptr;
	// 		sphere_.selected = false;
	// 		return false;
	// 	}
	//
	// 	light_selected_ = false;
	// 	surface_.selected_point_ = surface_.control_points_[pixelData / surface_.M_][pixelData % surface_.M_];
	// 	sphere_.selected = false;
	// 	return false;
	// }


private:
	float time_ = 0;

	// bezier_surface surface_;
	// sphere sphere_;
	std::shared_ptr<kEn::camera> camera_;
	std::unique_ptr<kEn::event_dispatcher> dispatcher_;
	std::shared_ptr<kEn::framebuffer> framebuffer_;

	// std::unique_ptr<kEn::shader> mouse_pick_shader_;

	std::shared_ptr<kEn::model> model_;
	std::unique_ptr<kEn::shader> phong_;

	kEn::game_object main, model, sphere1, sphere2;
	kEn::game_object camera_object_;

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
