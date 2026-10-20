#include <editor/scene/widgets.h>

#include <algorithm>
#include <cfloat>
#include <cmath>
#include <cstdarg>

#include <editor/scene/theme.h>

namespace roj::scene_ui {

	// ---- property grid --------------------------------------------------------

	float label_column() { return ImGui::GetFontSize() * 9.0f; }

	void label_cell(const char* label) {
		ImGui::AlignTextToFramePadding();
		ImGui::TextUnformatted(label);
		ImGui::SameLine(label_column());
	}

	void prop_label(const char* label) {
		label_cell(label);
		ImGui::SetNextItemWidth(-FLT_MIN);
	}

	bool prop_float(const char* label, float& v, float speed, float lo, float hi,
		const char* fmt, ImGuiSliderFlags flags) {
		ImGui::PushID(label);
		prop_label(label);
		const bool edited = ImGui::DragFloat("##v", &v, speed, lo, hi, fmt, flags | ImGuiSliderFlags_AlwaysClamp);
		ImGui::PopID();
		return edited;
	}

	bool prop_slider(const char* label, float& v, float lo, float hi, const char* fmt) {
		ImGui::PushID(label);
		prop_label(label);
		const bool edited = ImGui::SliderFloat("##v", &v, lo, hi, fmt);
		ImGui::PopID();
		return edited;
	}

	bool prop_check(const char* label, bool& v) {
		ImGui::PushID(label);
		label_cell(label);
		const bool edited = ImGui::Checkbox("##v", &v);
		ImGui::PopID();
		return edited;
	}

	void prop_value(const char* label, const char* fmt, ...) {
		label_cell(label);
		va_list args;
		va_start(args, fmt);
		ImGui::TextV(fmt, args);
		va_end(args);
	}

	bool prop_vec3(const char* label, glm::vec3& v, float speed) {
		static constexpr const char* kFmt[3] = { "X %.3f", "Y %.3f", "Z %.3f" };
		static constexpr ImU32 kTint[3] = { IM_COL32(150, 55, 55, 110), IM_COL32(65, 125, 55, 110), IM_COL32(55, 95, 155, 110) };

		ImGui::PushID(label);
		label_cell(label);

		const float width = (ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x * 2.0f) / 3.0f;
		bool edited = false;
		for (int i = 0; i < 3; ++i) {
			if (i > 0) ImGui::SameLine();
			ImGui::PushID(i);
			ImGui::PushStyleColor(ImGuiCol_FrameBg, kTint[i]);
			ImGui::SetNextItemWidth(width);
			edited |= ImGui::DragFloat("##v", &v[i], speed, 0.0f, 0.0f, kFmt[i]);
			ImGui::PopStyleColor();
			ImGui::PopID();
		}

		ImGui::PopID();
		return edited;
	}

	// ---- small controls -------------------------------------------------------

	bool link_button(const char* label) {
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
		ImGui::PushStyleColor(ImGuiCol_Text, kAccent);
		const bool clicked = ImGui::SmallButton(label);
		ImGui::PopStyleColor(2);
		if (ImGui::IsItemHovered()) ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
		return clicked;
	}

	float badge_height() { return ImGui::GetTextLineHeight() + 2.0f; }

	float draw_badge(ImDrawList* dl, ImVec2 pos, const Badge& badge) {
		const float width = ImGui::CalcTextSize(badge.text).x + 8.0f;
		dl->AddRectFilled(pos, ImVec2(pos.x + width, pos.y + badge_height()), badge.color, 3.0f);
		dl->AddText(ImVec2(pos.x + 4.0f, pos.y + 1.0f), IM_COL32(255, 255, 255, 235), badge.text);
		return width;
	}

	// ---- layout ---------------------------------------------------------------

	bool splitter(const char* id, float thickness, float total, float& ratio, float lo, float hi) {
		ImGui::InvisibleButton(id, ImVec2(thickness, ImGui::GetContentRegionAvail().y));

		const bool hovered = ImGui::IsItemHovered();
		const bool active = ImGui::IsItemActive();
		if (hovered || active) ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);

		bool changed = false;
		if (active) {
			const float next = std::clamp(ratio + ImGui::GetIO().MouseDelta.x / total, lo, hi);
			changed = next != ratio;
			ratio = next;
		}

		const ImVec2 a = ImGui::GetItemRectMin();
		const ImVec2 b = ImGui::GetItemRectMax();
		const ImU32 color = ImGui::GetColorU32(active ? ImGuiCol_SeparatorActive : hovered ? ImGuiCol_SeparatorHovered : ImGuiCol_Separator);
		const float x = std::floor((a.x + b.x) * 0.5f);
		ImGui::GetWindowDrawList()->AddLine(ImVec2(x, a.y), ImVec2(x, b.y), color, 2.0f);
		return changed;
	}

}
