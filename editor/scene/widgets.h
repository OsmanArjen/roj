// Generic ImGui building blocks: the property grid, link buttons, badges and the splitter.
// Knows nothing about entt or the scene; depends only on ImGui, glm and theme.h.
#pragma once
#include <glm/vec3.hpp>
#include <imgui.h>

namespace roj::scene_ui {

	// ---- property grid --------------------------------------------------------
	// Every label sits in one fixed column, so all widgets line up.

	float label_column();
	void label_cell(const char* label);  // label, then moves the cursor to the widget column
	void prop_label(const char* label);  // label_cell + a full-width next item

	bool prop_float(const char* label, float& v, float speed, float lo, float hi,
		const char* fmt = "%.3f", ImGuiSliderFlags flags = 0);
	bool prop_slider(const char* label, float& v, float lo, float hi, const char* fmt);
	bool prop_check(const char* label, bool& v);
	void prop_value(const char* label, const char* fmt, ...); // read-only printf-style value

	// Three drag fields tinted per axis (X red, Y green, Z blue).
	bool prop_vec3(const char* label, glm::vec3& v, float speed);

	// ---- small controls -------------------------------------------------------

	// Looks like a link, behaves like a button.
	bool link_button(const char* label);

	// Small coloured tag identifying a component kind.
	struct Badge {
		const char* text = nullptr;
		ImU32 color = 0;
	};

	float badge_height();
	float draw_badge(ImDrawList* dl, ImVec2 pos, const Badge& badge); // returns the width drawn

	// ---- layout ---------------------------------------------------------------

	// Draggable vertical divider between two panes; place it between two SameLine()'d children.
	// ratio is the first pane's share of `total`, kept within [lo, hi]. Returns true when it changed.
	bool splitter(const char* id, float thickness, float total, float& ratio, float lo, float hi);

}
