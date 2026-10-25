// Animation-related components: AnimationPlayer (editable), Rig, SkinPalette, AnimationClip (read-only).
#include <editor/scene/component_editors.h>

#include <cstdio>
#include <string>

#include <imgui.h>
#include <entt/entity/registry.hpp>
#include <editor/scene/entity_ui.h>
#include <editor/scene/theme.h>
#include <editor/scene/widgets.h>

namespace roj::scene_ui {

	namespace {
		constexpr std::size_t kShaderJointLimit = 256; // u_joints[256] in vs_static_forward.sc
	}

	bool animation_player_body(SceneEditor& ed, entt::registry& reg, entt::entity e, AnimationPlayer& p) {
		const AnimationClip* clip = reg.valid(p.clip) ? reg.try_get<AnimationClip>(p.clip) : nullptr;

		// Clip switches go through the module's own entry point instead of poking fields.
		// Chosen after the loop, because it modifies the registry we are iterating.
		entt::entity chosen = entt::null;
		ImGui::PushID("clip");
		prop_label("Clip");
		if (ImGui::BeginCombo("##v", clip ? clip->name.c_str() : "(none)")) {
			for (const entt::entity c : reg.view<AnimationClip>()) {
				const std::string& name = reg.get<AnimationClip>(c).name;
				ImGui::PushID(static_cast<int>(entt::to_integral(c)));
				if (ImGui::Selectable(name.empty() ? "(unnamed)" : name.c_str(), c == p.clip)) chosen = c;
				ImGui::PopID();
			}
			ImGui::EndCombo();
		}
		ImGui::PopID();

		if (chosen != entt::null) {
			animationPlay(reg, e, chosen, p.loop);
			return false; // animationPlay wrote the component; don't overwrite it with our copy
		}
		entity_link(ed, reg, "Clip entity", p.clip);

		bool edited = false;
		label_cell("Playback");
		if (ImGui::Button(p.playing ? "Pause" : "Play", ImVec2(ImGui::GetFontSize() * 4.5f, 0.0f))) {
			p.playing = !p.playing;
			edited = true;
		}
		ImGui::SameLine();
		if (ImGui::Button("Restart")) { p.time = 0.0f; edited = true; }
		ImGui::SameLine();
		edited |= ImGui::Checkbox("Loop", &p.loop);

		edited |= prop_float("Speed", p.speed, 0.01f, 0.0f, 4.0f, "%.2fx");

		if (clip && clip->duration > 0.0f) {
			edited |= prop_slider("Time", p.time, 0.0f, clip->duration, "%.3f s");
		}
		else {
			label_cell("Time");
			ImGui::TextDisabled("no clip duration to scrub");
		}
		return edited;
	}

	void rig_body(SceneEditor& ed, const entt::registry& reg, entt::entity, const Rig& rig) {
		prop_value("Nodes", "%zu", rig.nodes.size());
		prop_value("Joints", "%zu", rig.joints.size());

		if (ImGui::TreeNode("Nodes")) {
			for (std::size_t i = 0; i < rig.nodes.size(); ++i) {
				char name[24];
				std::snprintf(name, sizeof name, "[%zu]", i);
				entity_link(ed, reg, name, rig.nodes[i]);
			}
			ImGui::TreePop();
		}

		if (ImGui::TreeNode("Joints")) {
			for (std::size_t i = 0; i < rig.joints.size(); ++i) {
				char name[24];
				std::snprintf(name, sizeof name, "[%zu]", i);
				const RigJoint& joint = rig.joints[i];
				entity_link(ed, reg, name, rig.nodes[joint.node]);
				
			}
			ImGui::TreePop();
		}
	}

	void skin_palette_body(SceneEditor& ed, const entt::registry& reg, entt::entity, const SkinPalette& skin) {
		entity_link(ed, reg, "Rig", skin.rig);
		prop_value("Matrices", "%zu", skin.matrices.size());

		if (skin.matrices.size() > kShaderJointLimit) {
			ImGui::TextColored(kWarn, "exceeds the shader's %zu-joint uniform array", kShaderJointLimit);
		}
		if (reg.valid(skin.rig)) {
			if (const Rig* rig = reg.try_get<Rig>(skin.rig); rig && rig->joints.size() != skin.matrices.size()) {
				ImGui::TextColored(kWarn, "palette size differs from the rig's %zu joints", rig->joints.size());
			}
		}
	}

	void clip_body(SceneEditor&, const entt::registry&, entt::entity, const AnimationClip& clip) {
		prop_value("Name", "%s", clip.name.c_str());
		prop_value("Duration", "%.3f s", clip.duration);
		prop_value("Channels", "%zu", clip.channels.size());

		if (ImGui::TreeNode("Channel list")) {
			for (const AnimationChannel& ch : clip.channels) {
				ImGui::BulletText("node %u   keys: pos %zu, rot %zu, scale %zu",
					static_cast<unsigned>(ch.node),
					ch.position.times.size(), ch.rotation.times.size(), ch.scale.times.size());
			}
			ImGui::TreePop();
		}
	}

}
