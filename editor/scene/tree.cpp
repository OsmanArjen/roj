#include <editor/scene/tree.h>

#include <algorithm>
#include <cstdio>
#include <utility>

#include <imgui.h>
#include <entt/entity/registry.hpp>
#include <entity/transform.h>
#include <editor/scene/component_table.h>
#include <editor/scene/entity_ui.h>
#include <editor/scene/hierarchy.h>
#include <editor/scene/widgets.h>

namespace roj::scene_ui {

	namespace {

		// ---- model ----------------------------------------------------------------

		void validate_selection(SceneEditor& ed, const entt::registry& reg) {
			if (ed.selected != entt::null && !reg.valid(ed.selected)) ed.selected = entt::null;
		}

		// Selection changed from outside the tree (an entity link): open its ancestors and scroll to it.
		void sync_reveal(SceneEditor& ed, const entt::registry& reg) {
			if (ed.selected == ed.revealed) return;
			collect_ancestors(reg, ed.selected, ed.revealPath);
			ed.revealed = ed.selected;
			ed.scrollToSelected = ed.selected != entt::null && reg.all_of<Transform>(ed.selected);
		}

		void rebuild_tree(SceneEditor& ed, const entt::registry& reg) {
			ed.edges.clear();
			ed.roots.clear();

			for (const entt::entity e : reg.view<const Transform>()) {
				const Node* node = reg.try_get<Node>(e);
				const entt::entity parent = node ? parent_of(reg, e, *node) : entt::entity{ entt::null };

				if (parent == entt::null) ed.roots.push_back(e);
				else ed.edges.push_back({ parent, e });
			}

			// Stable, so siblings keep storage order.
			std::stable_sort(ed.edges.begin(), ed.edges.end(),
				[](const SceneEditor::Edge& a, const SceneEditor::Edge& b) { return a.parent < b.parent; });
		}

		auto children_of(const SceneEditor& ed, entt::entity parent) {
			const auto first = std::lower_bound(ed.edges.begin(), ed.edges.end(), parent,
				[](const SceneEditor::Edge& edge, entt::entity p) { return edge.parent < p; });
			auto last = first;
			while (last != ed.edges.end() && last->parent == parent) ++last;
			return std::pair{ first, last };
		}

		// ---- drawing --------------------------------------------------------------

		void draw_node(SceneEditor& ed, const entt::registry& reg, entt::entity e) {
			const auto [first, last] = children_of(ed, e);

			ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick
				| ImGuiTreeNodeFlags_SpanAvailWidth;
			if (first == last) flags |= ImGuiTreeNodeFlags_Leaf;
			if (e == ed.selected) flags |= ImGuiTreeNodeFlags_Selected;

			char label[64];
			entity_label(ed, reg, e, label, sizeof label);

			ImGui::PushID(static_cast<int>(entt::to_integral(e)));

			if (ed.treeCommand != SceneEditor::TreeCommand::None) {
				ImGui::SetNextItemOpen(ed.treeCommand == SceneEditor::TreeCommand::ExpandAll, ImGuiCond_Always);
			}
			else if (!ed.revealPath.empty() && std::find(ed.revealPath.begin(), ed.revealPath.end(), e) != ed.revealPath.end()) {
				ImGui::SetNextItemOpen(true, ImGuiCond_Always);
			}

			const bool open = ImGui::TreeNodeEx("node", flags, "%s", label);

			if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
				ed.selected = e;
				ed.revealed = e; // already visible, nothing to reveal
			}
			if (e == ed.selected && ed.scrollToSelected) {
				ImGui::SetScrollHereY();
				ed.scrollToSelected = false;
			}

			// Badges sit right after the label.
			const ImVec2 rowMin = ImGui::GetItemRectMin();
			const float rowHeight = ImGui::GetItemRectSize().y;
			draw_badges_at(reg, e, ImVec2(
				rowMin.x + ImGui::GetTreeNodeToLabelSpacing() + ImGui::CalcTextSize(label).x + 8.0f,
				rowMin.y + (rowHeight - badge_height()) * 0.5f));

			if (open) {
				for (auto it = first; it != last; ++it) draw_node(ed, reg, it->child);
				ImGui::TreePop();
			}
			ImGui::PopID();
		}

		void draw_tree(SceneEditor& ed, const entt::registry& reg) {
			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(6.0f, 3.0f)); // denser rows than the inspector

			if (ed.roots.empty()) {
				ImGui::TextDisabled("No entities with a Transform");
			}
			else {
				for (const entt::entity root : ed.roots) draw_node(ed, reg, root);
			}

			if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !ImGui::IsAnyItemHovered()) {
				ed.selected = entt::null; // click on empty space to deselect
				ed.revealed = entt::null;
			}

			ed.treeCommand = SceneEditor::TreeCommand::None;
			ed.revealPath.clear();
			ImGui::PopStyleVar();
		}

		// Title and entity count on the left, expand/collapse on the right.
		void draw_tree_header(SceneEditor& ed, const entt::registry& reg) {
			using Command = SceneEditor::TreeCommand;
			const ImGuiStyle& style = ImGui::GetStyle();

			const auto* transforms = reg.storage<Transform>();
			const std::size_t count = transforms ? transforms->size() : 0;

			ImGui::AlignTextToFramePadding();
			ImGui::TextUnformatted("Hierarchy");
			ImGui::SameLine();
			ImGui::TextDisabled("%zu", count);

			const float expandWidth = ImGui::CalcTextSize("Expand").x + style.FramePadding.x * 2.0f;
			const float collapseWidth = ImGui::CalcTextSize("Collapse").x + style.FramePadding.x * 2.0f;
			const float buttonsWidth = expandWidth + collapseWidth + style.ItemSpacing.x;

			ImGui::SameLine(ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x - buttonsWidth);
			if (ImGui::Button("Expand")) ed.treeCommand = Command::ExpandAll;
			ImGui::SameLine();
			if (ImGui::Button("Collapse")) ed.treeCommand = Command::CollapseAll;
		}

	} // namespace

	void update_tree(SceneEditor& ed, const entt::registry& reg) {
		validate_selection(ed, reg);
		sync_reveal(ed, reg);
		rebuild_tree(ed, reg);
	}

	// The header stays put; only the node list scrolls.
	void draw_tree_panel(SceneEditor& ed, const entt::registry& reg) {
		draw_tree_header(ed, reg);
		ImGui::Separator();
		ImGui::BeginChild("##nodes", ImVec2(0.0f, 0.0f));
		draw_tree(ed, reg);
		ImGui::EndChild();
	}

}
