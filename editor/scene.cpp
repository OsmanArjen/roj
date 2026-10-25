
#include <editor/scene.h>

#include <algorithm>
#include <cmath>

#include <imgui.h>
#include <editor/scene/inspector.h>
#include <editor/scene/theme.h>
#include <editor/scene/tree.h>
#include <editor/scene/widgets.h>

namespace roj {
	namespace {

		// Tree on the left, inspector on the right, a draggable divider between them.
		void draw_panels(SceneEditor& ed, entt::registry& reg) {
			constexpr float kBar = 7.0f;
			constexpr float kMinTree = 220.0f;      // fits "Hierarchy N" plus the Expand/Collapse buttons
			constexpr float kMinInspector = 300.0f; // label column plus three vec3 drag fields
			constexpr ImGuiWindowFlags kNoScroll = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;

			const float gap = ImGui::GetStyle().ItemSpacing.x;
			const float total = std::max(1.0f, ImGui::GetContentRegionAvail().x - kBar - 2.0f * gap);

			// Keep both panes usable; a window too narrow for both minimums falls back to an even split.
			// The stored ratio is only written when the user drags, so a temporarily narrow window doesn't lose it.
			const float lo = std::min(kMinTree / total, 0.5f);
			const float hi = std::max(1.0f - kMinInspector / total, 0.5f);
			float ratio = std::clamp(ed.splitRatio, lo, hi);
			const float treeWidth = std::floor(total * ratio);

			ImGui::BeginChild("##tree", ImVec2(treeWidth, 0.0f), scene_ui::kPanelBorder, kNoScroll);
			scene_ui::draw_tree_panel(ed, reg);
			ImGui::EndChild();

			ImGui::SameLine();
			if (scene_ui::splitter("##split", kBar, total, ratio, lo, hi)) ed.splitRatio = ratio;
			ImGui::SameLine();

			ImGui::BeginChild("##inspector", ImVec2(0.0f, 0.0f), scene_ui::kPanelBorder);
			scene_ui::draw_inspector(ed, reg);
			ImGui::EndChild();
		}

	} // namespace

	void draw_scene_editor(SceneEditor& ed, entt::registry& reg) {
		scene_ui::StyleScope style;
		ImGui::SetNextWindowSize(ImVec2(860.0f, 360.0f), ImGuiCond_FirstUseEver);

		if (ImGui::Begin("Scene", nullptr, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse)) {
			scene_ui::update_tree(ed, reg);
			draw_panels(ed, reg);
		}
		ImGui::End();
	}

}
