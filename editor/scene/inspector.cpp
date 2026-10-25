#include <editor/scene/inspector.h>

#include <algorithm>
#include <string>

#include <imgui.h>
#include <entt/entity/registry.hpp>
#include <entt/locator/locator.hpp>
#include <entt/meta/context.hpp>
#include <entt/meta/resolve.hpp>
#include <entt/tools/davey.hpp>
#include <editor/scene/component_table.h>
#include <editor/scene/entity_ui.h>
#include <editor/scene/hierarchy.h>
#include <editor/scene/theme.h>
#include <editor/scene/widgets.h>

namespace roj::scene_ui {

	namespace {

		// Path from the root: "... > grandparent > parent", each a link that selects it.
		void draw_breadcrumbs(SceneEditor& ed, const entt::registry& reg, entt::entity e) {
			collect_ancestors(reg, e, ed.crumbs);
			if (ed.crumbs.empty()) return;

			constexpr std::size_t kMaxCrumbs = 3;
			const std::size_t shown = std::min(ed.crumbs.size(), kMaxCrumbs);

			bool first = true;
			if (ed.crumbs.size() > shown) {
				ImGui::TextDisabled("...");
				first = false;
			}

			for (std::size_t i = shown; i-- > 0;) { // farthest first
				if (!first) {
					ImGui::SameLine(0.0f, 2.0f);
					ImGui::TextDisabled(">");
					ImGui::SameLine(0.0f, 2.0f);
				}
				first = false;

				char label[64];
				entity_label(ed, reg, ed.crumbs[i], label, sizeof label);
				ImGui::PushID(static_cast<int>(i));
				if (link_button(label)) ed.selected = ed.crumbs[i];
				ImGui::PopID();
			}
		}

		// Everything without a section: reflected components show their fields, the rest their type name.
		void draw_other_components(SceneEditor& ed, const entt::registry& reg, entt::entity e) {
			const entt::meta_ctx& ctx = entt::locator<entt::meta_ctx>::value_or();

			for ([[maybe_unused]] auto [id, storage] : reg.storage()) {
				const entt::type_info& info = storage.info();
				if (!storage.contains(e)) continue;
				if (const ComponentUi* ui = find_ui(info.hash()); ui && (ui->draw || ui->hidden)) continue;

				ImGui::PushID(&info);

				if (const auto type = entt::resolve(ctx, info); type) {
					const std::string name = type.name().empty() ? std::string{ info.name() } : std::string{ type.name() };

					if (ImGui::CollapsingHeader(name.c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
						if (const auto obj = type.from_void(storage.value(e)); obj) {
							entt::internal::present_element<entt::entity>(obj,
								[&](const char* field, entt::entity other) { entity_link(ed, reg, field, other); });
						}
						ImGui::Spacing();
					}
				}
				else {
					const auto name = info.name();
					ImGui::BulletText("%.*s", static_cast<int>(name.size()), name.data());
				}

				ImGui::PopID();
			}
		}

	} // namespace

	void draw_inspector(SceneEditor& ed, entt::registry& reg) {
		if (ed.selected == entt::null) {
			const char* message = "Select an entity";
			const ImVec2 avail = ImGui::GetContentRegionAvail();
			const ImVec2 size = ImGui::CalcTextSize(message);
			ImGui::SetCursorPos(ImVec2(
				ImGui::GetCursorPosX() + std::max(0.0f, (avail.x - size.x) * 0.5f),
				ImGui::GetCursorPosY() + avail.y * 0.4f));
			ImGui::TextDisabled("%s", message);
			return;
		}

		const entt::entity e = ed.selected;

		// Header: name, id and component badges on one row, then the path to the root.
		char label[64];
		entity_label(ed, reg, e, label, sizeof label);
		ImGui::TextColored(kAccent, "%s", label);
		ImGui::SameLine();
		ImGui::TextDisabled("#%u  v%u", static_cast<unsigned>(entt::to_entity(e)), static_cast<unsigned>(entt::to_version(e)));

		// Badges share the title row, which keeps the header compact in a short panel.
		ImGui::SameLine();
		const ImVec2 badgePos = ImGui::GetCursorScreenPos();
		const float badgesWidth = draw_badges_at(reg, e, ImVec2(
			badgePos.x + 4.0f, badgePos.y + (ImGui::GetTextLineHeight() - badge_height()) * 0.5f));
		ImGui::Dummy(ImVec2(badgesWidth + 4.0f, ImGui::GetTextLineHeight())); // advances the layout past the badges

		draw_breadcrumbs(ed, reg, e);

		ImGui::Separator();

		// Sections use a quieter header colour than the tree's selection highlight.
		ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.19f, 0.21f, 0.26f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.24f, 0.27f, 0.33f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.28f, 0.31f, 0.38f, 1.0f));

		for (const ComponentUi& row : component_table()) {
			if (!row.draw || !has_component(reg, row.type, e)) continue;

			ImGui::PushID(&row);
			const bool open = ImGui::CollapsingHeader(row.title, ImGuiTreeNodeFlags_DefaultOpen);

			if (row.badge.text) { // same tag as in the tree, right-aligned on the header
				const ImVec2 mn = ImGui::GetItemRectMin();
				const ImVec2 mx = ImGui::GetItemRectMax();
				const float width = ImGui::CalcTextSize(row.badge.text).x + 8.0f;
				draw_badge(ImGui::GetWindowDrawList(),
					ImVec2(mx.x - width - 8.0f, mn.y + (mx.y - mn.y - badge_height()) * 0.5f), row.badge);
			}

			if (open) {
				row.draw(ed, reg, e);
				ImGui::Spacing();
			}
			ImGui::PopID();
		}

		draw_other_components(ed, reg, e);
		ImGui::PopStyleColor(3);
	}

}
