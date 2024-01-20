#include <kEn.h>

#include <imgui/imgui.h>
#include <ImGuizmo/ImGuizmo.h>

#include "bezier_surface.h"
#include "torus.h"
#include "vertex.h"
#include "glm/gtc/type_ptr.hpp"
#include "glm/gtx/euler_angles.hpp"
#include "kEn/scene/camera/camera.h"
#include "kEn/core/transform.h"
#include "kEn/event/key_events.h"
#include "kEn/scene/light.h"
#include "kEn/renderer/texture.h"
#include "kEn/scene/mesh/model.h"
#include "kEn/scene/game_object.h"
#include "kEn/scene/core_components.h"

class main_layer : public kEn::layer
{
public:
	main_layer() : layer("BezierLayer"), ambient_color_{ 0, 0.02f, 0.2f }, window_center_(640, 360)
	{
		dispatcher_ = std::make_unique<kEn::event_dispatcher>();
		dispatcher_->subscribe<kEn::window_resize_event>(KEN_EVENT_SUBSCRIBER(on_window_resize));

		kEn::framebuffer_spec spec;
		spec.width = 1280;
		spec.height = 720;
		spec.attachments = { kEn::framebuffer_texture_format::RGBA8, kEn::framebuffer_texture_format::RED_INT };

		/// SHADERS
		{
			phong_ = kEn::shader::create("phong");
			gouraud_ = kEn::shader::create("gouraud");
			flat_ = kEn::shader::create("flat");

			active_shader_ = phong_;

			torus_phong_ = kEn::shader::create("torus_phong", { false, true });
			torus_gouraud_ = kEn::shader::create("torus_gouraud", { false, true });
			torus_flat_ = kEn::shader::create("torus_flat", { false, true });

			torus_active_shader_ = torus_phong_;
		}

		/// SCENE
		{
			headlight_.transform().set_local_pos({ 0, 0.78f, -0.5f });
			scooter_spot_light_ = std::make_shared<kEn::spot_light>();
			scooter_spot_light_->color = { 1.f, 1.f, 1.f };
			scooter_spot_light_->inner_cutoff_angle = glm::radians(15.f);
			scooter_spot_light_->outer_cutoff_angle = glm::radians(30.f);
			scooter_spot_light_->atten.linear = 0.045f;
			scooter_spot_light_->atten.quadratic = 0.0075f;
			headlight_.add_component(scooter_spot_light_);
			kEn::renderer::add_light(scooter_spot_light_);

			scooter_.transform().set_local_pos({ 3,0,0 });
			scooter_.transform().set_local_scale(2.f);
			scooter_model_ = kEn::model::load(R"(scooter\vino.obj)");
			scooter_.add_component(std::make_shared<kEn::model_component>(scooter_model_));
			scooter_.add_child(headlight_);

			scooter_mount_.transform().set_local_pos({ 20, 0, 20 });
			scooter_mount_.add_child(scooter_);
			root_.add_child(scooter_mount_);

			scene_.transform().set_local_pos({ 0,-4,0 });
			kEn::texture_spec specc = kEn::texture_spec().set_mipmap_levels(1).set_mag_filter(kEn::texture_spec::filter::NEAREST);
			terrain_ = kEn::model::load(R"(terrain\terrain.obj)", specc);
			scene_.add_component(std::make_shared<kEn::model_component>(terrain_));
			root_.add_child(scene_);

			torus_component_ = std::make_shared<torus>();
			torus_.add_component(torus_component_);
			torus_.transform().set_local_pos({ 0,7,0 });
		}


		/// CAMERAS
		{
			//camera_ = std::make_shared<kEn::orthographic_camera>(-16.f/9.f, 16.f / 9.f, -1.f, 1.f);
			free_camera_ = std::make_shared<kEn::perspective_camera>(glm::radians(90.f), 16.f / 9.f, 0.01f, 100.f);
			free_cam_object_.transform().set_local_pos({ 0,5,0 });
			free_cam_object_.add_component(free_camera_);
			free_cam_object_.add_component(std::make_shared<kEn::free_look_component>(0.1f));
			free_cam_object_.add_component(std::make_shared<kEn::free_move_component>(5.f));
			flashlight_ = std::make_shared<kEn::spot_light>();
			flashlight_->color = glm::vec3( 0.f );
			flashlight_->inner_cutoff_angle = glm::radians(15.f);
			flashlight_->outer_cutoff_angle = glm::radians(30.f);
			flashlight_->atten.linear = 0.027f;
			flashlight_->atten.quadratic = 0.0028f;
			free_cam_object_.add_component(flashlight_);
			kEn::renderer::add_light(flashlight_);

			moving_camera_ = std::make_shared<kEn::perspective_camera>(glm::radians(90.f), 16.f / 9.f, 0.01f, 100.f);
			moving_cam_object_.transform().set_local_pos({ 0,1.25f, .2f });
			moving_cam_object_.add_component(moving_camera_);
			scooter_.add_child(moving_cam_object_);

			lookat_camera_ = std::make_shared<kEn::perspective_camera>(glm::radians(90.f), 16.f / 9.f, 0.01f, 100.f);
			lookat_cam_object_.transform().set_local_pos({ 25.5f,4.5f,25.5f });
			lookat_cam_object_.add_component(lookat_camera_);
			lookat_cam_object_.add_component(std::make_shared<kEn::look_at_component>(scooter_));

			active_camera_ = free_camera_;
		}

		/// LIGHTS
		{
			auto lamp = std::make_shared<kEn::spot_light>();
			lamp->color = { .5f, .5f, 0.5f};
			lamp->inner_cutoff_angle = glm::radians(45.f);
			lamp->outer_cutoff_angle = glm::radians(60.f);
			lamp_.add_component(lamp);
			root_.add_child(lamp_);
			lamp_.transform().rotate({ 1,0,0 }, -glm::pi<float>() / 2.f);
			lamp_.transform().set_local_pos({ 0, 10, 0});
			kEn::renderer::add_light(lamp);

			prepare_point_lights();

			kEn::renderer::set_ambient(ambient_color_);
			kEn::renderer::prepare(*phong_);
			kEn::renderer::prepare(*gouraud_);
			kEn::renderer::prepare(*flat_);
			kEn::renderer::prepare(*torus_phong_);
			kEn::renderer::prepare(*torus_gouraud_);
			kEn::renderer::prepare(*torus_flat_);
		}
	}

