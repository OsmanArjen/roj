#include <editor/scene/component_editors.h>

#include <algorithm>

#include <editor/scene/widgets.h>

namespace roj {

	bool draw_camera_fields(Camera& c) {
		using namespace scene_ui;
		bool edited = false;

		edited |= prop_vec3("Position", c.position, 0.05f);
		edited |= prop_float("Yaw", c.yaw, 0.25f, -360.0f, 360.0f, "%.1f deg");
		edited |= prop_float("Pitch", c.pitch, 0.25f, -89.0f, 89.0f, "%.1f deg");
		edited |= prop_slider("FOV (Y)", c.fovY, 10.0f, 120.0f, "%.1f deg");

		edited |= prop_float("Near", c.nearPlane, 0.01f, 0.001f, c.farPlane * 0.999f, "%.3f", ImGuiSliderFlags_Logarithmic);
		edited |= prop_float("Far", c.farPlane, 1.0f, c.nearPlane * 1.001f, 1.0e6f, "%.1f", ImGuiSliderFlags_Logarithmic);

		return edited;
	}

	bool draw_fly_controller_fields(FlyController& fc) {
		using namespace scene_ui;
		bool edited = false;

		edited |= prop_float("Move speed", fc.moveSpeed, 0.1f, fc.minSpeed, fc.maxSpeed, "%.2f");
		edited |= prop_float("Sprint scale", fc.sprintScale, 0.05f, 1.0f, 20.0f, "%.2f");
		edited |= prop_float("Look speed", fc.lookSpeed, 0.005f, 0.01f, 1.0f, "%.3f deg/px");
		edited |= prop_float("Wheel step", fc.wheelStep, 0.005f, 1.0f, 2.0f, "%.3f");
		edited |= prop_float("Min speed", fc.minSpeed, 0.1f, 0.01f, fc.maxSpeed, "%.2f");
		edited |= prop_float("Max speed", fc.maxSpeed, 1.0f, fc.minSpeed, 10000.0f, "%.1f");

		if (edited) fc.moveSpeed = std::clamp(fc.moveSpeed, fc.minSpeed, fc.maxSpeed);
		return edited;
	}

}

namespace roj::scene_ui {

	bool camera_body(SceneEditor&, entt::registry&, entt::entity, Camera& c) { return draw_camera_fields(c); }
	bool fly_controller_body(SceneEditor&, entt::registry&, entt::entity, FlyController& c) { return draw_fly_controller_fields(c); }

}