	void prepare_point_lights()
	{
		kEn::point_light light;
		light.color = { 242.f / 255.f, 189.f / 255.f, 116.f / 255.f };
		light.atten.linear = .14f;
		light.atten.quadratic = .07f;
		float x = 3.5f;
		float z = -25.5f;
		float x2 = -25.5f;
		float z2 = -25.5f;
		for (int i = 0; i < 12; ++i)
		{
			auto component = std::dynamic_pointer_cast<kEn::point_light>(light.clone());
			lights_[i].add_component(component);
			lights_[i].transform().set_local_pos({ x, 2.5f, z });
			kEn::renderer::add_light(component);
			root_.add_child(lights_[i]);

			i++;

			component = std::dynamic_pointer_cast<kEn::point_light>(light.clone());
			lights_[i].add_component(component);
			lights_[i].transform().set_local_pos({ -x, 2.5f, z });
			kEn::renderer::add_light(component);
			root_.add_child(lights_[i]);

			i++;

			component = std::dynamic_pointer_cast<kEn::point_light>(light.clone());
			lights_[i].add_component(component);
			lights_[i].transform().set_local_pos({ x2, 2.5f, z2 });
			kEn::renderer::add_light(component);
			root_.add_child(lights_[i]);

			float tmp = x;
			x = z;
			z = -tmp;

			tmp = x2;
			x2 = z2;
			z2 = -tmp;
		}
	}

	void on_update(double delta, double) override
	{
		root_.update_all(delta);
		active_camera_->parent().update(delta);
		torus_.update(delta);

		flashlight_->load("u_SpotLights[1]", *active_shader_);
		flashlight_->load("u_SpotLights[1]", *torus_active_shader_);

		if (animate_scooter_) {
			static bool state = true;
			static double progress = 0.0;
			const double speed = 4.0;
			const double radius = 3.;

			static glm::vec3 front = scooter_mount_.transform().front();
			static glm::vec3 left = -scooter_mount_.transform().right();
			static glm::quat rot = glm::rotation(front, left);

			progress += speed * delta;
			if (state)
			{
				if (progress >= 40.)
				{
					scooter_mount_.transform().fma(scooter_mount_.transform().front(), progress - 40.);
					state = !state;
					progress = 0.0;
				}
				else
				{
					scooter_mount_.transform().fma(scooter_mount_.transform().front(), speed * delta);
				}
			}
			else
			{
				if (progress >= radius / 2. * glm::pi<double>())
				{
					state = !state;
					progress = 0.0;
					scooter_mount_.transform().set_local_rot(rot);
					left = glm::vec3(left.z, 0, -left.x);
					rot = glm::rotation(front, left);
				}
				else
				{
					scooter_mount_.transform().rotate({ 0,1,0 }, speed * delta / radius);
				}
			}

			scooter_spot_light_->load("u_SpotLights[0]", *active_shader_);
			scooter_spot_light_->load("u_SpotLights[0]", *torus_active_shader_);
		}

		if(animate_torus_)
		{
			static float time = 0;
			time += delta;
			torus_.transform().rotate({ 1,0,0 }, 1.f * delta);
			torus_.transform().rotate({ 0,1,0 }, 2.f * delta);
			torus_.transform().rotate({ 0,0,1 }, 3.f * delta);

			torus_.transform().set_local_scale({ 0.1f + 0.9f * glm::pow(glm::cos(time), 2), 1, 1 });
		}
	}

	void on_render() override
	{
		kEn::render_command::set_clear_color({ ambient_color_, 1.0f });
		kEn::render_command::clear();

		kEn::renderer::begin_scene(active_camera_);
		{
			root_.render_all(*active_shader_);
			torus_.render(*torus_active_shader_);
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
		ImGui::Begin("Properties");
		{
			if (ImGui::CollapsingHeader("Shader"))
			{
				static int shader_type = 2;
				if (ImGui::RadioButton("Flat", &shader_type, 0))
				{
					active_shader_ = flat_;
					torus_active_shader_ = torus_flat_;
					kEn::renderer::prepare(*phong_);
					kEn::renderer::prepare(*torus_phong_);
				}
				ImGui::SameLine();
				if (ImGui::RadioButton("Gouraud", &shader_type, 1))
				{
					active_shader_ = gouraud_;
					torus_active_shader_ = torus_gouraud_;
					kEn::renderer::prepare(*gouraud_);
					kEn::renderer::prepare(*torus_gouraud_);
				}
				ImGui::SameLine();
				if (ImGui::RadioButton("Phong", &shader_type, 2))
				{
					active_shader_ = phong_;
					torus_active_shader_ = torus_phong_;
					kEn::renderer::prepare(*flat_);
					kEn::renderer::prepare(*torus_flat_);
				}

				if(ImGui::ColorEdit3("Ambient", glm::value_ptr(ambient_color_)))
				{
					kEn::renderer::set_ambient(ambient_color_);
				}

				if(ImGui::SliderFloat("Fog density", &fog_, 0, 1))
				{
					kEn::renderer::set_fog(fog_);
				}
			}

			if(ImGui::CollapsingHeader("Camera"))
			{
				static int camera_type = 0;
				static bool flashlight = false;
				if (ImGui::RadioButton("Free", &camera_type, 0))
				{
					active_camera_ = free_camera_;
					flashlight_->color = flashlight ? glm::vec3(1) : glm::vec3(0);
				}
				ImGui::SameLine();
				if (ImGui::RadioButton("Look At", &camera_type, 1))
				{
					active_camera_ = lookat_camera_;
					flashlight_->color = glm::vec3(0);
				}
				ImGui::SameLine();
				if (ImGui::RadioButton("Moving", &camera_type, 2))
				{
					active_camera_ = moving_camera_;
					flashlight_->color = glm::vec3(0);
				}

				if(camera_type == 0)
				{
					if(ImGui::Checkbox("Flashlight", &flashlight))
					{
						flashlight_->color = flashlight ? glm::vec3(1) : glm::vec3(0);
					}
				}
			}

			if(ImGui::CollapsingHeader("Scooter"))
			{
				ImGui::PushID("Scooter");
				static bool gizmo = false;
				ImGui::Checkbox("Animate", &animate_scooter_);
				ImGui::Checkbox("Gizmo", &gizmo);
				if (gizmo && ImGuizmo::Manipulate(glm::value_ptr(active_camera_->view_matrix()), glm::value_ptr(active_camera_->projection_matrix()), ImGuizmo::ROTATE, ImGuizmo::LOCAL, glm::value_ptr(headlight_.transform().local_to_world_matrix()), NULL, NULL, NULL, NULL))
				{
					headlight_.transform().model_matrix_updated();
					scooter_spot_light_->load("u_SpotLights[0]", *active_shader_);
					scooter_spot_light_->load("u_SpotLights[0]", *torus_active_shader_);
				}

				scooter_model_->imgui();
				ImGui::PopID();
			}

			if (ImGui::CollapsingHeader("Terrain"))
			{
				ImGui::PushID("Terrain");
				terrain_->imgui();
				ImGui::PopID();
			}

			if(ImGui::CollapsingHeader("Torus"))
			{
				ImGui::PushID("Torus");
				ImGui::Checkbox("Animate", &animate_torus_);
				torus_component_->imgui();
				ImGui::PopID();
			}
		}
		ImGui::End();
	
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
	}

	void on_event(kEn::base_event& event) override
	{
		dispatcher_->dispatch(event);
		root_.on_event(event);
		active_camera_->parent().on_event(event);
	}

	bool on_window_resize(kEn::window_resize_event& event)
	{
		window_center_ = glm::vec2(event.width() / 2, event.height() / 2);
		return false;
	}


private:
	float time_ = 0;

	std::unique_ptr<kEn::event_dispatcher> dispatcher_;

	std::shared_ptr<kEn::model> terrain_, scooter_model_;
	std::shared_ptr<kEn::shader> phong_, gouraud_, flat_, active_shader_;

	kEn::game_object root_, scene_;
	kEn::game_object lamp_;
	bool animate_scooter_ = true;
	kEn::game_object scooter_mount_, scooter_, headlight_;
	std::shared_ptr<kEn::spot_light> scooter_spot_light_;

	kEn::game_object torus_;
	std::shared_ptr<torus> torus_component_;
	bool animate_torus_ = false;
	std::shared_ptr<kEn::shader> torus_phong_, torus_gouraud_, torus_flat_, torus_active_shader_;

	kEn::game_object free_cam_object_, lookat_cam_object_, moving_cam_object_;
	std::shared_ptr<kEn::camera> free_camera_, lookat_camera_, moving_camera_, active_camera_;
	std::shared_ptr<kEn::spot_light> flashlight_;

	glm::vec3 ambient_color_;
	float fog_;

	glm::vec2 window_center_;
	std::array<kEn::game_object, 12> lights_;

	friend class torus;
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
